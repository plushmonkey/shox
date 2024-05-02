#include <shox/ArenaSettings.h>
#include <shox/Draw.h>
#include <shox/Generator.h>
#include <shox/Image.h>
#include <shox/Platform.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char* argv[]) {
  shox::GenerateType generate_type = shox::GenerateType::Ball;

  for (int i = 1; i < argc; ++i) {
    if (_stricmp(argv[i], "--bomb") == 0) {
      generate_type = shox::GenerateType::Bomb;
    }
  }

  auto opt_process = shox::ExeProcess::OpenByName("Continuum.exe");
  if (!opt_process) {
    shox::DisplayError("Failed to find running Continuum process.");
    return 1;
  }

  shox::ExeProcess process = *opt_process;

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

  auto opt_file_data = shox::SelectAndReadFile(L"*.bm2");
  if (!opt_file_data) {
    return 1;
  }

  shox::ReadFileResult file_data = *opt_file_data;

  u8* data = shox::ImageLoadFromMemory((u8*)file_data.contents.data(), (int)file_data.contents.size(), &base_x_dim,
                                       &base_y_dim);
  if (!data) {
    shox::DisplayError("Failed to load image data from ships file.");
    return 1;
  }

  shox::ArenaSettings settings = *opt_settings;
  shox::Bitmap src_bitmap(data, base_x_dim, base_y_dim);

  shox::ShipGenerator generator(src_bitmap, settings, generate_type);

  for (int i = 0; i < 8; ++i) {
    if (!generator.Generate(i)) {
      shox::DisplayError("Failed to generate ship.");
      return 1;
    }
  }

  return 0;
}
