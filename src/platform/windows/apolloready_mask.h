#pragma once

#include <windows.h>
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdint>
#include <vector>
#include "apolloready_mask_locale.h"

namespace apolloready {
  // Draw into an offscreen DIB. No window, monitor or desktop is sampled.
  inline std::vector<std::uint8_t> render_mask(int width, int height, bool closing, bool paused, std::string_view locale, unsigned step) {
    if (width <= 0 || height <= 0 || width > 16384 || height > 16384) return {};
    HDC dc = CreateCompatibleDC(nullptr);
    if (!dc) return {};
    BITMAPINFO info {};
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = width;
    info.bmiHeader.biHeight = -height;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;
    void *bits = nullptr;
    HBITMAP bitmap = CreateDIBSection(dc, &info, DIB_RGB_COLORS, &bits, nullptr, 0);
    if (!bitmap) { DeleteDC(dc); return {}; }
    auto previous_bitmap = SelectObject(dc, bitmap);
    auto pixels = static_cast<std::uint8_t *>(bits);
    for (int y = 0; y < height; ++y) {
      for (int x = 0; x < width; ++x) {
        auto p = pixels + (std::size_t(y) * width + x) * 4;
        p[0] = 25 + y * 10 / height;
        p[1] = 16 + y * 5 / height;
        p[2] = 12;
        p[3] = 255;
      }
    }
    const int unit = std::max(1, height / 720);
    auto text = [&](const wchar_t *label, int top, int size, int weight, COLORREF color) {
      HFONT font = CreateFontW(-std::max(12, height * size / 720), 0, 0, 0, weight, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH, L"Segoe UI");
      if (!font) return;
      auto previous_font = SelectObject(dc, font);
      SetBkMode(dc, TRANSPARENT);
      SetTextColor(dc, color);
      RECT rect {0, height * top / 720, width, height * (top + size + 20) / 720};
      DrawTextW(dc, label, -1, &rect, DT_CENTER | DT_SINGLELINE | DT_NOPREFIX);
      SelectObject(dc, previous_font);
      DeleteObject(font);
    };
    text(L"ApolloReady", 230, 28, FW_SEMIBOLD, RGB(101, 222, 222));
    const auto &labels = mask_text(locale);
    const wchar_t *title = closing ? labels.closing : paused ? labels.background : labels.loading;
    text(title, 300, 43, FW_SEMIBOLD, RGB(241, 246, 252));
    text(closing ? labels.closing_detail : paused ? labels.background_detail : labels.loading_detail,
      365, 21, FW_NORMAL, RGB(159, 178, 198));
    int bar_width = width / 5;
    int bar_left = (width - bar_width) / 2;
    int bar_top = height * 440 / 720;
    HBRUSH track = CreateSolidBrush(RGB(36, 55, 70));
    RECT bar {bar_left, bar_top, bar_left + bar_width, bar_top + 4 * unit};
    FillRect(dc, &bar, track);
    DeleteObject(track);
    int segment = std::max(1, bar_width / 5);
    // A continuous three-second round trip, with gentle turns at either end.
    double phase = (step % 91) * 33.0 / 3000.0;
    int position = static_cast<int>((0.5 - 0.5 * std::cos(phase * 6.283185307179586)) * (bar_width - segment));
    HBRUSH accent = CreateSolidBrush(RGB(101, 222, 222));
    RECT indicator {bar_left + position, bar_top, bar_left + position + segment, bar.bottom};
    FillRect(dc, &indicator, accent);
    DeleteObject(accent);
    GdiFlush();
    std::vector<std::uint8_t> result(pixels, pixels + std::size_t(width) * height * 4);
    // GDI text may clear alpha in the DIB; this screen is always opaque.
    for (std::size_t i = 3; i < result.size(); i += 4) result[i] = 255;
    SelectObject(dc, previous_bitmap);
    DeleteObject(bitmap);
    DeleteDC(dc);
    return result;
  }

  inline std::uint16_t mask_half(float value) {
    // Inputs are nonnegative, finite linear SDR values in [0,1].
    if (value == 0) return 0;
    auto bits = std::bit_cast<std::uint32_t>(value);
    int exponent = int((bits >> 23) & 255) - 127 + 15;
    if (exponent <= 0) return 0; // Tiny background values may round to black.
    return static_cast<std::uint16_t>((exponent << 10) | ((bits >> 13) & 1023));
  }

  inline std::vector<std::uint16_t> mask_to_scrgb(const std::vector<std::uint8_t> &bgra) {
    std::uint16_t linear[256];
    for (int i = 0; i < 256; ++i) {
      float s = i / 255.0f;
      linear[i] = mask_half(s <= 0.04045f ? s / 12.92f : std::pow((s + 0.055f) / 1.055f, 2.4f));
    }
    std::vector<std::uint16_t> result(bgra.size());
    for (std::size_t i = 0; i + 3 < bgra.size(); i += 4) {
      result[i] = linear[bgra[i + 2]];
      result[i + 1] = linear[bgra[i + 1]];
      result[i + 2] = linear[bgra[i]];
      result[i + 3] = 0x3c00; // alpha = 1
    }
    return result;
  }
}
