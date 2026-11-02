#include "src/platform/windows/apolloready_mask.h"
#include <cassert>
#include <fstream>
#include <iostream>

void save(const char *path, const std::vector<std::uint8_t> &pixels) {
  BITMAPFILEHEADER file {};
  file.bfType = 0x4d42;
  file.bfOffBits = sizeof(file) + sizeof(BITMAPINFOHEADER);
  file.bfSize = file.bfOffBits + pixels.size();
  BITMAPINFOHEADER info {};
  info.biSize = sizeof(info);
  info.biWidth = 1280;
  info.biHeight = -720;
  info.biPlanes = 1;
  info.biBitCount = 32;
  std::ofstream out(path, std::ios::binary);
  out.write(reinterpret_cast<const char *>(&file), sizeof(file));
  out.write(reinterpret_cast<const char *>(&info), sizeof(info));
  out.write(reinterpret_cast<const char *>(pixels.data()), pixels.size());
  assert(out.good());
}

int main() {
  auto loading = apolloready::render_mask(1280,720,false,false,"fr",0);
  auto animated = apolloready::render_mask(1280,720,false,false,"fr",3);
  auto closing = apolloready::render_mask(1280,720,true,false,"fr",0);
  auto paused = apolloready::render_mask(1280,720,false,true,"fr",0);
  assert(apolloready::mask_text("fr_FR").locale == "fr");
  assert(apolloready::mask_text("FR-fr.UTF-8").locale == "fr");
  assert(apolloready::mask_text("en_GB").locale == "en");
  assert(apolloready::mask_text("pt_BR").locale == "pt-br");
  assert(apolloready::mask_text("pt-PT").locale == "pt");
  assert(apolloready::mask_text("zh-Hant-TW").locale == "zh-tw");
  assert(apolloready::mask_text("zh_CN").locale == "zh");
  assert(apolloready::mask_text("unknown").locale == "en");
  assert(apolloready::mask_text("").locale == "en");
  assert(loading == apolloready::render_mask(1280,720,false,false,"fr-FR",0));
  assert(loading != apolloready::render_mask(1280,720,false,false,"en",0));
  assert(loading.size() == 1280u*720*4);
  assert(loading != animated && loading != closing && loading != paused);
  for (std::size_t i = 3; i < loading.size(); i += 4) assert(loading[i] == 255);
  assert(apolloready::render_mask(0,720,false,false,"fr",0).empty());
  auto hdr = apolloready::mask_to_scrgb({0,0,255,255,255,255,255,255});
  assert(hdr.size() == 8 && hdr[0] == 0x3c00 && hdr[1] == 0 && hdr[2] == 0 && hdr[3] == 0x3c00);
  assert(hdr[4] == 0x3c00 && hdr[5] == 0x3c00 && hdr[6] == 0x3c00);
  save("mask-loading.bmp", loading);
  save("mask-closing.bmp", closing);
  save("mask-paused.bmp", paused);
  for (const auto &translation : apolloready::mask_translations) {
    assert(*translation.loading && *translation.loading_detail && *translation.closing &&
      *translation.closing_detail && *translation.background && *translation.background_detail);
  }
  save("mask-german.bmp", apolloready::render_mask(1280,720,false,true,"de",0));
  save("mask-japanese.bmp", apolloready::render_mask(1280,720,false,false,"ja",0));
  std::cout << "PASS: mask dimensions, phase screens, animation, opacity and HDR channel conversion\n";
}
