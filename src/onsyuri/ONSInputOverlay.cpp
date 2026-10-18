/**
 * ONSInputOverlay.cpp - kernel-drawn SDL input box.
 *
 * This is the "overlay" implementation of ons_input_overlay_request().  It is
 * intentionally SDL-based so it works on Android / web / desktop builds that
 * already link SDL2.  Libretro cores (no SDL) can provide their own
 * implementation later behind the same ONSInput.h contract.
 */
#include "ONScripter.h"
#include "ONSInput.h"

#include <SDL_ttf.h>

#if defined(ANDROID)
#include <android/log.h>
#endif

#include <algorithm>
#include <cmath>
#include <cstring>
#include <string>
#include <vector>

namespace {

int utf8Count(const std::string& s)
{
    int n = 0;
    for (size_t i = 0; i < s.size(); ) {
        unsigned char c = (unsigned char)s[i];
        size_t step = (c < 0x80) ? 1 : ((c & 0xE0) == 0xC0 ? 2
                     : ((c & 0xF0) == 0xE0 ? 3 : 4));
        if (i + step > s.size()) break;
        i += step;
        n++;
    }
    return n;
}

void popLastUtf8(std::string& s)
{
    if (s.empty()) return;
    s.pop_back();
    while (!s.empty() && (((unsigned char)s.back()) & 0xC0) == 0x80) {
        s.pop_back();
    }
}

} // namespace

void ONScripter::presentOverlaySurface(SDL_Surface* surface)
{
    if (!surface || !renderer || !texture) return;

    SDL_LockSurface(surface);
    SDL_UpdateTexture(texture, NULL, surface->pixels, surface->pitch);
    SDL_UnlockSurface(surface);

    SDL_RenderClear(renderer);
    const bool stretch = (stretch_mode && fullscreen_mode) ||
                         (force_window_height && force_window_width);
#if defined(USE_GLES)
    if (std::isnan(sharpness)) {
        if (stretch) SDL_RenderCopy(renderer, texture, NULL, NULL);
        else SDL_RenderCopy(renderer, texture, NULL, &render_view_rect);
    } else if (gles_renderer) {
        gles_renderer->copy(render_view_rect.x, render_view_rect.y);
    }
#else
    if (stretch) SDL_RenderCopy(renderer, texture, NULL, NULL);
    else SDL_RenderCopy(renderer, texture, NULL, &render_view_rect);
#endif
    SDL_RenderPresent(renderer);
}

int ONScripter::showInputOverlay(const char* prompt,
                                 const char* initial,
                                 int maxlen,
                                 char* out,
                                 int out_size)
{
    if (!renderer || !accumulation_surface || !out || out_size <= 0) return -1;
    out[0] = '\0';
#if defined(ANDROID)
    __android_log_print(ANDROID_LOG_INFO, "ONS-INPUT",
                        "overlay start renderer=%p surface=%dx%d",
                        (void*)renderer, accumulation_surface->w,
                        accumulation_surface->h);
#endif

    std::string text = initial ? initial : "";
    if (maxlen > 0 && utf8Count(text) > maxlen) text.clear();

    SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
    SDL_StartTextInput();

    if (!TTF_WasInit()) TTF_Init();
    TTF_Font* promptFont = font_file ? TTF_OpenFont(font_file, 24) : nullptr;
    TTF_Font* inputFont = font_file ? TTF_OpenFont(font_file, 32) : nullptr;
#if defined(ANDROID)
    __android_log_print(ANDROID_LOG_INFO, "ONS-INPUT",
                        "overlay font=%s prompt=%p input=%p err=%s",
                        font_file ? font_file : "(null)",
                        (void*)promptFont, (void*)inputFont, TTF_GetError());
#endif

    bool confirmed = false;
    bool cancelled = false;

    while (!confirmed && !cancelled) {
        // Draw directly onto the engine accumulation surface and present it
        // through the same texture path as normal frames.  flush() after the
        // loop rebuilds the game frame from bg/sprites/text.
        SDL_Surface* overlay = accumulation_surface;

        int sw = overlay->w;
        int sh = overlay->h;
        int boxW = std::max(320, sw * 4 / 5);
        int boxH = std::max(150, sh / 3);
        SDL_Rect box = { (sw - boxW) / 2, (sh - boxH) / 2, boxW, boxH };

        SDL_SetSurfaceBlendMode(overlay, SDL_BLENDMODE_NONE);
        SDL_FillRect(overlay, &box, SDL_MapRGB(overlay->format, 20, 20, 20));

        SDL_Color white = { 255, 255, 255, 255 };
        SDL_Surface* promptSurf = promptFont
                ? TTF_RenderUTF8_Blended_Wrapped(promptFont,
                        (prompt ? prompt : ""), white, boxW - 40)
                : nullptr;
        if (promptSurf) {
            SDL_Rect dst = { box.x + 20, box.y + 18, 0, 0 };
            SDL_SetSurfaceBlendMode(overlay, SDL_BLENDMODE_BLEND);
            SDL_BlitSurface(promptSurf, NULL, overlay, &dst);
            SDL_FreeSurface(promptSurf);
        }

        std::string shown = text + "_";
        SDL_Surface* inputSurf = inputFont
                ? TTF_RenderUTF8_Blended(inputFont, shown.c_str(), white)
                : nullptr;
        if (inputSurf) {
            SDL_Rect dst = { box.x + 20,
                             box.y + boxH - inputSurf->h - 18, 0, 0 };
            SDL_SetSurfaceBlendMode(overlay, SDL_BLENDMODE_BLEND);
            SDL_BlitSurface(inputSurf, NULL, overlay, &dst);
            SDL_FreeSurface(inputSurf);
        }

        presentOverlaySurface(overlay);

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                cancelled = true;
                break;
            }
            if (event.type == SDL_TEXTINPUT) {
                std::string add = event.text.text;
                if (maxlen <= 0 || utf8Count(text + add) <= maxlen) {
                    text += add;
                }
            } else if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_BACKSPACE:
                        popLastUtf8(text);
                        break;
                    case SDLK_RETURN:
                    case SDLK_KP_ENTER:
                        confirmed = true;
                        break;
                    case SDLK_ESCAPE:
                        cancelled = true;
                        break;
                    default:
                        break;
                }
            }
        }
        SDL_Delay(16);
    }

    if (promptFont) TTF_CloseFont(promptFont);
    if (inputFont) TTF_CloseFont(inputFont);
    SDL_StopTextInput();
    flush(REFRESH_NORMAL_MODE);

#if defined(ANDROID)
    __android_log_print(ANDROID_LOG_INFO, "ONS-INPUT",
                        "overlay end confirmed=%d cancelled=%d len=%zu",
                        confirmed ? 1 : 0, cancelled ? 1 : 0, text.size());
#endif
    if (!confirmed || cancelled) return 0;
    std::strncpy(out, text.c_str(), (size_t)out_size - 1);
    out[out_size - 1] = '\0';
    return 1;
}

