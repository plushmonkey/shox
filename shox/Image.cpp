#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "Image.h"

#include <shox/Types.h>

namespace shox {

#pragma pack(push, 1)
struct BitmapFileHeader {
  unsigned char signature[2];
  u32 size;
  u16 reserved1;
  u16 reserved2;
  u32 offset;
};

struct DIBHeader {
  u32 header_size;
  s32 width;
  s32 height;
  u16 planes;
  u16 bpp;
  u32 compression;
  u32 image_size;
  u32 x_ppm;
  u32 y_ppm;
  u32 color_table_count;
};
#pragma pack(pop)

static constexpr unsigned int kDefaultColorTable[256] = {
    0x000000, 0x393939, 0x424242, 0x525252, 0x5a5a5a, 0x737373, 0x7b7b7b, 0x848484, 0x8c8c8c, 0x949494, 0x9c9c9c,
    0xa5a5a5, 0xadadad, 0xc6c6c6, 0xcecece, 0xbdb5b5, 0xb5adad, 0x9c9494, 0x7b7373, 0x6b6363, 0xffdede, 0xefc6c6,
    0x8c7373, 0xffcece, 0xefb5b5, 0x634a4a, 0xb58484, 0xffb5b5, 0xe78484, 0x7b4242, 0x211010, 0xff7b7b, 0x842929,
    0x521818, 0xce0808, 0xce2921, 0xf76352, 0xce5a4a, 0x94635a, 0xa54231, 0xde3108, 0xff3908, 0x731800, 0x4d2d04,
    0xa52900, 0x7b635a, 0xff8452, 0xce6339, 0xbd4208, 0x632100, 0xffb584, 0xef6b18, 0x733910, 0xde8439, 0x635a52,
    0x948c84, 0xce8c4a, 0x845a29, 0xffa542, 0xf78c18, 0xffd6a5, 0xad8c63, 0x845208, 0xad7310, 0xa56b08, 0xffe7bd,
    0xb57b10, 0x946308, 0xf7dead, 0x9c8c6b, 0xd6b573, 0xa58442, 0xf7b529, 0xefad21, 0x736b5a, 0x4a4231, 0xefce84,
    0xffce63, 0xdead42, 0xffbd29, 0xb59442, 0xefbd42, 0xffc631, 0x5a4208, 0xce9408, 0xffde84, 0xe7b521, 0xfff7de,
    0xefe7ce, 0xb5ad94, 0xb5a573, 0xffe79c, 0xffd65a, 0xa59c7b, 0x847b5a, 0xfff7d6, 0xffefad, 0x9c8418, 0xe7c629,
    0xad9c21, 0xffe721, 0xadada5, 0x9c9c8c, 0x4a4a42, 0xadad94, 0x313129, 0xa5a584, 0x5a5a18, 0xcece31, 0xffff39,
    0x7b8421, 0xd6e739, 0xa5b531, 0xadc639, 0x738439, 0x94b542, 0xb5ce7b, 0x739431, 0x526b29, 0xbdf763, 0x7ba542,
    0xa5de52, 0x7bb542, 0xe7f7de, 0x7b8c73, 0xa5bd9c, 0x9cf77b, 0xc6e7bd, 0x7bb56b, 0x529442, 0x398429, 0x183910,
    0x6bde5a, 0x295a21, 0x73ff63, 0x5ace4a, 0x429439, 0x6bce63, 0x4aad42, 0xadb5ad, 0xb5ceb5, 0x394239, 0x213121,
    0xa5f7a5, 0x397339, 0x215221, 0x296b29, 0x399439, 0x185218, 0x084a31, 0x104239, 0x313952, 0x31398c, 0x6b73ce,
    0x4a52b5, 0xefefff, 0xe7e7ff, 0xbdbdd6, 0x6b6b7b, 0xcecef7, 0xc6c6f7, 0x42425a, 0xa5a5e7, 0xb5b5ff, 0x4a4a6b,
    0x9494d6, 0x9c9ce7, 0x7373ad, 0x6b6ba5, 0xa5a5ff, 0x8484ce, 0x63639c, 0x7373b5, 0x525284, 0x7b7bc6, 0x42426b,
    0x5a5a94, 0x4a4a7b, 0x6363a5, 0x393963, 0x29294a, 0x42427b, 0x313163, 0x393973, 0x6363ce, 0x6363ff, 0x5252d6,
    0x4242bd, 0x212163, 0x5252ff, 0x3939b5, 0x4242de, 0x212173, 0x31297b, 0x635a94, 0x8c84ad, 0x635a8c, 0x4a29a5,
    0x4210c6, 0x290094, 0x4a2994, 0xb59ce7, 0x211831, 0x63429c, 0x3f2d0e, 0x210052, 0x31007b, 0x8463ad, 0x8442d6,
    0x6321b5, 0x390084, 0xb584ef, 0xad6bf7, 0x7342ad, 0x844ac6, 0x52297b, 0x4a2173, 0x310063, 0x4a0094, 0x5200a5,
    0x4a395a, 0x634a7b, 0xce94ff, 0x9c6bc6, 0x7b42ad, 0x9c52de, 0x4a0884, 0x210039, 0xad63c6, 0x4a0063, 0x9c63a5,
    0x630073, 0x7b737b, 0x0a190a, 0x4a004a, 0x520052, 0x7b0073, 0x7b4a73, 0x4a3142, 0x632942, 0x846b73, 0x94737b,
    0xff7394, 0x633942, 0x946b73, 0x9c394a, 0xff8c9c, 0xbd6b73, 0xef6373, 0xce2939, 0xff394a, 0x7b1821, 0xf7737b,
    0xd63942, 0xa52129, 0xffffff,
};

inline u32 GetColor(u32* color_table, u32 index) {
  if (color_table == nullptr) {
    return kDefaultColorTable[index];
  }

  return color_table[index];
}

static unsigned char* LoadBitmap(const u8* data, size_t file_size, int* width, int* height) {
  BitmapFileHeader* file_header = (BitmapFileHeader*)data;
  DIBHeader* dib_header = (DIBHeader*)(data + sizeof(BitmapFileHeader));

  // Only support 8 bit RLE for now
  if (dib_header->compression != 1) {
    return nullptr;
  }

  assert(dib_header->bpp == 8);
  if (dib_header->bpp != 8) return nullptr;

  *width = abs(dib_header->width);
  *height = abs(dib_header->height);

  // Seek to color table
  const u8* ptr = data + sizeof(BitmapFileHeader) + dib_header->header_size;

  u32* color_table = nullptr;

  if (dib_header->color_table_count > 0) {
    color_table = (u32*)ptr;
  }

  // Seek to image data
  ptr = data + file_header->offset;

  u32* result = nullptr;

  if (dib_header->bpp == 8) {
    size_t image_size = dib_header->width * dib_header->height;
    const u8* image_data = ptr;

    // Expand out the data to rgba
    result = (u32*)malloc(image_size * sizeof(u32));
    if (!result) return nullptr;

    memset(result, 0, image_size * sizeof(u32));
    size_t i = 0;
    int x = 0;
    int y = dib_header->height - 1;

    while (i < image_size) {
      u8 count = image_data[i++];
      u8 color_index = image_data[i++];

      if (count == 0) {
        if (color_index == 0) {
          // End of line
          --y;
          x = 0;
        } else if (color_index == 1) {
          // End of bitmap
          break;
        } else if (color_index == 2) {
          // Delta
          int next_x = image_data[i++];
          int next_y = image_data[i++];

          x += next_x;
          y -= next_y;

          assert(x >= 0 && x < dib_header->width);
          assert(y >= 0 && y < dib_header->height);
        } else {
          // Run of absolute values
          for (int j = 0; j < color_index; ++j) {
            u8 absolute_index = image_data[i++];

            u32 color = GetColor(color_table, absolute_index) | 0xFF000000;
            color = ((color & 0xFF) << 16) | ((color & 0x00FF0000) >> 16) | (color & 0xFF000000) | (color & 0x0000FF00);
            result[y * dib_header->width + x] = color;
            ++x;
          }

          if (i & 1) {
            ++i;
          }
        }
      } else {
        u32 color = GetColor(color_table, color_index) | 0xFF000000;
        color = ((color & 0xFF) << 16) | ((color & 0x00FF0000) >> 16) | (color & 0xFF000000) | (color & 0x0000FF00);

        for (int j = 0; j < count; ++j) {
          size_t index = (size_t)y * (size_t)dib_header->width + (size_t)x;
          result[index] = color;
          ++x;
        }
      }
    }
  }

  return (u8*)result;
}

unsigned char* ImageLoadFromMemory(const unsigned char* data, size_t size, int* width, int* height) {
  int comp;
  unsigned char* result = stbi_load_from_memory(data, (int)size, width, height, &comp, STBI_rgb_alpha);

  if (result == nullptr) {
    result = LoadBitmap(data, size, width, height);
  }

  return result;
}

}  // namespace shox
