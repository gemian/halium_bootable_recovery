/*
 * Copyright (C) 2007 The Android Open Source Project
 * Copyright (C) 2019 The LineageOS Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "graphics.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <memory>

#include "font_10x18.h"
#include "graphics_adf.h"
#include "graphics_drm.h"
#include "graphics_fbdev.h"
#include "minui/minui.h"

static GRFont* gr_font = NULL;
static GRFont* gr_font_menu = NULL;
static MinuiBackend* gr_backend = nullptr;

static int overscan_percent = OVERSCAN_PERCENT;
static int overscan_offset_x = 0;
static int overscan_offset_y = 0;

static uint32_t gr_current = ~0;
static constexpr uint32_t alpha_mask = 0xff000000;

static GRSurface* gr_draw = NULL;
static GRRotation rotation = ROTATION_NONE;

static bool outside(int x, int y) {
  return x < 0 || x >= (rotation % 2 ? gr_draw->height : gr_draw->width) || y < 0 ||
         y >= (rotation % 2 ? gr_draw->width : gr_draw->height);
}

const GRFont* gr_sys_font() {
  return gr_font;
}

const GRFont* gr_menu_font() {
  return gr_font_menu;
}

int gr_measure(const GRFont* font, const char* s) {
  return font->char_width * strlen(s);
}

void gr_font_size(const GRFont* font, int* x, int* y) {
  *x = font->char_width;
  *y = font->char_height;
}

// Blends gr_current onto pix value, assumes alpha as most significant byte.
static inline uint32_t pixel_blend(uint8_t alpha, uint32_t pix) {
  if (alpha == 255) return gr_current;
  if (alpha == 0) return pix;
  uint32_t pix_r = pix & 0xff;
  uint32_t pix_g = pix & 0xff00;
  uint32_t pix_b = pix & 0xff0000;
  uint32_t cur_r = gr_current & 0xff;
  uint32_t cur_g = gr_current & 0xff00;
  uint32_t cur_b = gr_current & 0xff0000;

  uint32_t out_r = (pix_r * (255 - alpha) + cur_r * alpha) / 255;
  uint32_t out_g = (pix_g * (255 - alpha) + cur_g * alpha) / 255;
  uint32_t out_b = (pix_b * (255 - alpha) + cur_b * alpha) / 255;

  return (out_r & 0xff) | (out_g & 0xff00) | (out_b & 0xff0000) | (gr_current & 0xff000000);
}

// increments pixel pointer right, with current rotation.
static void incr_x(uint32_t** p, int row_pixels) {
  if (rotation % 2) {
    *p = *p + (rotation == 1 ? 1 : -1) * row_pixels;
  } else {
    *p = *p + (rotation ? -1 : 1);
  }
}

// increments pixel pointer down, with current rotation.
static void incr_y(uint32_t** p, int row_pixels) {
  if (rotation % 2) {
    *p = *p + (rotation == 1 ? -1 : 1);
  } else {
    *p = *p + (rotation ? -1 : 1) * row_pixels;
  }
}

// returns pixel pointer at given coordinates with rotation adjustment.
static uint32_t* pixel_at(GRSurface* surf, int x, int y, int row_pixels) {
  switch (rotation) {
    case ROTATION_NONE:
      return reinterpret_cast<uint32_t*>(surf->data) + y * row_pixels + x;
    case ROTATION_RIGHT:
      return reinterpret_cast<uint32_t*>(surf->data) + x * row_pixels + (surf->width - y);
    case ROTATION_DOWN:
      return reinterpret_cast<uint32_t*>(surf->data) + (surf->height - 1 - y) * row_pixels +
             (surf->width - 1 - x);
    case ROTATION_LEFT:
      return reinterpret_cast<uint32_t*>(surf->data) + (surf->height - 1 - x) * row_pixels + y;
    default:
      printf("invalid rotation %d", rotation);
  }
  return nullptr;
}

static void text_blend(uint8_t* src_p, int src_row_bytes, uint32_t* dst_p, int dst_row_pixels,
                       int width, int height) {
  uint8_t alpha_current = static_cast<uint8_t>((alpha_mask & gr_current) >> 24);
  for (int j = 0; j < height; ++j) {
    uint8_t* sx = src_p;
    uint32_t* px = dst_p;
    for (int i = 0; i < width; ++i, incr_x(&px, dst_row_pixels)) {
      uint8_t a = *sx++;
      if (alpha_current < 255) a = (static_cast<uint32_t>(a) * alpha_current) / 255;
      *px = pixel_blend(a, *px);
    }
    src_p += src_row_bytes;
    incr_y(&dst_p, dst_row_pixels);
  }
}

static int rainbow_index = 0;
static int rainbow_enabled = 0;
static int rainbow_colors[] = { 255, 0, 0,        // red
                                255, 127, 0,      // orange
                                255, 255, 0,      // yellow
                                0, 255, 0,        // green
                                60, 80, 255,      // blue
                                143, 0, 255 };    // violet
static int num_rb_colors =
        (sizeof(rainbow_colors)/sizeof(rainbow_colors[0])) / 3;

static void rainbow(int col) {
  int rainbow_color = ((rainbow_index + col) % num_rb_colors) * 3;
  gr_color(rainbow_colors[rainbow_color], rainbow_colors[rainbow_color+1],
              rainbow_colors[rainbow_color+2], 255);
}

void set_rainbow_mode(int enabled) {
  rainbow_enabled = enabled;
}

void move_rainbow(int x) {
  rainbow_index += x;
  if (rainbow_index < 0) {
    rainbow_index = num_rb_colors - 1;
  } else if (rainbow_index >= num_rb_colors) {
    rainbow_index = 0;
  }
}

void gr_text(const GRFont* font, int x, int y, const char* s, bool bold) {
  if (!font || !font->texture || (gr_current & alpha_mask) == 0) return;

  if (font->texture->pixel_bytes != 1) {
    printf("gr_text: font has wrong format\n");
    return;
  }

  bold = bold && (font->texture->height != font->char_height);

  x += overscan_offset_x;
  y += overscan_offset_y;

  unsigned char ch;
  while ((ch = *s++)) {
    if (rainbow_enabled) rainbow(x / font->char_width + (gr_fb_height() - y) / (font->char_height * 3));

    if (outside(x, y) || outside(x + font->char_width - 1, y + font->char_height - 1)) break;

    if (ch < ' ' || ch > '~') {
      ch = '?';
    }

    int row_pixels = gr_draw->row_bytes / gr_draw->pixel_bytes;
    uint8_t* src_p = font->texture->data + ((ch - ' ') * font->char_width) +
                     (bold ? font->char_height * font->texture->row_bytes : 0);
    uint32_t* dst_p = pixel_at(gr_draw, x, y, row_pixels);

    text_blend(src_p, font->texture->row_bytes, dst_p, row_pixels, font->char_width,
               font->char_height);

    x += font->char_width;
  }
}

void gr_texticon(int x, int y, GRSurface* icon) {
  if (icon == NULL) return;

  if (icon->pixel_bytes != 1) {
    printf("gr_texticon: source has wrong format\n");
    return;
  }

  x += overscan_offset_x;
  y += overscan_offset_y;

  if (outside(x, y) || outside(x + icon->width - 1, y + icon->height - 1)) return;

  int row_pixels = gr_draw->row_bytes / gr_draw->pixel_bytes;
  uint8_t* src_p = icon->data;
  uint32_t* dst_p = pixel_at(gr_draw, x, y, row_pixels);

  text_blend(src_p, icon->row_bytes, dst_p, row_pixels, icon->width, icon->height);
}

void gr_color(unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
  uint32_t r32 = r, g32 = g, b32 = b, a32 = a;
#if defined(RECOVERY_ABGR) || defined(RECOVERY_BGRA)
  gr_current = (a32 << 24) | (r32 << 16) | (g32 << 8) | b32;
#else
  gr_current = (a32 << 24) | (b32 << 16) | (g32 << 8) | r32;
#endif
}

void gr_clear() {
  if ((gr_current & 0xff) == ((gr_current >> 8) & 0xff) &&
      (gr_current & 0xff) == ((gr_current >> 16) & 0xff) &&
      (gr_current & 0xff) == ((gr_current >> 24) & 0xff) &&
      gr_draw->row_bytes == gr_draw->width * gr_draw->pixel_bytes) {
    memset(gr_draw->data, gr_current & 0xff, gr_draw->height * gr_draw->row_bytes);
  } else {
    uint32_t* px = reinterpret_cast<uint32_t*>(gr_draw->data);
    int row_diff = gr_draw->row_bytes / gr_draw->pixel_bytes - gr_draw->width;
    for (int y = 0; y < gr_draw->height; ++y) {
      for (int x = 0; x < gr_draw->width; ++x) {
        *px++ = gr_current;
      }
      px += row_diff;
    }
  }
}

void gr_fill(int x1, int y1, int x2, int y2) {
  x1 += overscan_offset_x;
  y1 += overscan_offset_y;

  x2 += overscan_offset_x;
  y2 += overscan_offset_y;

  if (outside(x1, y1) || outside(x2 - 1, y2 - 1)) return;

  int row_pixels = gr_draw->row_bytes / gr_draw->pixel_bytes;
  uint32_t* p = pixel_at(gr_draw, x1, y1, row_pixels);
  uint8_t alpha = static_cast<uint8_t>(((gr_current & alpha_mask) >> 24));
  if (alpha > 0) {
    for (int y = y1; y < y2; ++y) {
      uint32_t* px = p;
      for (int x = x1; x < x2; ++x) {
        *px = pixel_blend(alpha, *px);
        incr_x(&px, row_pixels);
      }
      incr_y(&p, row_pixels);
    }
  }
}

void gr_blit(GRSurface* source, int sx, int sy, int w, int h, int dx, int dy) {
  if (source == NULL) return;

  if (gr_draw->pixel_bytes != source->pixel_bytes) {
    printf("gr_blit: source has wrong format\n");
    return;
  }

  dx += overscan_offset_x;
  dy += overscan_offset_y;

  if (outside(dx, dy) || outside(dx + w - 1, dy + h - 1)) return;

  if (rotation) {
    int src_row_pixels = source->row_bytes / source->pixel_bytes;
    int row_pixels = gr_draw->row_bytes / gr_draw->pixel_bytes;
    uint32_t* src_py = reinterpret_cast<uint32_t*>(source->data) + sy * source->row_bytes / 4 + sx;
    uint32_t* dst_py = pixel_at(gr_draw, dx, dy, row_pixels);

    for (int y = 0; y < h; y += 1) {
      uint32_t* src_px = src_py;
      uint32_t* dst_px = dst_py;
      for (int x = 0; x < w; x += 1) {
        *dst_px = *src_px++;
        incr_x(&dst_px, row_pixels);
      }
      src_py += src_row_pixels;
      incr_y(&dst_py, row_pixels);
    }
  } else {
    unsigned char* src_p = source->data + sy * source->row_bytes + sx * source->pixel_bytes;
    unsigned char* dst_p = gr_draw->data + dy * gr_draw->row_bytes + dx * gr_draw->pixel_bytes;

    int i;
    for (i = 0; i < h; ++i) {
      memcpy(dst_p, src_p, w * source->pixel_bytes);
      src_p += source->row_bytes;
      dst_p += gr_draw->row_bytes;
    }
  }
}

unsigned int gr_get_width(GRSurface* surface) {
  if (surface == NULL) {
    return 0;
  }
  return surface->width;
}

unsigned int gr_get_height(GRSurface* surface) {
  if (surface == NULL) {
    return 0;
  }
  return surface->height;
}

int gr_init_font(const std::string &res_path, GRFont** dest) {
  GRFont* font = static_cast<GRFont*>(calloc(1, sizeof(*gr_font)));
  if (font == nullptr) {
    return -1;
  }

  int res = res_create_alpha_surface(res_path, &(font->texture));
  if (res < 0) {
    free(font);
    return res;
  }

  // The font image should be a 96x2 array of character images.  The
  // columns are the printable ASCII characters 0x20 - 0x7f.  The
  // top row is regular text; the bottom row is bold.
  font->char_width = font->texture->width / 96;
  font->char_height = font->texture->height / 2;

  *dest = font;

  return 0;
}

static void gr_init_font(void) {
  std::string res_path("/sdcard/cosmo-customos-installer/18x32.png");
  int res = gr_init_font(res_path, &gr_font);
  if (res != 0) {
    res_path = std::string("/res/images/font.png");
    res = gr_init_font(res_path, &gr_font);
  }
  if (res == 0) {
    gr_font_menu = gr_font;
    return;
  }

  printf("failed to read font: res=%d\n", res);

  // fall back to the compiled-in font.
  gr_font = static_cast<GRFont*>(calloc(1, sizeof(*gr_font)));
  gr_font->texture = static_cast<GRSurface*>(malloc(sizeof(*gr_font->texture)));
  gr_font->texture->width = font.width;
  gr_font->texture->height = font.height;
  gr_font->texture->row_bytes = font.width;
  gr_font->texture->pixel_bytes = 1;

  unsigned char* bits = static_cast<unsigned char*>(malloc(font.width * font.height));
  gr_font->texture->data = bits;

  unsigned char data;
  unsigned char* in = font.rundata;
  while ((data = *in++)) {
    memset(bits, (data & 0x80) ? 255 : 0, data & 0x7f);
    bits += (data & 0x7f);
  }

  gr_font->char_width = font.char_width;
  gr_font->char_height = font.char_height;
}

void gr_flip() {
  gr_draw = gr_backend->Flip();
}

int gr_init(GRRotation rot) {
  gr_init_font();

  auto backend = std::unique_ptr<MinuiBackend>{ std::make_unique<MinuiBackendAdf>() };
  gr_draw = backend->Init();

  if (!gr_draw) {
    backend = std::make_unique<MinuiBackendDrm>();
    gr_draw = backend->Init();
  }

  if (!gr_draw) {
    backend = std::make_unique<MinuiBackendFbdev>();
    gr_draw = backend->Init();
  }

  if (!gr_draw) {
    return -1;
  }

  gr_backend = backend.release();

  overscan_offset_x = gr_draw->width * overscan_percent / 100;
  overscan_offset_y = gr_draw->height * overscan_percent / 100;

  gr_flip();
  gr_flip();

  gr_rotate(rot);

  if (gr_draw->pixel_bytes != 4) {
    printf("gr_init: Only 4-byte pixel formats supported\n");
  }

  return 0;
}

void gr_exit() {
  delete gr_backend;
}

int gr_fb_width() {
  return rotation % 2 ? gr_draw->height - 2 * overscan_offset_y
                      : gr_draw->width - 2 * overscan_offset_x;
}

int gr_fb_height() {
  return rotation % 2 ? gr_draw->width - 2 * overscan_offset_x
                      : gr_draw->height - 2 * overscan_offset_y;
}

void gr_fb_blank(bool blank) {
  gr_backend->Blank(blank);
}

void gr_rotate(GRRotation rot) {
  rotation = rot;
}
