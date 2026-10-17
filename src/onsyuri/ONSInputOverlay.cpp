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
#include "DirectReader.h"

#if defined(ANDROID)
#include <android/log.h>
#endif

#include <algorithm>
#include <cmath>
#include <cstring>
#include <string>
#include <vector>

extern ONScripter ons;

namespace {

std::string utf8ToCoding(const std::string& s)
{
    if (s.empty()) return std::string();
    std::vector<char> buf(s.size() * 2 + 2);
    DirectReader::convertFromUTF8ToCoding(buf.data(), s.c_str());
    return std::string(buf.data());
}

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

        FontInfo f = sentence_font;
        uchar3 white = { 255, 255, 255 };
        f.xy[0] = box.x + 20;
        f.xy[1] = box.y + 18;
        f.num_xy[0] = std::max(8, (boxW - 40) /
                              std::max(1, f.font_size_xy[0]));
        std::string promptCoded = utf8ToCoding(prompt ? prompt : "");
        if (!promptCoded.empty()) {
            drawString(promptCoded.c_str(), white, &f, false, overlay);
        }

        FontInfo fi = sentence_font;
        fi.xy[0] = box.x + 20;
        fi.xy[1] = box.y + boxH - fi.font_size_xy[1] - 22;
        fi.num_xy[0] = std::max(8, (boxW - 40) /
                               std::max(1, fi.font_size_xy[0]));
        std::string shown = utf8ToCoding(text) + "_";
        drawString(shown.c_str(), white, &fi, false, overlay);

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

extern "C" int ons_input_overlay_request(const char* prompt,
                                         const char* initial,
                                         int maxlen,
                                         int widechar,
                                         char* out,
                                         int out_size)
{
    (void)widechar;
    return ons.showInputOverlay(prompt, initial, maxlen, out, out_size);
}
