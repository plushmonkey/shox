#include <shox/ArenaSettings.h>
#include <shox/Draw.h>
#include <shox/Generator.h>
#include <shox/Image.h>
#include <shox/Platform.h>
#include <stdio.h>
#include <string.h>

#include <atomic>
#include <memory>
#include <thread>
//
#include <Windows.h>
//
#include <Uxtheme.h>
#include <commctrl.h>
//
#include "resource.h"

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "UxTheme.lib")

// This is necessary to get visual styles.
#pragma comment(linker, \
                "\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#define ID_CREATEBUTTON 101
#define ID_TYPESELECTOR 102
#define ID_EXPLAINLABEL 103
#define ID_STATUSLABEL 104
#define ID_COLORCHECKBOX 105

#define WM_GENERATE_COMPLETE WM_USER + 0x123

#define WINDOW_BACKGROUND_COLOR RGB(32, 32, 32)
#define STATIC_BACKGROUND_COLOR RGB(60, 60, 60)

struct ShoxApp {
  HWND hwnd = nullptr;
  HWND button_hwnd = nullptr;
  HWND type_selector_hwnd = nullptr;
  HWND explain_label_hwnd = nullptr;
  HWND status_label_hwnd = nullptr;
  HWND color_option_hwnd = nullptr;

  std::thread generate_thread;

  std::string working_directory;
  std::unique_ptr<shox::ShipGenerator> generator;

  inline void SetStatus(const std::string& status) const {
    SendMessage(status_label_hwnd, WM_SETTEXT, 0, (LPARAM)status.data());
    ShowWindow(status_label_hwnd, 1);
  }

  bool BeginGeneration(shox::GenerateType generate_type) {
    auto opt_process = shox::ExeProcess::OpenByName("Continuum.exe");
    if (!opt_process) {
      SetStatus("Failed to find running Continuum process.");
      return false;
    }

    shox::ExeProcess process = *opt_process;

    auto opt_directory = process.GetDirectory();
    if (!opt_directory) {
      SetStatus("Failed to read Continuum's working directory.");
      return false;
    }

    auto opt_game_addr = process.GetU32(0x4C1AFC);
    if (!opt_game_addr) {
      SetStatus("Failed to read game address.");
      return false;
    }

    auto opt_settings = process.Get<shox::ArenaSettings>(*opt_game_addr + 0x127ec + 0x1AE70);
    if (!opt_settings || opt_settings->Type != 0x0F) {
      SetStatus("Failed to read arena settings.");
      return false;
    }

    int base_x_dim, base_y_dim;

    const wchar_t* kDialogTitles[] = {
        L"Select base bombs file.",
        L"Select base mines file.",
        L"Select base ship file.",
    };

    const wchar_t* kFilters[] = {
        L"Bombs\0bombs.bm2;bombs.bmp;bombs.png\0Images (bm2,bmp,png)\0*.bm2;*.bmp;*.png\0All Files\0*.*\0\0",
        L"Mines\0mines.bm2;mines.bmp;mines.png\0Images (bm2,bmp,png)\0*.bm2;*.bmp;*.png\0All Files\0*.*\0\0",
        L"Ships\0ships.bm2;ships.bmp;ships.png\0Images (bm2,bmp,png)\0*.bm2;*.bmp;*.png\0All Files\0*.*\0\0",
    };

    std::wstring initial_dir = *opt_directory + L"graphics\\";
    size_t type_index = (size_t)generate_type;

    auto opt_file_data = shox::SelectAndReadFile(kDialogTitles[type_index], kFilters[type_index], initial_dir.data());
    if (!opt_file_data) {
      SetStatus("Invalid file selected.");
      return false;
    }

    shox::ReadFileResult file_data = *opt_file_data;

    u8* data = shox::ImageLoadFromMemory((u8*)file_data.contents.data(), (int)file_data.contents.size(), &base_x_dim,
                                         &base_y_dim);
    if (!data) {
      SetStatus("Failed to load image data from provided file.");
      return false;
    }

    shox::ArenaSettings settings = *opt_settings;
    shox::Bitmap src_bitmap(data, base_x_dim, base_y_dim);

    generator = std::make_unique<shox::ShipGenerator>(src_bitmap, settings, generate_type);
    generator->working_directory = working_directory;

    generator->use_colors = (bool)SendMessage(this->color_option_hwnd, BM_GETCHECK, 0, 0);

    generate_thread = std::thread(&ShoxApp::RunGeneration, this);
    SetStatus("Generating...");

    EnableWindow(button_hwnd, FALSE);

    return true;
  }

  void RunGeneration() {
    bool success = generator->Generate();

    PostMessage(hwnd, WM_GENERATE_COMPLETE, (WPARAM)success, 0);
  }
};

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  ShoxApp* shox = (ShoxApp*)GetWindowLongPtrA(hwnd, GWLP_USERDATA);

  switch (msg) {
    case WM_CREATE: {
      CREATESTRUCT* createstruct = (CREATESTRUCT*)lParam;
      shox = (ShoxApp*)createstruct->lpCreateParams;

      SetWindowLongPtrA(hwnd, GWLP_USERDATA, (LONG_PTR)shox);

      RECT rect = {};

      if (!GetClientRect(hwnd, &rect)) {
        PostQuitMessage(0);
        return 0;
      }

      int surface_width = rect.right - rect.left;
      int surface_height = rect.bottom - rect.top;

      const char* explain_text =
          "Make sure Continuum is running for the settings.\nSelect the type of image to be generated below.\nWhen the "
          "file selector pops up, choose the base image from your graphics folder.";

      shox->explain_label_hwnd = CreateWindowEx(0, WC_STATIC, explain_text, WS_VISIBLE | WS_CHILD, 10, 10,
                                                surface_width - 20, surface_height / 2, hwnd, (HMENU)ID_EXPLAINLABEL,
                                                (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);

      shox->type_selector_hwnd =
          CreateWindowEx(0, WC_COMBOBOX, "", WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST, 5, surface_height - 30, 80, 20,
                         hwnd, (HMENU)ID_TYPESELECTOR, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);

      shox->button_hwnd =
          CreateWindowEx(0, WC_BUTTON, "Create", WS_VISIBLE | WS_CHILD, 130, surface_height - 25, 80, 20, hwnd,
                         (HMENU)ID_CREATEBUTTON, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);

      shox->status_label_hwnd =
          CreateWindowEx(0, WC_STATIC, "", WS_CHILD, 10, surface_height / 2 + 20, surface_width - 20, 20, hwnd,
                         (HMENU)ID_STATUSLABEL, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);

      shox->color_option_hwnd = CreateWindowEx(
          WS_EX_TRANSPARENT, WC_BUTTON, "Colored", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX, 100, surface_height - 28,
          100, 20, hwnd, (HMENU)ID_COLORCHECKBOX, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);

      // Set the theme to something invalid so it can be colored.
      SetWindowTheme(shox->color_option_hwnd, L"", L"");
      SendMessage(shox->color_option_hwnd, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);

      RECT create_rect = {};

      if (GetClientRect(shox->button_hwnd, &create_rect)) {
        s32 width = create_rect.right - create_rect.left;
        s32 height = create_rect.bottom - create_rect.top;
        MoveWindow(shox->button_hwnd, surface_width - width - 8, surface_height - 28, width, height, TRUE);
      }

      const char* kSelections[] = {"Bombs", "Mines", "Ball"};
      for (size_t i = 0; i < sizeof(kSelections) / sizeof(*kSelections); ++i) {
        SendMessage(shox->type_selector_hwnd, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)kSelections[i]);
      }

      SendMessage(shox->type_selector_hwnd, CB_SETCURSEL, (WPARAM)0, (LPARAM)0);
    } break;
    case WM_CTLCOLORSTATIC: {
      static HBRUSH brush = CreateSolidBrush(STATIC_BACKGROUND_COLOR);

      HDC hdc = (HDC)wParam;
      HWND static_hwnd = (HWND)lParam;

      SetTextColor(hdc, RGB(255, 255, 255));
      SetBkMode(hdc, TRANSPARENT);

      if (static_hwnd == shox->color_option_hwnd) {
        static HBRUSH dark_brush = CreateSolidBrush(WINDOW_BACKGROUND_COLOR);

        return (INT_PTR)dark_brush;
      }

      return (INT_PTR)brush;
    } break;
    case WM_SETFOCUS: {
      InvalidateRect(shox->color_option_hwnd, NULL, FALSE);
      UpdateWindow(shox->color_option_hwnd);
    } break;
    case WM_COMMAND: {
      u32 id = LOWORD(wParam);

      switch (id) {
        case ID_CREATEBUTTON: {
          int index = (int)SendMessage(shox->type_selector_hwnd, CB_GETCURSEL, 0, 0);

          if (index >= 0 && index <= (int)shox::GenerateType::Count) {
            shox::GenerateType type = (shox::GenerateType)index;

            shox->BeginGeneration(type);
          } else {
            shox->SetStatus("Invalid type index.");
          }
        } break;
        case ID_TYPESELECTOR: {
          ShowWindow(shox->status_label_hwnd, 0);

          int index = (int)SendMessage(shox->type_selector_hwnd, CB_GETCURSEL, 0, 0);

          ShowWindow(shox->color_option_hwnd, index != (int)shox::GenerateType::Ball);
        } break;
        default: {
        } break;
      }
    } break;
    case WM_GENERATE_COMPLETE: {
      bool success = (bool)wParam;

      shox->generate_thread.join();
      shox->generate_thread = std::thread();

      if (success) {
        shox->SetStatus("Generation complete.");
      } else {
        shox->SetStatus("Error: " + shox->generator->error_message);
      }

      EnableWindow(shox->button_hwnd, TRUE);
    } break;
    case WM_CLOSE: {
      DestroyWindow(hwnd);
    } break;
    case WM_DESTROY: {
      PostQuitMessage(0);
    } break;
    default: {
    } break;
  }

  return DefWindowProc(hwnd, msg, wParam, lParam);
}

int CALLBACK WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine,
                     _In_ int nCmdShow) {
  const char* kClassName = "shox_window";

  WNDCLASSEX wc = {};

  wc.cbSize = sizeof(wc);
  wc.style = 0;
  wc.lpfnWndProc = WndProc;
  wc.hInstance = hInstance;
  wc.hIcon = (HICON)LoadImage(hInstance, MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON, GetSystemMetrics(SM_CXICON),
                              GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR);
  wc.hIconSm = (HICON)LoadImage(hInstance, MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON),
                                GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  wc.hbrBackground = CreateSolidBrush(WINDOW_BACKGROUND_COLOR);
  wc.lpszClassName = kClassName;

  if (!RegisterClassEx(&wc)) {
    MessageBoxA(NULL, "Failed to register window.", "Error", MB_ICONERROR | MB_OK);
    return 1;
  }

  INITCOMMONCONTROLSEX icex = {};

  icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
  icex.dwICC = ICC_STANDARD_CLASSES;

  InitCommonControlsEx(&icex);

  int width = 400;
  int height = 200;

  ShoxApp* shox = new ShoxApp();

  auto opt_working_directory = shox::GetWorkingDirectory();
  if (opt_working_directory) {
    shox->working_directory = *opt_working_directory;
  }

  DWORD style = WS_OVERLAPPEDWINDOW & ~(WS_THICKFRAME | WS_MAXIMIZEBOX);
  shox->hwnd = CreateWindowEx(0, kClassName, "shox", style, CW_USEDEFAULT, CW_USEDEFAULT, width, height, NULL, NULL,
                              hInstance, shox);
  if (!shox->hwnd) {
    MessageBoxA(NULL, "Failed to create window.", "Error", MB_ICONERROR | MB_OK);
    return 1;
  }

  ShowWindow(shox->hwnd, nCmdShow);
  UpdateWindow(shox->hwnd);

  MSG msg = {};

  while (GetMessage(&msg, NULL, 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  return 0;
}
