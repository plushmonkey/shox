#pragma once

#include <stb_image.h>
#include <stb_image_write.h>

namespace shox {

unsigned char* ImageLoadFromMemory(const unsigned char* data, size_t size, int* width, int* height);

}  // namespace shox
