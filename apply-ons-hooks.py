# apply-ons-hooks.py — 统一行尾后应用 drawChar 追踪与 clickNewPage 钩子
import os

TEXT_CPP = r'D:\Desktop\OnscripterYuri-master\src\onsyuri\ONScripter_text.cpp'

# 1) 统一行尾为 LF（编译器无影响）
data = open(TEXT_CPP, 'rb').read()
data = data.replace(b'\r\n', b'\n')
open(TEXT_CPP, 'wb').write(data)
src = data.decode('utf-8')

# 2) drawChar 入口追踪（stderr，CDP 可见）
anchor_draw = (
    'void ONScripter::drawChar( char* text, FontInfo *info, bool flush_flag, '
    'bool lookback_flag, SDL_Surface *surface, AnimationInfo *cache_info, SDL_Rect *clip )\n'
    '{\n'
)
draw_dbg = (
    anchor_draw +
    '    // [ONS-BRIDGE] 诊断：文本绘制路径追踪（前 30 次）\n'
    '    {\n'
    '        static int dbg = 0;\n'
    '        if (dbg < 30) { fprintf( stderr, "[ONS-BRIDGE] drawChar #%d info=sentence:%d flush=%d text=%.12s\\n", dbg++, info == &sentence_font, flush_flag, text ); }\n'
    '    }\n'
)
if '[ONS-BRIDGE] drawChar' in src:
    print('drawChar: already applied')
elif anchor_draw in src:
    src = src.replace(anchor_draw, draw_dbg, 1)
    print('drawChar: applied')
else:
    raise SystemExit('drawChar anchor NOT FOUND')

# 3) clickNewPage 增量上抛钩子
anchor_cnp = (
    'bool ONScripter::clickNewPage( char *out_text )\n'
    '{\n'
    '    if ( out_text ){\n'
)
cnp_hook = (
    'bool ONScripter::clickNewPage( char *out_text )\n'
    '{\n'
    '    // [ONS-BRIDGE] 页末增量上抛（NOeSIS 等游戏的页等待走此路径而非 clickWait）\n'
    '    {\n'
    '        static int s_emitted_cnp = 0;\n'
    '        int total = current_page->text_count;\n'
    '        if (total < s_emitted_cnp) s_emitted_cnp = 0; // newPage\n'
    '        if (total > s_emitted_cnp) {\n'
    '            std::string b64 = onsbridge::B64( current_page->text + s_emitted_cnp, total - s_emitted_cnp );\n'
    '            if (!b64.empty()) onsbridge::Emit( "dialogue", "{\\"b64\\":\\"" + b64 + "\\"}" );\n'
    '            s_emitted_cnp = total;\n'
    '        }\n'
    '    }\n'
    '    if ( out_text ){\n'
)
if 's_emitted_cnp' in src:
    print('clickNewPage: already applied')
elif anchor_cnp in src:
    src = src.replace(anchor_cnp, cnp_hook, 1)
    print('clickNewPage: applied')
else:
    raise SystemExit('clickNewPage anchor NOT FOUND')

open(TEXT_CPP, 'w', encoding='utf-8', newline='\n').write(src)
print('text.cpp saved (LF)')
