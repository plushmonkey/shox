#pragma once

#include <shox/ArenaSettings.h>
#include <shox/Draw.h>
#include <shox/Types.h>

#include <string>

namespace shox {

struct ArenaSettings;

enum class GenerateType {
  Bomb,
  Mine,
  Ball,

  Count
};

struct ShipGenerator {
  ArenaSettings settings;
  Bitmap src_bitmap;

  std::string working_directory;

  GenerateType type = GenerateType::Ball;
  bool use_colors = true;
  std::string error_message;

  ShipGenerator(Bitmap src_bitmap, const ArenaSettings& settings, GenerateType type)
      : src_bitmap(src_bitmap), settings(settings), type(type) {}

  bool Generate();

 private:
  bool Generate(int ship);
};

}  // namespace shox
