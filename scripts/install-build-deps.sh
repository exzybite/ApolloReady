#!/usr/bin/env bash
set -euo pipefail
pacman -S --needed --noconfirm git \
  mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-boost \
  mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja \
  mingw-w64-ucrt-x86_64-pkgconf mingw-w64-ucrt-x86_64-cppwinrt \
  mingw-w64-ucrt-x86_64-curl-winssl mingw-w64-ucrt-x86_64-MinHook \
  mingw-w64-ucrt-x86_64-miniupnpc mingw-w64-ucrt-x86_64-nodejs \
  mingw-w64-ucrt-x86_64-onevpl mingw-w64-ucrt-x86_64-openssl \
  mingw-w64-ucrt-x86_64-opus mingw-w64-ucrt-x86_64-nlohmann_json \
  mingw-w64-ucrt-x86_64-nsis mingw-w64-ucrt-x86_64-7zip
