#!/usr/bin/env bash
set -euo pipefail

echo "=========================================="
echo " Building GraphFlow (Release Mode, C++20) "
echo "=========================================="

BUILD_DIR="build"
mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

cmake .. -DCMAKE_BUILD_TYPE=Release -GNinja 2>/dev/null || cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -- -j$(nproc)

echo "Build complete! Executables located in ${BUILD_DIR}/"
