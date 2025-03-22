#include <shox/ArenaSettings.h>
#include <shox/Draw.h>
#include <shox/Generator.h>
#include <shox/Image.h>
#include <shox/Platform.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char* argv[]) {
  auto opt_working_directory = shox::GetWorkingDirectory();

  shox::GenerateType generate_type = shox::GenerateType::Ball;

  for (int i = 1; i < argc; ++i) {
    if (_stricmp(argv[i], "--bomb") == 0) {
      generate_type = shox::GenerateType::Bomb;
    } else if (_stricmp(argv[i], "--bombs") == 0) {
      generate_type = shox::GenerateType::Bomb;
    } else if (_stricmp(argv[i], "--mine") == 0) {
      generate_type = shox::GenerateType::Mine;
    } else if (_stricmp(argv[i], "--mines") == 0) {
      generate_type = shox::GenerateType::Mine;
    }
  }

  auto opt_process = shox::ExeProcess::OpenByName("Continuum.exe");
  if (!opt_process) {
    shox::DisplayError("Failed to find running Continuum process.");
    return 1;
  }

  shox::ExeProcess process = *opt_process;

  auto opt_directory = process.GetDirectory();
  if (!opt_directory) {
    shox::DisplayError("Failed to read Continuum's working directory.");
    return 1;
  }

  auto opt_game_addr = process.GetU32(0x4C1AFC);
  if (!opt_game_addr) {
    shox::DisplayError("Failed to read game address.");
    return 1;
  }

  auto opt_settings = process.Get<shox::ArenaSettings>(*opt_game_addr + 0x127ec + 0x1AE70);
  if (!opt_settings) {
    shox::DisplayError("Failed to read arena settings.");
    return 1;
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
    return 1;
  }

  shox::ReadFileResult file_data = *opt_file_data;

  u8* data = shox::ImageLoadFromMemory((u8*)file_data.contents.data(), (int)file_data.contents.size(), &base_x_dim,
                                       &base_y_dim);
  if (!data) {
    shox::DisplayError("Failed to load image data from provided file.");
    return 1;
  }

  shox::ArenaSettings settings = *opt_settings;
  shox::Bitmap src_bitmap(data, base_x_dim, base_y_dim);

  shox::ShipGenerator generator(src_bitmap, settings, generate_type);

  if (opt_working_directory) {
    generator.working_directory = *opt_working_directory;
  }

  generator.Generate();

  return 0;
}
