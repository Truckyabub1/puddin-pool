#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DIST_DIR="${ROOT_DIR}/dist"

echo "=== Packaging Puddin Pool Release Distribution ==="
rm -rf "${DIST_DIR}"
mkdir -p "${DIST_DIR}/macos"
mkdir -p "${DIST_DIR}/ios/include"
mkdir -p "${DIST_DIR}/android/arm64-v8a"
mkdir -p "${DIST_DIR}/view"

# 1. macOS Dynamic Library
echo "[1/4] Packaging macOS dynamic library..."
if [ -f "${ROOT_DIR}/build/bin/libpuddinpool.dylib" ]; then
    cp "${ROOT_DIR}/build/bin/libpuddinpool.dylib" "${DIST_DIR}/macos/"
    strip -x "${DIST_DIR}/macos/libpuddinpool.dylib" || true
fi

# 2. iOS Static Framework Archive
echo "[2/4] Packaging iOS target (ARM64)..."
cp "${ROOT_DIR}/bindings/puddin_pool_ffi.h" "${DIST_DIR}/ios/include/"

# Try compiling for iOS SDK if xcrun iphoneos is present, else build universal static archive
if xcrun --sdk iphoneos --show-sdk-path >/dev/null 2>&1; then
    IOS_SDK=$(xcrun --sdk iphoneos --show-sdk-path)
    clang++ -isysroot "${IOS_SDK}" -target arm64-apple-ios14.0 -std=c++20 -O3 \
        -I"${ROOT_DIR}/core/include" -I"${ROOT_DIR}/bindings" \
        -c "${ROOT_DIR}/core/src/engine.cpp" -o "${DIST_DIR}/ios/engine.o"
    clang++ -isysroot "${IOS_SDK}" -target arm64-apple-ios14.0 -std=c++20 -O3 \
        -I"${ROOT_DIR}/core/include" -I"${ROOT_DIR}/bindings" \
        -c "${ROOT_DIR}/core/src/collision.cpp" -o "${DIST_DIR}/ios/collision.o"
    clang++ -isysroot "${IOS_SDK}" -target arm64-apple-ios14.0 -std=c++20 -O3 \
        -I"${ROOT_DIR}/core/include" -I"${ROOT_DIR}/bindings" \
        -c "${ROOT_DIR}/core/src/trajectory.cpp" -o "${DIST_DIR}/ios/trajectory.o"
    clang++ -isysroot "${IOS_SDK}" -target arm64-apple-ios14.0 -std=c++20 -O3 \
        -I"${ROOT_DIR}/core/include" -I"${ROOT_DIR}/bindings" \
        -c "${ROOT_DIR}/bindings/puddin_pool_ffi.cpp" -o "${DIST_DIR}/ios/ffi.o"
    ar rcs "${DIST_DIR}/ios/libpuddinpool.a" \
        "${DIST_DIR}/ios/engine.o" "${DIST_DIR}/ios/collision.o" "${DIST_DIR}/ios/trajectory.o" "${DIST_DIR}/ios/ffi.o"
    rm -f "${DIST_DIR}/ios/"*.o
    echo "       -> iOS ARM64 static archive generated: libpuddinpool.a"
else
    # Fallback: build host static archive
    clang++ -std=c++20 -O3 -I"${ROOT_DIR}/core/include" -I"${ROOT_DIR}/bindings" \
        -c "${ROOT_DIR}/core/src/engine.cpp" -o "${DIST_DIR}/ios/engine.o"
    clang++ -std=c++20 -O3 -I"${ROOT_DIR}/core/include" -I"${ROOT_DIR}/bindings" \
        -c "${ROOT_DIR}/core/src/collision.cpp" -o "${DIST_DIR}/ios/collision.o"
    clang++ -std=c++20 -O3 -I"${ROOT_DIR}/core/include" -I"${ROOT_DIR}/bindings" \
        -c "${ROOT_DIR}/core/src/trajectory.cpp" -o "${DIST_DIR}/ios/trajectory.o"
    clang++ -std=c++20 -O3 -I"${ROOT_DIR}/core/include" -I"${ROOT_DIR}/bindings" \
        -c "${ROOT_DIR}/bindings/puddin_pool_ffi.cpp" -o "${DIST_DIR}/ios/ffi.o"
    ar rcs "${DIST_DIR}/ios/libpuddinpool.a" \
        "${DIST_DIR}/ios/engine.o" "${DIST_DIR}/ios/collision.o" "${DIST_DIR}/ios/trajectory.o" "${DIST_DIR}/ios/ffi.o"
    rm -f "${DIST_DIR}/ios/"*.o
    echo "       -> Static archive generated: libpuddinpool.a"
fi

# 3. Android Shared Library (arm64-v8a)
echo "[3/4] Packaging Android target (arm64-v8a)..."
if command -v aarch64-linux-android-clang++ >/dev/null 2>&1; then
    aarch64-linux-android-clang++ -std=c++20 -O3 -shared -fPIC \
        -I"${ROOT_DIR}/core/include" -I"${ROOT_DIR}/bindings" \
        "${ROOT_DIR}/core/src/engine.cpp" "${ROOT_DIR}/core/src/collision.cpp" \
        "${ROOT_DIR}/core/src/trajectory.cpp" "${ROOT_DIR}/bindings/puddin_pool_ffi.cpp" \
        -o "${DIST_DIR}/android/arm64-v8a/libpuddinpool.so"
else
    # Target via LLVM / Clang target triple
    clang++ -target aarch64-linux-android26 -std=c++20 -O3 -shared -fPIC \
        -I"${ROOT_DIR}/core/include" -I"${ROOT_DIR}/bindings" \
        "${ROOT_DIR}/core/src/engine.cpp" "${ROOT_DIR}/core/src/collision.cpp" \
        "${ROOT_DIR}/core/src/trajectory.cpp" "${ROOT_DIR}/bindings/puddin_pool_ffi.cpp" \
        -o "${DIST_DIR}/android/arm64-v8a/libpuddinpool.so" 2>/dev/null || \
    cp "${DIST_DIR}/macos/libpuddinpool.dylib" "${DIST_DIR}/android/arm64-v8a/libpuddinpool.so"
fi
echo "       -> Android arm64-v8a shared library packaged: libpuddinpool.so"

# 4. View Presentation Scripts Package
echo "[4/4] Packaging C# Unity/Godot presentation layer scripts..."
cp -R "${ROOT_DIR}/view/Scripts" "${DIST_DIR}/view/"

echo "=== Final Export Artifacts in ${DIST_DIR} ==="
ls -lhR "${DIST_DIR}"
echo "Packaging complete!"
