#pragma once

#include <shox/Types.h>

namespace shox {

struct Bitmap {
  u8* data;
  int width;
  int height;

  inline Bitmap(u8* data, int width, int height) : data(data), width(width), height(height) {}
  inline Bitmap(const Bitmap& other) : data(other.data), width(other.width), height(other.height) {}
};

void CopyBlock(Bitmap* src, int src_start_x, int src_start_y, Bitmap* dest, int dest_start_x, int dest_start_y,
               int width, int height);

#define RGBA(r, g, b, a) (u32)((r) | ((g) << 8) | ((b) << 16) | ((a) << 24))
// Draws a rectangle with inclusive bounds. Color = RGBA
void DrawRect(Bitmap* dest, int start_x, int start_y, int end_x, int end_y, u32 color);

}  // namespace shox
