#pragma once

#include <shox/Draw.h>
#include <shox/Types.h>

namespace shox {

struct ArenaSettings;

enum class GenerateType {
  Bomb,
  Ball,
};

struct ShipGenerator {
  const ArenaSettings& settings;
  Bitmap src_bitmap;

  GenerateType type = GenerateType::Ball;

  ShipGenerator(Bitmap src_bitmap, const ArenaSettings& settings, GenerateType type)
      : src_bitmap(src_bitmap), settings(settings), type(type) {}

  bool Generate(int ship);
};

}  // namespace shox
