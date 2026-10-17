# fix-shared-cursor.py — clickWait 与 clickNewPage 共用同一增量游标（v2 修正锚点）
import os

P = r'D:\Desktop\OnscripterYuri-master\src\onsyuri\ONScripter_text.cpp'
src = open(P, encoding='utf-8', newline='').read()

# 0) 声明共享游标（若未有）
marker = '#include "ONSBridge.h"'
decl = (
    '#include "ONSBridge.h"\n'
    '\n'
    '// [ONS-BRIDGE] 页文本增量游标（clickWait 与 clickNewPage 共用；按字节计）\n'
    'static int s_bridge_emitted = 0;'
)
if 's_bridge_emitted' not in src:
    assert marker in src, 'include anchor'
    src = src.replace(marker, decl, 1)
    print('decl added')

# 1) clickWait 块：static 局部游标 → 共享游标
old1 = """    // [ONS-BRIDGE] 页文本增量上抛：current_page->text 是引擎维护的当前页原始
    // 文本缓冲（脚本编码）。clickWait 在每个点击等待点触发——上抛自上次上抛
    // 以来的增量（行级语义，匹配 TTS 逐行节奏）；页翻新（text_count 回退）时重置
    {
        static int s_emitted = 0;
        int total = current_page->text_count;
        if (total < s_emitted) s_emitted = 0; // newPage
        printf( "[ONS-BRIDGE] clickWait total=%d emitted=%d\\n", total, s_emitted );
        if (total > s_emitted) {
            // 原始脚本编码（SJIS/GBK/UTF-8）base64 透传，宿主嗅探解码
            std::string b64 = onsbridge::B64( current_page->text + s_emitted, total - s_emitted );
            if (!b64.empty()) onsbridge::Emit( "dialogue", "{\\"b64\\":\\"" + b64 + "\\"}" );
            s_emitted = total;
        }
    }"""
new1 = """    // [ONS-BRIDGE] 页文本增量上抛：current_page->text 是引擎维护的当前页原始
    // 文本缓冲（脚本编码）。游标与 clickNewPage 共用（s_bridge_emitted）——两种
    // 等待在同一脚本里交替触发，独立游标会让增量区间重叠/跳空产生乱码
    {
        int total = current_page->text_count;
        if (total < s_bridge_emitted) s_bridge_emitted = 0; // newPage
        fprintf( stderr, "[ONS-BRIDGE] clickWait total=%d emitted=%d\\n", total, s_bridge_emitted );
        if (total > s_bridge_emitted) {
            // 原始脚本编码（SJIS/GBK/UTF-8）base64 透传，宿主嗅探解码
            std::string b64 = onsbridge::B64( current_page->text + s_bridge_emitted, total - s_bridge_emitted );
            if (!b64.empty()) onsbridge::Emit( "dialogue", "{\\"b64\\":\\"" + b64 + "\\"}" );
            s_bridge_emitted = total;
        }
    }"""
if old1 in src:
    src = src.replace(old1, new1, 1)
    print('clickWait cursor shared')
else:
    print('clickWait anchor NOT FOUND (可能已改)')

# 2) clickNewPage 块：static 局部游标 → 共享游标
old2 = """    // [ONS-BRIDGE] 页末增量上抛（NOeSIS 等游戏的页等待走此路径而非 clickWait）
    {
        static int s_emitted = 0;
        int total = current_page->text_count;
        if (total < s_emitted) s_emitted = 0; // newPage
        if (total > s_emitted) {
            std::string b64 = onsbridge::B64( current_page->text + s_emitted, total - s_emitted );
            if (!b64.empty()) onsbridge::Emit( "dialogue", "{\\"b64\\":\\"" + b64 + "\\"}" );
            s_emitted = total;
        }
    }"""
new2 = """    // [ONS-BRIDGE] 页末增量上抛（与 clickWait 共用游标 s_bridge_emitted）
    {
        int total = current_page->text_count;
        if (total < s_bridge_emitted) s_bridge_emitted = 0; // newPage
        if (total > s_bridge_emitted) {
            std::string b64 = onsbridge::B64( current_page->text + s_bridge_emitted, total - s_bridge_emitted );
            if (!b64.empty()) onsbridge::Emit( "dialogue", "{\\"b64\\":\\"" + b64 + "\\"}" );
            s_bridge_emitted = total;
        }
    }"""
if old2 in src:
    src = src.replace(old2, new2, 1)
    print('clickNewPage cursor shared')
else:
    print('clickNewPage anchor NOT FOUND')
    import sys
    i = src.find('bool ONScripter::clickNewPage')
    print(repr(src[i:i+500]))
    sys.exit(1)

open(P, 'w', encoding='utf-8', newline='').write(src)
print('shared cursor applied')
