#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/../build"

if [ ! -d "${BUILD_DIR}" ]; then
    echo "Build directory not found. Please run ./scripts/build.sh first."
    exit 1
fi

cd "${BUILD_DIR}"

echo "=========================================="
echo " 1. Running Allocator Benchmark           "
echo "=========================================="
./bench_allocator

echo ""
echo "=========================================="
echo " 2. Running Network Flow Benchmark        "
echo "=========================================="
./bench_flow

echo ""
echo "=========================================="
echo " 3. Running 500k+ Node Pathfinding Bench  "
echo "=========================================="
./bench_pathfinding 500000
