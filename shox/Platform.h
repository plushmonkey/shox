#pragma once

#include <shox/Types.h>

#include <optional>
#include <string>

namespace shox {

struct ExeProcess {
  static std::optional<ExeProcess> OpenByName(const char* name);

  std::optional<u32> GetU32(uintptr_t addr) const;
  // Reads the entire length. Use without len argument to get null-terminated string.
  std::optional<std::string> GetString(uintptr_t addr, size_t len) const;
  // Reads until a null terminated character is found.
  std::optional<std::string> GetString(uintptr_t addr) const;

  template <typename T>
  std::optional<T> Get(uintptr_t addr) const {
    auto opt_data = GetString(addr, sizeof(T));
    if (!opt_data) return {};

    T result = *(const T*)opt_data->data();

    return result;
  }

  std::optional<std::wstring> GetDirectory() const;

 private:
  ExeProcess() {}
  void* win32_handle = (void*)~0;
};

void DisplayError(const char* error);

struct ReadFileResult {
  std::wstring filename;
  std::string contents;
};
std::optional<ReadFileResult> SelectAndReadFile(const wchar_t* dialog_title, const wchar_t* filter,
                                                const wchar_t* initial_dir);

std::optional<std::string> GetWorkingDirectory();

}  // namespace shox
