/**
 * ONSBridge.h - [ONS-BRIDGE] dialogue/voice extraction bridge
 * ============================================================================
 * Header-only utility functions. Injection function implementations are in
 * ONScripter_text.cpp. All bridge traffic goes through the 150ms rate gate.
 */
#pragma once

#include <string>
#include <emscripten.h>

extern "C" {
void ons_inject_move(int x, int y);
void ons_inject_click(int x, int y);
void ons_inject_rclick(int x, int y);
void ons_inject_key(int sym, int mod);
}

namespace onsbridge {

// JSON string escape: drop internal control codes (<0x20 except \n\r\t),
// escape quotes/backslashes, pass through UTF-8 multibyte
inline std::string JsonEscape(const char* s, int len) {
    std::string esc;
    for (int i = 0; i < len; i++) {
        unsigned char c = (unsigned char)s[i];
        // JSON escape: drop internal control codes (<0x20 except \n \r \t),
        // escape quote/backslash, pass UTF-8 multibyte
        if (c == 0x22) { esc += 0x5C; esc += c; }           // " -> \"
        else if (c == 0x5C) { esc += 0x5C; esc += c; }      // \\ -> \\\\
        else if (c == 0x0A) { esc += 0x5C; esc += 0x6E; }   // \n -> \n
        else if (c == 0x0D) { esc += 0x5C; esc += 0x72; }   // \r -> \r
        else if (c == 0x09) { esc += 0x5C; esc += 0x74; }   // \t -> \t
        else if (c < 0x20) { /* drop internal ctrl */ }
        else { esc += c; }                                   // UTF-8 pass
    }
    return esc;
}

inline void Emit(const char* type, const std::string& payload) {
    std::string json = std::string("{\"type\":\"") + type + "\",\"payload\":" + payload + "}";
    EM_ASM({
        try {
            if (window.OnsExtract && window.OnsExtract.event) {
                const obj = JSON.parse(UTF8ToString($0));
                window.OnsExtract.event(obj.type, obj.payload);
            }
        } catch (e) { console.warn('[ons-bridge]', e && e.message); }
    }, json.c_str());
}

// base64 encode raw bytes (script encoding may be SJIS/GBK - UTF8ToString
// would mangle non-UTF-8 text; host decodes with TextDecoder sniffing)
inline std::string B64(const char* s, int len) {
    static const char* TBL = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    out.reserve(((len + 2) / 3) * 4);
    for (int i = 0; i < len; i += 3) {
        int b0 = (unsigned char)s[i];
        int b1 = (i + 1 < len) ? (unsigned char)s[i + 1] : 0;
        int b2 = (i + 2 < len) ? (unsigned char)s[i + 2] : 0;
        out += TBL[b0 >> 2];
        out += TBL[((b0 & 3) << 4) | (b1 >> 4)];
        out += (i + 1 < len) ? TBL[((b1 & 15) << 2) | (b2 >> 6)] : '=';
        out += (i + 2 < len) ? TBL[b2 & 63] : '=';
    }
    return out;
}

} // namespace onsbridge
