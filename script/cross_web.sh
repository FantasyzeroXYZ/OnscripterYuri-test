# sh -c "BUILD_TYPE=Debug ./cross_web.sh"
BUILD_PATH=$(pwd)/../build_web
CMAKELISTS_PATH=$(pwd)/..
PORTBUILD_PATH=$CMAKELISTS_PATH/thirdparty/build/arch_wasm
CORE_NUM=$(cat /proc/cpuinfo | grep -c ^processor)
TARGETS=$@

# config env
source ./_fetch.sh
if [ -n "$(uname -a | grep Msys)" ]; then # fix python problem in windows
    PATH=$MSYS2_HOME/mingw32/bin/:$PATH
fi
source "$EMSDK_HOME/emsdk_env.sh"
# [WIN PATCH] emsdk_env 会整体重写 PATH，丢掉 Git Bash 会话里追加的工具目录；
# 在其后重新追加（make 走 mingw32-make 副本，emcc/embuilder 在 emsdk upstream）
export PATH="/d/ons-tools:/d/emsdk/upstream/emscripten:/d/emsdk/upstream/bin:/d/emsdk/node/24.19.0_64bit/bin:$PATH"
if [ -z "$BUILD_TYPE" ]; then BUILD_TYPE=MinSizeRel; fi
if [ -z "$TARGETS" ]; then TARGETS=all; fi

# build ports
function build_lua()
{
    echo "## LUA_SRC=$LUA_SRC"
    if ! [ -d "$PORTBUILD_PATH/lua" ]; then mkdir -p "$PORTBUILD_PATH/lua"; fi
    make -C $LUA_SRC clean
    make -C $LUA_SRC all PLAT=linux CC=emcc AR="emar rcu" -j$CORE_NUM
    make -C $LUA_SRC install INSTALL_TOP=$PORTBUILD_PATH -j$CORE_NUM
}

fetch_lua && build_lua
embuilder build sdl2 sdl2_ttf sdl2_image sdl2_mixer bzip2 ogg vorbis mpg123
emcc $CMAKELISTS_PATH/src/onsyuri_web/dummy.c \
    -o $BUILD_PATH/dummy.js \
    -sUSE_SDL=2 -sUSE_SDL_IMAGE=2 -sUSE_SDL_MIXER=2 \
    -sSDL2_IMAGE_FORMATS=bmp,png,jpg \
    -sSDL2_MIXER_FORMATS=ogg,mp3,mid -sUSE_OGG=1 -sUSE_VORBIS=1 -sUSE_MPG123=1

# config and build project
# rm -rf $BUILD_PATH/*
if [ -n "$GAME_PATH" ]; then cp -rf $GAME_PATH/* $BUILD_PATH; fi
emcmake cmake -G "Unix Makefiles" \
    -S $CMAKELISTS_PATH -B $BUILD_PATH \
    -DCMAKE_BUILD_TYPE=$BUILD_TYPE
make -C $BUILD_PATH $TARGETS -j$CORE_NUM