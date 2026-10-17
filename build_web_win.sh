#!/bin/bash
# Windows(Git Bash) 自动构建 onsyuri web 版：包装 cross_web.sh + 下载重试
export EMSDK_HOME=/d/emsdk
export EMSDK=/d/emsdk
export PATH=/d/ons-tools:/d/emsdk/upstream/emscripten:/d/emsdk/upstream/bin:/d/emsdk/node/24.19.0_64bit/bin:/d/software/STM32CubeCLT_1.18.0/Ninja/bin:/c/Program\ Files/mingw64/bin:$PATH
export BUILD_TYPE=MinSizeRel

cd "$(dirname "$0")/script"
for i in $(seq 1 15); do
    echo "===== web build attempt $i ====="
    BUILD_TYPE=$BUILD_TYPE ./cross_web.sh > /tmp/ons_build_iter.log 2>&1
    rc=$?
    if [ $rc -eq 0 ]; then
        echo "BUILD-OK at attempt $i"
        break
    fi
    # 提取最后失败的下载 URL 预下载（fetch_port 的缓存是目录，删掉坏包重下）
    badtar=$(grep -E "curl|tar" /tmp/ons_build_iter.log | grep -oE "https?://[^ ]+\.tar\.gz" | tail -1)
    echo "attempt $i 失败 (rc=$rc)，最后涉及的包: $badtar"
    if [ -n "$badtar" ]; then
        name=$(basename "$badtar" .tar.gz)
        rm -rf "../thirdparty/port/$name" "../thirdparty/port/$name.tar.gz"
        curl -sL --retry 30 --retry-all-errors --retry-delay 2 --max-time 600 -o "../thirdparty/port/$name.tar.gz" "$badtar" \
          && tar zxf "../thirdparty/port/$name.tar.gz" -C ../thirdparty/port && echo "预下载成功: $name"
    fi
    sleep 5
done
