#include "Generator.h"

#include <shox/ArenaSettings.h>
#include <shox/Draw.h>
#include <shox/Image.h>
#include <shox/Platform.h>

namespace shox {

static uint16_t GetCombinedRadius(const ShipGenerator& gen, int ship, int level) {
  const auto& settings = gen.settings;

  uint16_t radius = settings.ShipSettings[ship].Radius;

  if (level <= 0) level = 1;

  // 14 is the default ship radius when nothing is specified.
  if (radius == 0) radius = 14;

  switch (gen.type) {
    case GenerateType::Bomb: {
      radius += (settings.ProximityDistance + (level - 1)) * 16;
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

  int original_ship_dim = src_bitmap.width / 10;
  u16 new_ship_radius = GetCombinedRadius(*this, ship, 3) + 2;

  // Make radius even
  if (new_ship_radius & 1) ++new_ship_radius;
  u16 new_ship_dim = (new_ship_radius * 2);

  int total_width = new_ship_dim * 10;
  int total_height = new_ship_dim * 4;
  size_t total_size = total_width * total_height * 4;

  u8* write_data = (u8*)malloc(total_size);
  if (!write_data) {
    shox::DisplayError("Failed to allocate memory for new ships bitmap.");
    return false;
  }

  u8* write_current = write_data;
  u8* write_end = write_data + total_size;

  while (write_current < write_end) {
    *(u32*)write_current = 0xFF000000;
    write_current += 4;
  }

  int empty_growth = new_ship_radius - (original_ship_dim / 2);
  if (empty_growth < 0) empty_growth = 0;

  if (empty_growth & 1) ++empty_growth;

  Bitmap dest_bitmap(write_data, total_width, total_height);

  for (int rotation = 0; rotation < 40; ++rotation) {
    int src_start_x = (rotation % 10) * original_ship_dim;
    int src_start_y = (ship * 4 + rotation / 10) * original_ship_dim;
    int dest_start_x = (rotation % 10) * new_ship_dim + empty_growth;
    int dest_start_y = (rotation / 10) * new_ship_dim + empty_growth;

    // Perform copy from old ship data to new data
    CopyBlock(&src_bitmap, src_start_x, src_start_y, &dest_bitmap, dest_start_x, dest_start_y, original_ship_dim,
              original_ship_dim);

    int center_x = dest_start_x + original_ship_dim / 2;
    int center_y = dest_start_y + original_ship_dim / 2;

    switch (type) {
      case GenerateType::Bomb: {
        u32 colors[] = {
            RGBA(255, 0, 0, 255),
            RGBA(255, 255, 0, 255),
            RGBA(0, 0, 255, 255),
        };

        for (int i = 0; i < SHOX_ARRAY_SIZE(colors); ++i) {
          uint16_t radius = GetCombinedRadius(*this, ship, i + 1);

          if (radius > 0) {
            DrawRect(&dest_bitmap, center_x - radius, center_y - radius, center_x + radius, center_y + radius,
                     colors[i]);
          }
        }
      } break;
      case GenerateType::Ball: {
        uint16_t radius = GetCombinedRadius(*this, ship, 1);

        if (radius > 0) {
          DrawRect(&dest_bitmap, center_x - radius, center_y - radius, center_x + radius, center_y + radius,
                   RGBA(255, 255, 255, 255));
        }
      } break;
      default: {
      } break;
    }
  }

  stbi_write_png_compression_level = 16;

  char output_filename[1024];
  sprintf(output_filename, "ship%d.png", ship + 1);
  stbi_write_png(output_filename, total_width, total_height, 4, write_data, total_width * 4);

  free(write_data);

  return true;
}

}  // namespace shox
