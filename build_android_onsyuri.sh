# build_android_onsyuri.sh - rebuild arm64 libonsyuri.so with [ONS-BRIDGE] hooks
# ============================================================================
# Mirrors config_android() in CMakeLists.txt (same sources/defines/libs) but
# only builds the onsyuri core, linking against the PREBUILT SDL2 family from
# the Tyranor-Next ONS native plugin (same versions the shipped .so used:
# SDL2 2.26.3 / image 2.6.3 / ttf 2.20.2 / mixer 2.6.3 / lua 5.4.4 / bzip2 1.0.8).
#
# Usage:  ./build_android_onsyuri.sh <NDK_PATH> <TYRANOR_ONS_LIB_DIR> [out.so]
# e.g.    ./build_android_onsyuri.sh /d/dev-cache/Android/Sdk/ndk/28.2.13676358 \
#             /d/code/Tyranor-Next/engine/src/main/nativeplugins/ons/arm64-v8a
# ============================================================================
set -e

NDK="$1"
LIBDIR="$2"
OUT="${3:-./libonsyuri.so}"
if [ -z "$NDK" ] || [ -z "$LIBDIR" ]; then
    echo "usage: $0 <NDK_PATH> <TYRANOR_ONS_LIB_DIR> [out.so]" >&2
    exit 1
fi

ROOT="$(cd "$(dirname "$0")" && pwd)"
PORT="$ROOT/thirdparty/port"
API=26
TRIPLE=aarch64-linux-android
CXX="$NDK/toolchains/llvm/prebuilt/windows-x86_64/bin/${TRIPLE}${API}-clang++"

# ONSYURI_CODE from root CMakeLists.txt
SRCS="
src/onsyuri/AnimationInfo.cpp
src/onsyuri/coding2utf16.cpp
src/onsyuri/DirectReader.cpp
src/onsyuri/DirtyRect.cpp
src/onsyuri/FontInfo.cpp
src/onsyuri/gbk2utf16.cpp
src/onsyuri/LUAHandler.cpp
src/onsyuri/NsaReader.cpp
src/onsyuri/ONScripter.cpp
src/onsyuri/ONScripter_animation.cpp
src/onsyuri/ONScripter_command.cpp
src/onsyuri/ONScripter_effect.cpp
src/onsyuri/ONScripter_effect_breakup.cpp
src/onsyuri/ONScripter_event.cpp
src/onsyuri/ONScripter_file.cpp
src/onsyuri/ONScripter_file2.cpp
src/onsyuri/ONScripter_image.cpp
src/onsyuri/ONScripter_lut.cpp
src/onsyuri/ONSInput.cpp
src/onsyuri/onscripter_main.cpp
src/onsyuri/ONScripter_rmenu.cpp
src/onsyuri/ONScripter_sound.cpp
src/onsyuri/ONScripter_text.cpp
src/onsyuri/Parallel.cpp
src/onsyuri/resize_image.cpp
src/onsyuri/SarReader.cpp
src/onsyuri/ScriptHandler.cpp
src/onsyuri/ScriptParser.cpp
src/onsyuri/ScriptParser_command.cpp
src/onsyuri/sjis2utf16.cpp
src/onsyuri/renderer/gles_renderer.cpp
src/onsyuri/builtin_dll/layer_oldmovie.cpp
src/onsyuri/builtin_dll/layer_snow.cpp
src/onsyuri/builtin_dll/ONScripter_effect_cascade.cpp
src/onsyuri/builtin_dll/ONScripter_effect_trig.cpp
"

DEFINES="-DUSE_LUA -DUSE_BUILTIN_LAYER_EFFECTS -DENABLE_1BYTE_CHAR -DANDROID -DUSE_PARALLEL -DUSE_GLES -DUSE_FILELOG -DUTF8_FILESYSTEM -DSDL_JAVA_PACKAGE_PATH=com_yuri_onscripter"

INCLUDES="
-I$ROOT/src/onsyuri
-I$PORT/SDL2-2.26.3/include
-I$PORT/SDL2_image-2.6.3
-I$PORT/SDL2_image-2.6.3/external/jpeg
-I$PORT/SDL2_ttf-2.20.2
-I$PORT/SDL2_mixer-2.6.3
-I$PORT/SDL2_mixer-2.6.3/include
-I$PORT/lua-5.4.4/src
-I$PORT/bzip2-1.0.8
"

CXXFLAGS="-std=c++11 -Os -fPIC -Wno-invalid-source-encoding -Wno-unknown-attributes $DEFINES $INCLUDES"
LDFLAGS="-shared -Os -flto -static-libstdc++ -Wl,-z,max-page-size=16384 -Wl,-soname,libonsyuri.so -Wl,--allow-shlib-undefined"

echo "## building $OUT"
OBJDIR="$ROOT/build_android_obj"
mkdir -p "$OBJDIR"
OBJS=""
for s in $SRCS; do
    obj="$OBJDIR/$(echo "$s" | tr '/' '_').o"
    if [ ! -f "$obj" ] || [ "$s" -nt "$obj" ]; then
        echo "CXX $s"
        "$CXX" $CXXFLAGS -c "$ROOT/$s" -o "$obj"
    fi
    OBJS="$OBJS $obj"
done

"$CXX" $OBJS $LDFLAGS -o "$OUT" \
    -L"$LIBDIR" -lSDL2 -lSDL2_image -lSDL2_ttf -lSDL2_mixer -llua -ljpeg -lbz2 -lGLESv2 -llog
echo "## done: $OUT"
