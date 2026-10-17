/**
 * ONSInput.cpp - default (headless/desktop) implementation.
 *
 * Returning 0 makes the core keep the script-provided default value, which is
 * the historical behavior.  Desktop builds can replace this file with a
 * SDL/native-dialog implementation later; the interface stays unchanged.
 */
#include "ONSInput.h"

// Weak default: platform builds (Android onscripter_main.cpp / Web
// ons_input_web.cpp) provide a strong implementation that overrides this one.
#if defined(__GNUC__)
#define ONS_INPUT_WEAK __attribute__((weak))
#else
#define ONS_INPUT_WEAK
#endif

extern "C" ONS_INPUT_WEAK int ons_host_request_input(const char* prompt,
                                      const char* initial,
                                      int maxlen,
                                      int widechar,
                                      char* out,
                                      int out_size)
{
    (void)prompt;
    (void)initial;
    (void)maxlen;
    (void)widechar;
    (void)out;
    (void)out_size;
    return 0;
}
