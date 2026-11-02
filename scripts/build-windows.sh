#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
export BRANCH=main
export BUILD_VERSION="$(tr -d '\r\n' < VERSION.txt)"
export TAG="v$BUILD_VERSION"
cmake -S . -B build-gameready -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DBUILD_DOCS=OFF -DBUILD_TESTS=OFF -DBUILD_WERROR=OFF \
  -DSUNSHINE_PUBLISHER_NAME=ApolloReady \
  -DSUNSHINE_PUBLISHER_WEBSITE=https://github.com/exzybite/ApolloReady \
  -DSUNSHINE_PUBLISHER_ISSUE_URL=https://github.com/exzybite/ApolloReady/issues
node tests/apolloready_restart_test.mjs
g++ -std=c++23 -static -pthread -I . tests/apolloready_session_test.cpp -o build-gameready/apolloready-session-test.exe
./build-gameready/apolloready-session-test.exe
g++ -std=c++23 -static -I . tests/apolloready_mask_test.cpp -lgdi32 -o build-gameready/apolloready-mask-test.exe
(cd build-gameready && ./apolloready-mask-test.exe)
cmake --build build-gameready --target sunshine sunshinesvc web-ui --parallel "${BUILD_JOBS:-2}"
(cd build-gameready && cpack -G NSIS --config CPackConfig.cmake)
