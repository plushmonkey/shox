#include "Platform.h"

#include <Windows.h>
#include <commdlg.h>
#include <tlhelp32.h>

#include <vector>

namespace shox {

static bool g_HasDebugToken = false;

void DisplayError(const char* error) {
  MessageBox(NULL, error, "Error", MB_OK | MB_ICONERROR);
}

static bool GetDebugPrivileges() {
  if (g_HasDebugToken) return true;

  HANDLE token = nullptr;
  bool success = false;

  if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &token)) {
    TOKEN_PRIVILEGES privileges = {};

    LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &privileges.Privileges[0].Luid);

    privileges.PrivilegeCount = 1;
    privileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    if (AdjustTokenPrivileges(token, FALSE, &privileges, sizeof(TOKEN_PRIVILEGES), 0, 0)) {
      DWORD last_error = GetLastError();
      success = last_error == ERROR_SUCCESS;
    }

    CloseHandle(token);
  }

  if (success) {
    g_HasDebugToken = true;
  }

  return success;
}

enum class FindProcessError { Success, CreateSnapshot, ProcessEnumeration };

struct FindProcessResult {
  FindProcessError error = FindProcessError::Success;
  std::vector<DWORD> pids;
};

static FindProcessResult FindProcesses(const char* find) {
  FindProcessResult result;

  HANDLE snapshot = INVALID_HANDLE_VALUE;

  if ((snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0)) == INVALID_HANDLE_VALUE) {
    result.error = FindProcessError::CreateSnapshot;
    return result;
  }

  PROCESSENTRY32 entry = {};
  entry.dwSize = sizeof(PROCESSENTRY32);

  if (!Process32First(snapshot, &entry)) {
    CloseHandle(snapshot);
    result.error = FindProcessError::ProcessEnumeration;
    return result;
  }

  do {
    if (_stricmp(find, entry.szExeFile) == 0) {
      result.pids.push_back(entry.th32ProcessID);
    }
  } while (Process32Next(snapshot, &entry));

  CloseHandle(snapshot);

  return result;
}

std::optional<ReadFileResult> SelectAndReadFile(const wchar_t* filter) {
  OPENFILENAMEW ofn = {};
  std::wstring filename;

  filename.resize(1024 * 8, 0);

  ofn.lStructSize = sizeof(ofn);
  ofn.lpstrFilter = filter;
  ofn.lpstrFile = filename.data();
  ofn.nMaxFile = (DWORD)filename.size();
  ofn.nFilterIndex = 0;
  ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

  if (!GetOpenFileNameW(&ofn)) {
    return {};
  }

  HANDLE ofh =
      CreateFileW(filename.data(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

  if (!ofh) {
    DisplayError("Failed to open file for reading.");
    return {};
  }

  LARGE_INTEGER large_size;
  std::string data;

  if (GetFileSizeEx(ofh, &large_size)) {
    if (large_size.QuadPart > 0xFFFFFFFF) {
      DisplayError("Selected file is too large.");
      CloseHandle(ofh);
      return {};
    }

    data.resize((u32)large_size.QuadPart);
    DWORD amount_read = 0;
    DWORD total_read = 0;

    while (total_read < data.size()) {
      if (!ReadFile(ofh, data.data() + total_read, (DWORD)data.size() - total_read, &amount_read, NULL)) {
        CloseHandle(ofh);
        return {};
      }

      total_read += amount_read;
    }
  }

  CloseHandle(ofh);

  if (data.empty()) return {};

  ReadFileResult result;

  result.filename = filename;
  result.contents = data;

  return result;
}

std::optional<ExeProcess> ExeProcess::OpenByName(const char* name) {
  if (!GetDebugPrivileges()) {
    DisplayError("Failed to get OpenProcess privileges. Run as administrator.");
    return {};
  }

  FindProcessResult result = FindProcesses(name);

  if (result.error != FindProcessError::Success) {
    switch (result.error) {
      case FindProcessError::CreateSnapshot: {
        DisplayError("Failed to create snapshot of running processes.");
      } break;
      case FindProcessError::ProcessEnumeration: {
        DisplayError("Failed to enumerate running processes.");
      } break;
      default: {
        DisplayError("Unknown error while finding process.");
      } break;
    }
    return {};
  }

  if (result.pids.empty()) return {};

  ExeProcess process;

  process.win32_handle = OpenProcess(PROCESS_VM_READ, FALSE, result.pids[0]);

  if (!process.win32_handle) {
    DisplayError("Failed to OpenProcess with PROCESS_VM_READ.");
    return {};
  }

  return process;
}

std::optional<u32> ExeProcess::GetU32(uintptr_t address) const {
  u32 value = 0;
  SIZE_T num_read;

  if (ReadProcessMemory(win32_handle, (LPVOID)address, &value, sizeof(u32), &num_read)) return value;

  return {};
}

std::optional<std::string> ExeProcess::GetString(uintptr_t address, size_t len) const {
  std::string value;
  SIZE_T read;

  value.resize(len);

  if (ReadProcessMemory(win32_handle, (LPVOID)address, const_cast<char*>(value.c_str()), len, &read)) return value;

  return {};
}

std::optional<std::string> ExeProcess::GetString(uintptr_t address) const {
  std::string value;
  char c = 1;

  while (true) {
    if (!ReadProcessMemory(win32_handle, (LPVOID)address, &c, 1, nullptr)) {
      return {};
    }

    if (c == 0) break;

    value += c;
  }

  return value;
}

}  // namespace shox
