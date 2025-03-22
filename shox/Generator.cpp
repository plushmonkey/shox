#include "Generator.h"

#include <shox/ArenaSettings.h>
#include <shox/Draw.h>
#include <shox/Image.h>
#include <shox/Platform.h>

namespace shox {

static uint16_t GetShipRadius(const ShipGenerator& gen, int ship, int level) {
  const auto& settings = gen.settings;

  uint16_t radius = settings.ShipSettings[ship].Radius;

  if (level <= 0) level = 1;

  // 14 is the default ship radius when nothing is specified.
  if (radius == 0) radius = 14;

  switch (gen.type) {
    case GenerateType::Mine:
    case GenerateType::Bomb: {
      constexpr s16 kBombRadius = 7;

      radius = (settings.ProximityDistance + (level - 1)) * 16 - kBombRadius;
    } break;
    case GenerateType::Ball: {
      // Shrink by the ball radius so the bounding box will be created for edge-overlap test.
      constexpr s16 kBallRadius = 7;

      // Overwrite the radius since the ship's radius is not included in soccer ball proximity.
      if (settings.ShipSettings[ship].SoccerBallProximity > kBallRadius) {
        radius = settings.ShipSettings[ship].SoccerBallProximity - kBallRadius;
      }
    } break;
    default: {
    } break;
  }

  return radius;
}

bool ShipGenerator::Generate(int ship) {
  if (ship < 0 || ship > 7) return false;

  int original_item_dim = src_bitmap.width / 10;
  u16 new_ship_radius = GetShipRadius(*this, ship, 4) + 2;

  // Make radius even
  if (new_ship_radius & 1) ++new_ship_radius;
  u16 new_item_dim = (new_ship_radius * 2);

  int rows = 4;
  int cols = 10;

  if (type == GenerateType::Bomb) {
    rows = 13;
  } else if (type == GenerateType::Mine) {
    rows = 8;
  }

  int total_width = new_item_dim * cols;
  int total_height = new_item_dim * rows;
  size_t total_size = (size_t)total_width * (size_t)total_height * sizeof(u32);

  u8* write_data = (u8*)malloc(total_size);
  if (!write_data) {
    shox::DisplayError("Failed to allocate memory for output bitmap.");
    return false;
  }

  u8* write_current = write_data;
  u8* write_end = write_data + total_size;

  while (write_current < write_end) {
    *(u32*)write_current = 0xFF000000;
    write_current += 4;
  }

  int empty_growth = new_ship_radius - (original_item_dim / 2);
  if (empty_growth < 0) empty_growth = 0;

  if (empty_growth & 1) ++empty_growth;

  Bitmap dest_bitmap(write_data, total_width, total_height);

  if (type == GenerateType::Ball) {
    for (int rotation = 0; rotation < 40; ++rotation) {
      int src_start_x = (rotation % 10) * original_item_dim;
      int src_start_y = (ship * 4 + rotation / 10) * original_item_dim;
      int dest_start_x = (rotation % 10) * new_item_dim + empty_growth;
      int dest_start_y = (rotation / 10) * new_item_dim + empty_growth;

      // Perform copy from old ship data to new data
      CopyBlock(&src_bitmap, src_start_x, src_start_y, &dest_bitmap, dest_start_x, dest_start_y, original_item_dim,
                original_item_dim);

      int center_x = dest_start_x + original_item_dim / 2;
      int center_y = dest_start_y + original_item_dim / 2;

      uint16_t radius = GetShipRadius(*this, ship, 1);
      u32 color = RGBA(255, 255, 255, 255);

      if (radius > 0) {
        DrawRect(&dest_bitmap, center_x - radius, center_y - radius, center_x + radius, center_y + radius, color);
      }
    }
  } else {
    for (int bomb_row = 0; bomb_row < rows; ++bomb_row) {
      for (int rotation = 0; rotation < cols; ++rotation) {
        int src_start_x = rotation * original_item_dim;
        int src_start_y = bomb_row * original_item_dim;
        int dest_start_x = rotation * new_item_dim + empty_growth;
        int dest_start_y = bomb_row * new_item_dim + empty_growth;

        // Perform copy from old item data to new data
        CopyBlock(&src_bitmap, src_start_x, src_start_y, &dest_bitmap, dest_start_x, dest_start_y, original_item_dim,
                  original_item_dim);

        int center_x = dest_start_x + original_item_dim / 2;
        int center_y = dest_start_y + original_item_dim / 2;

        int level = (bomb_row % 4) + 1;
        if (bomb_row == 12) {
          level = 4;
        }

        u32 colors[] = {
            RGBA(255, 0, 0, 255),
            RGBA(255, 255, 0, 255),
            RGBA(0, 0, 255, 255),
            RGBA(255, 0, 255, 255),
        };
        u32 color = colors[level - 1];

        uint16_t radius = GetShipRadius(*this, ship, level);

        if (radius > 0) {
          DrawRect(&dest_bitmap, center_x - radius, center_y - radius, center_x + radius, center_y + radius, color);
        }
      }
    }
  }

  stbi_write_png_compression_level = 16;

  char output_filename[2048];

  size_t output_filename_offset = 0;

  if (!working_directory.empty()) {
    output_filename_offset += sprintf(output_filename, "%s\\", working_directory.data());
  }

  switch (type) {
    case GenerateType::Ball: {
      sprintf(output_filename + output_filename_offset, "ship%d.png", ship + 1);
    } break;
    case GenerateType::Bomb: {
      sprintf(output_filename + output_filename_offset, "bombs.png");
    } break;
    case GenerateType::Mine: {
      sprintf(output_filename + output_filename_offset, "mines.png");
    } break;
    default: {
      return false;
    } break;
  }

  stbi_write_png(output_filename, total_width, total_height, 4, write_data, total_width * 4);

  free(write_data);

  return true;
}

bool ShipGenerator::Generate() {
  switch (type) {
    case GenerateType::Ball: {
      for (int i = 0; i < 8; ++i) {
        if (!Generate(i)) {
          shox::DisplayError("Failed to generate ship.");
          return false;
        }
      }
    } break;
    case GenerateType::Mine:
    case GenerateType::Bomb: {
      Generate(0);
    } break;
    default: {
      shox::DisplayError("Illegal generate type.");
      return false;
    } break;
  }

  return true;
}

}  // namespace shox
