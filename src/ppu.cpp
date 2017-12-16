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

enum class ColourMode { IDX_4, IDX_16, IDX_64, IDX_256, ARGB1555 };

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
        if (fmt == ColourMode::IDX_4) {
          raw = ((*srcptr) >> src_bit) & 0x03;
          src_bit += 2;
          if (src_bit >= 8) {
            src_bit = 0;
            srcptr++;
          }
        } else if (fmt == ColourMode::IDX_16) {
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
  case ColourMode::IDX_4:
    bpp = 2;
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

static void render_sprites() {
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

const int reg_bkg_x[2] = {0x10, 0x14};
const int reg_bkg_y[2] = {0x11, 0x15};
const int reg_bkg_ctrl1[2] = {0x12, 0x16};

// const int reg_bkg_linescroll = 0x20;
const int reg_bkg_ctrl2[2] = {0x13, 0x17};

const int reg_bkg_pal_sel = 0x0F;

const int reg_bkg_seg_lsb[2] = {0x1C, 0x1E};
const int reg_bkg_seg_msb[2] = {0x1D, 0x1F};

enum BkgScrollMode {
  SCROLL_FIX = 0,
  SCROLL_H = 1,
  SCROLL_V = 2,
  SCROLL_4P = 3
};

// Return the address of a tile given index, tile size and scroll mode
// and whether or not a tile actually exists
// This needs checking as the datasheet is fairly poor for this
static pair<uint16_t, bool> get_tile_addr(int tx, int ty, bool y8, bool x8,
                                          int size, bool bmp, int layer,
                                          BkgScrollMode scrl) {
  if (size == 8) {
    uint16_t base = 0;
    uint16_t offset = ((tx % 32) + 32 * (ty % 32)) * 2;
    bool mapped = false;
    switch (scrl) {
    case SCROLL_FIX:
      base = (y8 == 0 && x8 == 0) ? 0x000 : 0x800;
      mapped = (tx < 32 && ty < 32);
      break;
    case SCROLL_H:
      base = ((tx > 32) != x8) ? 0x800 : 0x000;
      mapped = ty < 32;
      break;
    case SCROLL_V:
      base = ((ty > 32) != y8) ? 0x800 : 0x000;
      mapped = tx < 32;
      break;
    case SCROLL_4P:
      assert(false);
      break;
    }
    return make_pair(mapped, base + offset);
  } else if (size == 16) {
    uint16_t base = 0;
    uint16_t offset = ((tx % 16) + 16 * (ty % 16)) * 2;
    bool mapped = false;
    switch (scrl) {
    case SCROLL_FIX:
      base = (layer << 11) | (y8 << 10) | (x8 << 9);
      mapped = (tx < 16 && ty < 16);
      break;
    case SCROLL_H:
      base = ((tx > 16) != x8) ? 0x200 : 0x000;
      base |= (layer << 11);
      mapped = ty < 16;
      break;
    case SCROLL_V:
      base = ((ty > 16) != y8) ? 0x200 : 0x000;
      base |= (layer << 11);
      mapped = tx < 16;
      break;
    case SCROLL_4P:
      base = ((tx > 16) != x8) ? 0x200 : 0x000;
      base |= ((ty > 16) != y8) ? 0x400 : 0x000;
      base |= (layer << 11);
      mapped = true;
      break;
    }
    return make_pair(mapped, base + offset);
  } else if (bmp) {
    assert(layer == 0);
    uint16_t base = 0;
    uint16_t offset = (ty % 256) * 2;
    bool mapped = false;
    switch (scrl) {
    case SCROLL_FIX:
      base = (layer << 11) | (y8 << 10) | (x8 << 9);
      mapped = (tx < 1 && ty < 256);
      break;
    case SCROLL_H:
      base = ((tx > 1) != x8) ? 0x200 : 0x000;
      mapped = ty < 256;
      break;
    case SCROLL_V:
      base = ((ty > 256) != y8) ? 0x200 : 0x000;
      mapped = tx < 1;
      break;
    case SCROLL_4P:
      base = ((tx > 1) != x8) ? 0x200 : 0x000;
      base |= ((ty > 256) != y8) ? 0x400 : 0x000;
      mapped = true;
      break;
    }
    return make_pair(mapped, base + offset);
  } else {
    assert(false);
  }
}

// Render the given background layer (idx = [0, 1])
static void render_background(int idx) {
  bool en = get_bit(ppu_regs[reg_bkg_ctrl2[idx]], 7);
  if (!en)
    return;
  bool bkx_pal = get_bit(ppu_regs[reg_bkg_ctrl2[idx]], 6);
  ColourMode fmt;
  bool hclr = (idx == 0) ? get_bit(ppu_regs[reg_bkg_ctrl1[idx]], 4) : false;
  int bkx_clr = (ppu_regs[reg_bkg_ctrl2[idx]] >> 2) & 0x03;
  if (hclr) {
    fmt = ColourMode::ARGB1555;
  } else {
    switch (bkx_clr) { // check, datasheet doesn't specify
    case 0:
      fmt = ColourMode::IDX_4;
      break;
    case 1:
      fmt = ColourMode::IDX_16;
      break;
    case 2:
      fmt = ColourMode::IDX_64;
      break;
    case 3:
      fmt = ColourMode::IDX_256;
      break;
    }
  }
  bool x8 = get_bit(ppu_regs[reg_bkg_ctrl1[idx]], 0);
  bool y8 = get_bit(ppu_regs[reg_bkg_ctrl1[idx]], 1);
  int xoff = ppu_regs[reg_bkg_x[idx]];
  if (x8)
    xoff = xoff - 256;
  int yoff = ppu_regs[reg_bkg_y[idx]];
  if (y8)
    yoff = yoff - 256;
  bool bmp = (idx == 0) ? get_bit(ppu_regs[reg_bkg_ctrl2[idx]], 1) : false;
  BkgScrollMode scrl_mode =
      (BkgScrollMode)((ppu_regs[reg_bkg_ctrl1[idx]] >> 2) & 0x03);
  // bool line_scroll = get_bit(ppu_regs[reg_bkg_linescroll], 4 + idx);
  // int line_scroll_bank = ppu_regs[reg_bkg_linescroll] & 0x0F;
  bool bkx_size = get_bit(ppu_regs[reg_bkg_ctrl2[idx]], 0);
  int tile_height = bmp ? 1 : (bkx_size ? 16 : 8);
  int tile_width = bmp ? 256 : (bkx_size ? 16 : 8);
  int y0 =
      ((scrl_mode == SCROLL_V || scrl_mode == SCROLL_4P) && !bmp) ? -256 : 0;
  int x0 =
      ((scrl_mode == SCROLL_H || scrl_mode == SCROLL_4P) && !bmp) ? -256 : 0;
  int yn = 256;
  int xn = 256;
  uint8_t char_buf[512];

  uint16_t seg = ((ppu_regs[reg_bkg_seg_msb[idx]] & 0x0F) << 8UL) |
                 ppu_regs[reg_bkg_seg_lsb[idx]];

  for (int y = y0; y < yn; y += tile_height) {
    for (int x = x0; x < xn; x += tile_width) {
      int lx = x + xoff;
      int ly = y + yoff;
      int tx = (x - x0) / tile_width;
      int ty = (y - y0) / tile_height;
      // Various inefficiencies here, should not draw unless at least part
      // visible
      auto tile_d =
          get_tile_addr(tx, ty, y8, x8, tile_width, bmp, idx, scrl_mode);
      uint16_t tile_addr = tile_d.first;
      bool tile_mapped = tile_d.second;
      if (!tile_mapped)
        continue;
      uint16_t cell = (vram[tile_addr + 1] << 8UL) | vram[tile_addr];
      uint16_t vector = cell & 0xFFF;
      uint8_t cell_pal_bk = (cell >> 12) & 0x0F;
      if (vector == 0) // transparent
        continue;
      uint8_t pal_bank = 0;
      uint8_t depth = 0;
      if (bkx_pal) {
        depth = (ppu_regs[reg_bkg_ctrl2[idx]] >> 4) & 0x03;
        pal_bank = (fmt == ColourMode::IDX_16)
                       ? cell_pal_bk
                       : ((fmt == ColourMode::IDX_64) ? (cell_pal_bk >> 2) : 0);
      } else {
        depth = cell_pal_bk & 0x03;
        pal_bank = (fmt == ColourMode::IDX_16)
                       ? (((ppu_regs[reg_bkg_ctrl2[idx]] >> 4) & 0x03) |
                          (cell_pal_bk >> 2))
                       : ((fmt == ColourMode::IDX_64) ? (cell_pal_bk >> 2) : 0);
      }
      get_char_data(seg, vector, tile_width, tile_height, fmt, bmp, char_buf);
      // TODO: line scrolling
      uint16_t palette_offset =
          (fmt == ColourMode::IDX_16)
              ? (pal_bank * 32)
              : (fmt == ColourMode::IDX_64 ? (pal_bank * 128) : 0);
      uint8_t *pal0, *pal1;
      pal0 = (vram + 0x1E00 + palette_offset);
      pal1 = (vram + 0x1C00 + palette_offset);
      vt_blit(tile_width, tile_height, char_buf, layer_width, layer_height,
              layer_width, lx, ly, layers[depth & 0x03], fmt, pal0, pal1);
    }
  }
}

} // namespace VTxx
