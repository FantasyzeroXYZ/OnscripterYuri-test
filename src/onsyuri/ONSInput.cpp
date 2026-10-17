/**
 * ONSInput.cpp - input-mode dispatcher + weak platform fallbacks.
 *
 * The core calls ons_host_request_input().  Depending on the mode selected by
 * the host (--input-mode:host / --input-mode:overlay) it dispatches to:
 *
 *   host    -> ons_input_host_request()   (Android JNI dialog / web prompt)
 *   overlay -> ons_input_overlay_request() (kernel-drawn SDL input box)
 *
 * Platform builds provide strong implementations; weak defaults here keep
 * headless/desktop builds linkable and preserve the script default value.
 */
#include "ONSInput.h"

#if defined(__GNUC__)
#define ONS_INPUT_WEAK __attribute__((weak))
#else
#define ONS_INPUT_WEAK
#endif

static int g_input_mode = ONS_INPUT_MODE_HOST;

extern "C" void ons_set_input_mode(int mode)
{
    g_input_mode = (mode == ONS_INPUT_MODE_OVERLAY) ? ONS_INPUT_MODE_OVERLAY
                                                    : ONS_INPUT_MODE_HOST;
}

extern "C" int ons_get_input_mode(void)
{
    return g_input_mode;
}

extern "C" ONS_INPUT_WEAK int ons_input_host_request(const char* prompt,
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

extern "C" ONS_INPUT_WEAK int ons_input_overlay_request(const char* prompt,
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
    return -1;
}

extern "C" int ons_host_request_input(const char* prompt,
                                      const char* initial,
                                      int maxlen,
                                      int widechar,
                                      char* out,
                                      int out_size)
{
    if (g_input_mode == ONS_INPUT_MODE_OVERLAY) {
        int rc = ons_input_overlay_request(prompt, initial, maxlen, widechar,
                                           out, out_size);
        // -1 means the kernel overlay is unavailable; fall back to the host
        // dialog so a broken renderer never freezes the game.
        if (rc >= 0) return rc;
    }
    return ons_input_host_request(prompt, initial, maxlen, widechar,
                                  out, out_size);
}
