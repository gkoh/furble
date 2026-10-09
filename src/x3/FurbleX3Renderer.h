#ifndef FURBLE_X3_RENDERER_H
#define FURBLE_X3_RENDERER_H
#include <algorithm>
#include <cstdio>
#include <cstring>
#include "vendor/FurbleUISans.h"
#include "x3/FurbleX3View.h"

namespace Furble {
// Logical portrait coordinates rotate directly into the controller's packed
// landscape framebuffer. This is the FreeInk X3 Portrait transform; the SPI
// driver's verified bottom-first gate order and refresh waveform stay intact.
class X3Renderer {
 public:
  static constexpr int WIDTH = 528, HEIGHT = 792;
  static constexpr int PANEL_WIDTH = 792, PANEL_HEIGHT = 528;
  static constexpr int STRIDE = PANEL_WIDTH / 8, FRAME_SIZE = STRIDE * PANEL_HEIGHT;
  explicit X3Renderer(uint8_t *buffer) : frame(buffer) {}
  static constexpr int panelX(int x, int y) {
    (void)x;
    return y;
  }
  static constexpr int panelY(int x, int y) {
    (void)y;
    return PANEL_HEIGHT - 1 - x;
  }
  // Read back a logical pixel for host previews and orientation checks.
  bool inkAt(int x, int y) const {
    if (x < 0 || y < 0 || x >= WIDTH || y >= HEIGHT)
      return false;
    const int px = panelX(x, y), py = panelY(x, y);
    return !(frame[py * STRIDE + px / 8] & (0x80 >> (px % 8)));
  }
  void render(const X3View &view) {
    memset(frame, 0xff, FRAME_SIZE);
    camera(28, 29, true);
    text(70, 28, "FURBLE", x3Font18);
    battery(view.batteryPercent);
    wrapped(28, 72, view.title, x3Font32, 472, 2, 38);
    rectangle(28, 157, 472, 2, true);
    wrapped(28, 172, view.status[0] ? view.status : "Bluetooth camera control", x3Font18, 472, 2,
            24);

    const unsigned count = std::min<unsigned>(view.lineCount, X3View::MAX_LINES);
    if (view.kind == X3View::Kind::REMOTE && count) {
      hero(view);
      const int rowHeight = std::min(64u, 322 / std::max(1u, count - 1));
      for (unsigned i = 1; i < count; ++i)
        row(view.lines[i], 367 + (i - 1) * rowHeight, rowHeight,
            view.selected == static_cast<int>(i));
    } else {
      const int rowHeight = std::min(74u, 464 / std::max(1u, count));
      for (unsigned i = 0; i < count; ++i)
        row(view.lines[i], 221 + i * rowHeight, rowHeight, view.selected == static_cast<int>(i),
            view.kind == X3View::Kind::INFO);
    }
    rectangle(28, 701, 472, 2, true);
    wrapped(28, 718, view.footer, x3Font18, 472, 3, 23);
  }

 private:
  uint8_t *frame;
  void pixel(int x, int y, bool black) {
    if (x < 0 || y < 0 || x >= WIDTH || y >= HEIGHT)
      return;
    const int px = panelX(x, y), py = panelY(x, y);
    uint8_t &b = frame[py * STRIDE + px / 8];
    uint8_t mask = 0x80 >> (px % 8);
    if (black)
      b &= ~mask;
    else
      b |= mask;
  }
  void rectangle(int x, int y, int w, int h, bool black) {
    for (int yy = std::max(0, y); yy < std::min(HEIGHT, y + h); ++yy)
      for (int xx = std::max(0, x); xx < std::min(WIDTH, x + w); ++xx)
        pixel(xx, yy, black);
  }
  void outline(int x, int y, int w, int h, int weight, bool black) {
    rectangle(x, y, w, weight, black);
    rectangle(x, y + h - weight, w, weight, black);
    rectangle(x, y, weight, h, black);
    rectangle(x + w - weight, y, weight, h, black);
  }
  void rounded(int x, int y, int w, int h, int radius, bool black) {
    for (int yy = 0; yy < h; ++yy) {
      int inset = 0;
      const int dy = yy < radius ? radius - yy - 1 : yy >= h - radius ? yy - (h - radius) : 0;
      while (inset < radius && (radius - inset) * (radius - inset) + dy * dy > radius * radius)
        ++inset;
      rectangle(x + inset, y + yy, w - 2 * inset, 1, black);
    }
  }
  static const X3Glyph &glyph(unsigned char c, const X3Font &font) {
    return font.glyphs[(c >= 32 && c <= 126 ? c : '?') - 32];
  }
  static int measure(const char *s, const X3Font &font) {
    int width = 0;
    for (; *s; ++s)
      width += glyph(static_cast<unsigned char>(*s), font).advance;
    return width;
  }
  int text(int x,
           int y,
           const char *s,
           const X3Font &font,
           bool black = true,
           int maxX = WIDTH - 28) {
    for (; *s; ++s) {
      const auto &g = glyph(static_cast<unsigned char>(*s), font);
      if (x + std::max(g.width, g.advance) > maxX)
        break;
      for (int yy = 0; yy < g.height; ++yy)
        for (int xx = 0; xx < g.width; ++xx) {
          const int bit = yy * g.width + xx;
          if (font.bits[g.offset + bit / 8] & (0x80 >> (bit % 8)))
            pixel(x + xx, y + g.top + yy, black);
        }
      x += g.advance;
    }
    return x;
  }
  void clipped(int x, int y, const char *s, const X3Font &font, bool black, int maxX) {
    if (measure(s, font) <= maxX - x) {
      text(x, y, s, font, black, maxX);
      return;
    }
    char line[80] = {};
    int used = 0;
    unsigned n = 0;
    const int dots = measure("...", font);
    while (s[n] && n < sizeof(line) - 4 && used + glyph(s[n], font).advance + dots <= maxX - x) {
      used += glyph(s[n], font).advance;
      line[n] = s[n];
      ++n;
    }
    strcpy(line + n, "...");
    text(x, y, line, font, black, maxX);
  }
  void wrapped(int x, int y, const char *s, const X3Font &font, int width, int maxLines, int step) {
    for (int row = 0; *s && row < maxLines; ++row) {
      while (*s == ' ')
        ++s;
      if (row == maxLines - 1) {
        clipped(x, y + row * step, s, font, true, x + width);
        break;
      }
      char line[80] = {};
      unsigned n = 0, lastSpace = 0;
      int used = 0;
      while (s[n] && n < sizeof(line) - 1 && used + glyph(s[n], font).advance <= width) {
        used += glyph(s[n], font).advance;
        if (s[n] == ' ')
          lastSpace = n;
        ++n;
      }
      if (s[n] && lastSpace)
        n = lastSpace;
      if (!n)
        break;
      memcpy(line, s, n);
      text(x, y + row * step, line, font, true, x + width);
      s += n;
    }
  }
  void camera(int x, int y, bool black) {
    outline(x, y + 5, 30, 21, 2, black);
    outline(x + 9, y, 12, 7, 2, black);
    outline(x + 10, y + 10, 10, 10, 2, black);
    rectangle(x + 24, y + 9, 3, 3, black);
  }
  void battery(int percent) {
    char label[12];
    snprintf(label, sizeof(label), percent < 0 ? "--" : "%d%%", std::min(100, percent));
    text(438 - measure(label, x3Font18), 28, label, x3Font18);
    outline(449, 33, 42, 20, 2, true);
    rectangle(491, 39, 4, 8, true);
    if (percent >= 0)
      rectangle(453, 37, 34 * std::min(100, percent) / 100, 12, true);
  }
  void chevron(int x, int y, bool black) {
    for (int i = 0; i < 7; ++i) {
      rectangle(x + i, y + i, 2, 2, black);
      rectangle(x + i, y + 12 - i, 2, 2, black);
    }
  }
  void row(const char *label, int y, int height, bool selected, bool info = false) {
    if (selected)
      rounded(20, y + 3, 488, height - 6, 9, true);
    else if (!info)
      rectangle(36, y + height - 1, 456, 1, true);
    const bool ink = !selected;
    const int textY = y + (height - 28) / 2;
    const char *colon = strstr(label, ": ");
    if (colon && measure(colon + 2, x3Font24) < 168) {
      char name[64] = {};
      memcpy(name, label, std::min<size_t>(colon - label, sizeof(name) - 1));
      const int valueX = 478 - measure(colon + 2, x3Font24);
      clipped(36, textY, name, x3Font24, ink, valueX - 18);
      text(valueX, textY, colon + 2, x3Font24, ink, 478);
    } else
      clipped(36, textY, label, x3Font24, ink, info ? 492 : 466);
    if (selected && !colon)
      chevron(482, textY + 7, false);
  }
  void hero(const X3View &view) {
    const bool selected = view.selected == 0;
    rounded(20, 225, 488, 125, 12, true);
    camera(40, 244, false);
    text(83, 239, "SHUTTER", x3Font18, false);
    clipped(40, 269, view.lines[0], x3Font32, false, 480);
    text(40, 315, "Hold right-side button", x3Font18, false);
    if (selected)
      chevron(478, 248, false);
  }
};
}  // namespace Furble

#endif
