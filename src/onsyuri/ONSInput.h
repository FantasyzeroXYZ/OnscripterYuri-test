/**
 * ONSInput.h - platform text-input hook for the NScripter `input` command.
 *
 * The core never renders an input box itself.  Each platform build provides
 * exactly one implementation of ons_host_request_input():
 *
 *   Android : onscripter_main.cpp  -> JNI -> ONScripter.java dialog
 *   Web     : ons_input_web.cpp    -> window.prompt / DOM overlay
 *   Desktop : ONSInput.cpp         -> default (keep script value); replace
 *                                     with SDL/native dialog later
 *
 * Contract:
 *   - prompt / initial / out are UTF-8, NUL-terminated.
 *   - The call is synchronous: return only after the user confirms/cancels.
 *   - Return 1 on confirm, 0 on cancel/timeout/unsupported.
 *   - On success, out must contain a NUL-terminated UTF-8 string.
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

int ons_host_request_input(const char* prompt,
                           const char* initial,
                           int maxlen,
                           int widechar,
                           char* out,
                           int out_size);

#ifdef __cplusplus
}
#endif
