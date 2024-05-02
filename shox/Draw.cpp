#include "Draw.h"

#include <shox/Platform.h>

namespace shox {

void CopyBlock(Bitmap* src, int src_start_x, int src_start_y, Bitmap* dest, int dest_start_x, int dest_start_y,
               int width, int height) {
  if (src_start_x < 0 || src_start_y < 0 || dest_start_x < 0 || dest_start_y < 0 || src_start_x + width > src->width ||
      src_start_y + height > src->height || dest_start_x + width > dest->width ||
      dest_start_y + height > dest->height) {
    DisplayError("CopyBlock was called out of bounds.");
    exit(1);
  }

  for (int y = src_start_y; y < src_start_y + height; ++y) {
    u8* src_ptr = src->data + (y * src->width + src_start_x) * 4;
    u8* dest_ptr = dest->data + ((dest_start_y + (y - src_start_y)) * dest->width + dest_start_x) * 4;

    for (int x = src_start_x; x < src_start_x + width; ++x) {
      *(dest_ptr + 0) = *(src_ptr + 0);
      *(dest_ptr + 1) = *(src_ptr + 1);
      *(dest_ptr + 2) = *(src_ptr + 2);
      *(dest_ptr + 3) = *(src_ptr + 3);

      src_ptr += 4;
      dest_ptr += 4;
    }
  }
}

// Draws a rectangle with inclusive bounds. Color = RGBA
void DrawRect(Bitmap* dest, int start_x, int start_y, int end_x, int end_y, u32 color) {
  if (start_x < 0 || start_y < 0 || end_x >= dest->width || end_y >= dest->height) {
    DisplayError("DrawRect was called out of bounds.");
    exit(1);
  }

  u8* dest_ptr = dest->data + (start_y * dest->width + start_x) * 4;

  for (int y = start_y; y <= end_y; ++y) {
    u8* dest_ptr_left = dest->data + (y * dest->width + start_x) * 4;
    u8* dest_ptr_right = dest->data + (y * dest->width + end_x) * 4;

    *(u32*)dest_ptr_left = color;
    *(u32*)dest_ptr_right = color;
  }

  for (int x = start_x; x <= end_x; ++x) {
    u8* dest_ptr_top = dest->data + (start_y * dest->width + x) * 4;
    u8* dest_ptr_bottom = dest->data + (end_y * dest->width + x) * 4;

    *(u32*)dest_ptr_top = color;
    *(u32*)dest_ptr_bottom = color;
  }
}

}  // namespace shox
