# fix-json-escape.py — 重写 JsonEscape 函数体（用数值比较替代字符转义，避免多层转义歧义）
P = r'D:\Desktop\OnscripterYuri-master\src\onsyuri\ONSBridge.h'
data = open(P, 'rb').read()

# 找 JsonEscape 函数体的起止
start_marker = b'inline std::string JsonEscape'
end_marker = b'inline void Emit'
i = data.find(start_marker)
j = data.find(end_marker)
assert i >= 0 and j > i, f'JsonEscape not found i={i} j={j}'

new_fn = (
    b'inline std::string JsonEscape(const char* s, int len) {\n'
    b'    std::string esc;\n'
    b'    for (int i = 0; i < len; i++) {\n'
    b'        unsigned char c = (unsigned char)s[i];\n'
    b'        // JSON escape: drop internal control codes (<0x20 except \\n \\r \\t),\n'
    b'        // escape quote/backslash, pass UTF-8 multibyte\n'
    b'        if (c == 0x22) { esc += 0x5C; esc += c; }           // " -> \\"\n'
    b'        else if (c == 0x5C) { esc += 0x5C; esc += c; }      // \\\\ -> \\\\\\\\\n'
    b'        else if (c == 0x0A) { esc += 0x5C; esc += 0x6E; }   // \\n -> \\n\n'
    b'        else if (c == 0x0D) { esc += 0x5C; esc += 0x72; }   // \\r -> \\r\n'
    b'        else if (c == 0x09) { esc += 0x5C; esc += 0x74; }   // \\t -> \\t\n'
    b'        else if (c < 0x20) { /* drop internal ctrl */ }\n'
    b'        else { esc += c; }                                   // UTF-8 pass\n'
    b'    }\n'
    b'    return esc;\n'
    b'}\n'
    b'\n'
)

src = data[:i] + new_fn + data[j:]
open(P, 'wb').write(src)
print('JsonEscape rewritten (numeric comparisons)')
