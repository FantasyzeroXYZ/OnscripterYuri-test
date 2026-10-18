/**
 * ons_input_web.cpp - Emscripten text-input implementation.
 *
 * First version uses window.prompt(), which is synchronous and needs no
 * Asyncify.  A future version can replace the body with a DOM overlay or the
 * in-engine SDL text input; the ONSInput.h contract stays the same.
 */
#include "ONSInput.h"

#if defined(__EMSCRIPTEN__)

#include <emscripten.h>
#include <cstring>
#include <string>

namespace {

std::string jsQuote(const char* s)
{
    std::string out = "'";
    if (s) {
        for (const char* p = s; *p; ++p) {
            unsigned char c = (unsigned char)*p;
            switch (c) {
                case '\\': out += "\\\\"; break;
                case '\'': out += "\\'"; break;
                case '\n': out += "\\n";  break;
                case '\r': out += "\\r";  break;
                case '\t': out += "\\t";  break;
                default:   out += (char)c; break;
            }
        }
    }
    out += "'";
    return out;
}

} // namespace

extern "C" int ons_host_request_input(const char* prompt,
                                      const char* initial,
                                      int maxlen,
                                      int widechar,
                                      char* out,
                                      int out_size)
{
    (void)maxlen;
    (void)widechar;
    if (!out || out_size <= 0) return 0;
    out[0] = '\0';

    // \u0001 is the cancel sentinel; prompt() returns null on cancel.
    std::string js = "(function(){var r=prompt(" + jsQuote(prompt) + "," +
                     jsQuote(initial) + ");return r===null?'\\u0001':r;})()";
    const char* result = emscripten_run_script_string(js.c_str());
    if (!result || result[0] == '\0' || result[0] == '\1') return 0;

    std::strncpy(out, result, (size_t)out_size - 1);
    out[out_size - 1] = '\0';
    return 1;
}

#endif // __EMSCRIPTEN__
