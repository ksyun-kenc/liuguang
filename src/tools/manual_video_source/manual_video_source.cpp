/*
 * Copyright 2020-present Ksyun
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// C
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

// C++, SDL
#include <string>

// C++, ATL
#include <atlbase.h>
#include <atlpath.h>
#include <atlstr.h>

// C++, Boost
#include <boost/scope_exit.hpp>

#include "../sdl_hack/sdl_hack.h"

// UMU
#include "umu/env.h"

#pragma comment(lib, "sdl2.lib")
#pragma comment(lib, "sdl2main.lib")
#pragma comment(lib, "SDL2_ttf.lib")

#pragma comment(lib, "yuv.lib")

TTF_Font* font = nullptr;
SDL_Window* sdl_win = nullptr;
SDL_Renderer* renderer = nullptr;
int render_driver = -1;

static SdlHack sdl_hack;

static void refresh(SDL_KeyboardEvent key) {
  char text[32];
  snprintf(text, 31, "%c", key.keysym.sym);

  SDL_Rect rect = {50, 50, 120, 200};
  SDL_Color color = {255, 0, 0, 128};
  SDL_Surface* surface = TTF_RenderText_Blended(font, text, color);
  SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
  if (nullptr == texture) {
    SDL_DestroyRenderer(renderer);
    renderer = SDL_GetRenderer(sdl_win);
    texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (nullptr == texture) {
      SDL_FreeSurface(surface);
      return;
    }
  }
  SDL_RenderClear(renderer);
  SDL_RenderCopy(renderer, texture, nullptr, &rect);
  SDL_DestroyTexture(texture);
  SDL_FreeSurface(surface);

  snprintf(text, 31, "@ %u", key.timestamp);

  rect = {200, 120, 360, 100};
  color = {255, 255, 255, 128};
  surface = TTF_RenderText_Blended(font, text, color);
  texture = SDL_CreateTextureFromSurface(renderer, surface);
  SDL_RenderCopy(renderer, texture, nullptr, &rect);
  SDL_DestroyTexture(texture);
  SDL_FreeSurface(surface);

  if (sdl_hack.IsStarted() && -1 != render_driver) {
    sdl_hack.GetTexture(renderer);
  }

  SDL_RenderPresent(renderer);
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                      _In_opt_ HINSTANCE hPrevInstance,
                      _In_ LPWSTR lpCmdLine,
                      _In_ int nCmdShow) {
  if (!sdl_hack.Init()) {
    return -1;
  }

  if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
    return -1;
  }
  BOOST_SCOPE_EXIT_ALL(&) { SDL_Quit(); };

  if (TTF_Init() == -1) {
    CString error_text;
    error_text.Format(_T("TTF_Init() failed: %s\n"),
                      CA2T(TTF_GetError()).m_psz);
    MessageBox(nullptr, error_text, nullptr, MB_ICONERROR);
    return -1;
  }
  BOOST_SCOPE_EXIT_ALL(&) { TTF_Quit(); };

  CPath windir;
  font = TTF_OpenFont(
      umu::env::ExpandEnvironmentStringA("%windir%\\Fonts\\simhei.ttf").data(),
      80);
  if (nullptr == font) {
    CString error_text;
    error_text.Format(_T("TTF_OpenFont(simhei.ttf) failed: %s\n"),
                      CA2T(TTF_GetError()).m_psz);
    MessageBox(nullptr, error_text, nullptr, MB_ICONERROR);
    return -1;
  }
  BOOST_SCOPE_EXIT_ALL(&) { TTF_CloseFont(font); };

  CStringA title("SDL Text - ");
  sdl_win = SDL_CreateWindow(title, 400, 200, 640, 300, 0);
  if (nullptr == sdl_win) {
    CString error_text;
    error_text.Format(_T("SDL_CreateWindow() failed: %s\n"),
                      CA2T(TTF_GetError()).m_psz);
    MessageBox(nullptr, error_text, nullptr, MB_ICONERROR);
    return -1;
  }
  BOOST_SCOPE_EXIT_ALL(&) { SDL_DestroyWindow(sdl_win); };

  int num = SDL_GetNumRenderDrivers();
  for (int i = 0; i < num; ++i) {
    SDL_RendererInfo info = {};
    if (0 == SDL_GetRenderDriverInfo(i, &info)) {
      if (CStringA("direct3d11") == info.name) {
        render_driver = i;
        title.Append(info.name);
        break;
      }
    }
  }

  renderer = SDL_CreateRenderer(sdl_win, render_driver, 0);
  if (nullptr == renderer) {
    CString error_text;
    error_text.Format(_T("SDL_CreateRenderer() failed: %s\n"),
                      CA2T(SDL_GetError()).m_psz);
    MessageBox(nullptr, error_text, nullptr, MB_ICONERROR);
    return -1;
  }
  BOOST_SCOPE_EXIT_ALL(&) { SDL_DestroyRenderer(renderer); };

  if (-1 == render_driver) {
    SDL_RendererInfo info = {};
    SDL_GetRendererInfo(renderer, &info);
    title.Append(info.name);
  }
  SDL_SetWindowTitle(sdl_win, title);

  SDL_RenderClear(renderer);
  SDL_RenderPresent(renderer);

  for (;;) {
    SDL_Event sdl_event;
    if (SDL_PollEvent(&sdl_event)) {
      if (SDL_QUIT == sdl_event.type) {
        break;
      } else if (SDL_KEYDOWN == sdl_event.type) {
        if (SDLK_ESCAPE == sdl_event.key.keysym.sym) {
          break;
        }
        refresh(sdl_event.key);
      }
    }
  }
  return 0;
}