#include "ppu.hpp"
#include "mmu.hpp"
#include "util.hpp"
#include <algorithm>
#include <cassert>
namespace VTxx {
static uint8_t ppu_regs[256];
static uint8_t vram[4096];
static uint8_t spram[2048];

// Graphics layers
// These use a *very* unusual format to match - at least as close as possible -
// how the VT168 works. It consists of two 16-bit words, the MSW for palette
// bank 1 and the LSW for palette bank 0. Each word is in TRGB1555 format,
// where the MSb is 1 for transparent and 0 for solid
static uint32_t *layers[4];
static int layer_width, layer_height;

static uint32_t *obuf;
static int out_width, out_height;

void ppu_init() {
  layer_width = 256;
  layer_height = 256;

  for (int i = 0; i < 4; i++) {
    layers[i] = new uint32_t[layer_width * layer_height];
  }
  out_width = 256;
  out_height = 240;
  obuf = new uint32_t[out_width * out_height];
}

enum class ColourMode { IDX_16, IDX_64, IDX_256, ARGB1555 };

// Our custom (slow) blitting function
static void vt_blit(int src_width, int src_height, uint8_t *src, int dst_width,
                    int dst_height, int dst_stride, int dst_x, int dst_y,
                    uint32_t *dst, ColourMode fmt, uint8_t *pal0 = nullptr,
                    uint8_t *pal1 = nullptr) {
  uint8_t *srcptr = src;
  int src_bit = 0;
  for (int sy = 0; sy < src_height; sy++) {
    int dy = dst_y + sy;
    for (int sx = 0; sx < src_width; sx++) {
      int dx = dst_x + sx;
      uint16_t argb0, argb1;
      if (fmt == ColourMode::ARGB1555) {
        argb0 = (*(srcptr + 1) << 8UL) | (*srcptr);
        argb1 = argb0;
        srcptr += 2;
      } else {
        uint8_t raw = 0;
        if (fmt == ColourMode::IDX_16) {
          raw = ((*srcptr) >> src_bit) & 0x0F;
          src_bit += 4;
          if (src_bit >= 8) {
            src_bit = 0;
            srcptr++;
          }
        } else if (fmt == ColourMode::IDX_64) {
          switch (src_bit) {
          case 0:
            raw = (*srcptr) & 0x3F;
            src_bit = 6;
            break;
          case 2:
            raw = ((*srcptr) >> 2) & 0x3F;
            src_bit = 0;
            srcptr++;
            break;
          case 4:
            raw = (((*srcptr) & 0xF0) >> 4) | ((*(srcptr + 1) & 0x03) << 4);
            src_bit = 2;
            srcptr++;
            break;
          case 6:
            raw = (((*srcptr) & 0xC0) >> 6) | ((*(srcptr + 1) & 0x0F) << 2);
            src_bit = 4;
            srcptr++;
            break;
          default:
            assert(false);
          }
        } else if (fmt == ColourMode::IDX_256) {
          raw = *srcptr;
          srcptr++;
        } else {
          assert(false);
        }
        if (raw == 0) {
          argb0 = 0x8000; // idx 0 is always transparent
          argb1 = 0x8000; // idx 0 is always transparent
        } else {
          if (pal0 != nullptr)
            argb0 = (pal0[2 * raw + 1] << 8) | pal0[2 * raw];
          if (pal1 != nullptr)
            argb1 = (pal1[2 * raw + 1] << 8) | pal1[2 * raw];
        }
      }
      if ((dx >= 0) && (dx < dst_width) && (dy >= 0) && (dy < dst_height)) {
        if (!(argb0 & 0x8000)) {
          dst[dy * dst_stride + dx] =
              (dst[dy * dst_stride + dx] & 0xFFFF0000) | argb0;
        }
        if (!(argb1 & 0x8000)) {
          dst[dy * dst_stride + dx] =
              (dst[dy * dst_stride + dx] & 0x0000FFFF) | (argb1 << 16UL);
        }
      }
    }
  }
};

void clear_layer(uint16_t *ptr, int w, int h) {
  fill(ptr, ptr + (w * h), 0x8000); // fill with transparent
}

const int reg_sp_seg_lsb = 0x1A;
const int reg_sp_seg_msb = 0x1B;
const int reg_sp_ctrl = 0x18;

// Get character data from ROM for an item
static void get_char_data(uint16_t seg, uint16_t vector, int w, int h,
                          ColourMode fmt, bool bmp, uint8_t *buf) {
  int spacing = 0;
  if (bmp || fmt == ColourMode::ARGB1555) {
    spacing = 16 * 16;
  } else {
    spacing = w * h;
  }
  int bpp = 0;
  switch (fmt) {
  case ColourMode::ARGB1555:
    bpp = 16;
    break;
  case ColourMode::IDX_256:
    bpp = 8;
    break;
  case ColourMode::IDX_64:
    bpp = 6;
    break;
  case ColourMode::IDX_16:
    bpp = 4;
    break;
  }
  if (bpp == 16)
    spacing *= 8;
  else
    spacing *= bpp;
  spacing /= 8;
  uint32_t pa = (seg << 13UL) + vector * spacing;
  int len = (w * h * bpp) / 8;
  for (int i = 0; i < len; i++)
    buf[i] = read_mem_physical(pa + i);
}

static void render_sprites(int pal_bank) {
  // TODO: lots of rendering fixes, e.g. multi palette blending, sprite per line
  // limit, "dig"
  bool sp_en = get_bit(ppu_regs[reg_sp_ctrl], 2);
  if (!sp_en)
    return;
  bool spalsel = get_bit(ppu_regs[reg_sp_ctrl], 3);
  int sp_size = ppu_regs[reg_sp_ctrl] & 0x03;
  int sp_width = (sp_size == 2 || sp_size == 3) ? 16 : 8;
  int sp_height = (sp_size == 1 || sp_size == 3) ? 16 : 8;
  uint16_t sp_seg =
      (ppu_regs[reg_sp_seg_msb] & 0x0F) << 8 | ppu_regs[reg_sp_seg_lsb];

  uint8_t tempbuf[16 * 16];

  for (int idx = 0; idx < 240; idx++) {
    uint8_t *spdata = spram + 8 * idx;
    uint16_t vector = ((spdata[1] & 0x0F) << 8UL) | spdata[0];
    if (vector == 0)
      continue;
    int layer = (spdata[3] >> 3) & 0x03;
    int palette = (spdata[1] >> 4) & 0x0F;
    bool psel = get_bit(spdata[5], 1);
    int x = spdata[2];
    if (get_bit(spdata[3], 0))
      x = x - 256;
    int y = spdata[4];
    if (get_bit(spdata[5], 0))
      y = y - 256;
    get_char_data(sp_seg, vector, sp_width, sp_height, ColourMode::IDX_16,
                  false, tempbuf);
    uint8_t *pal0 = nullptr, *pal1 = nullptr;
    if (spalsel || !psel)
      pal0 = (vram + 0x1E00 + 32 * palette);
    if (spalsel || psel)
      pal1 = (vram + 0x1C00 + 32 * palette);
    vt_blit(sp_width, sp_height, tempbuf, layer_width, layer_height,
            layer_width, x, y, layers[layer], ColourMode::IDX_16, pal0, pal1);
  }
}

} // namespace VTxx
