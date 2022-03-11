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
#include <boost/program_options.hpp>
#include <boost/scope_exit.hpp>

#include "sdl_hack/sdl_hack.h"

// UMU
#include "umu/env.h"

#pragma comment(lib, "sdl2.lib")
#pragma comment(lib, "sdl2main.lib")
#pragma comment(lib, "SDL2_ttf.lib")

#pragma comment(lib, "yuv.lib")

namespace po = boost::program_options;

namespace {

using namespace regame;

bool global_mode = false;
VideoFrameType frame_type = VideoFrameType::kYuv;

TTF_Font* font = nullptr;
SDL_Window* sdl_win = nullptr;
SDL_Renderer* renderer = nullptr;
int render_driver = -1;

void Refresh(SdlHack& sdl_hack, SDL_KeyboardEvent key) {
  char text[32];
  snprintf(text, std::size(text) - 1, "%c", key.keysym.sym);

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

  snprintf(text, std::size(text) - 1, "@ %u", key.timestamp);

  rect = {200, 120, 360, 100};
  color = {255, 255, 255, 128};
  surface = TTF_RenderText_Blended(font, text, color);
  texture = SDL_CreateTextureFromSurface(renderer, surface);
  SDL_RenderCopy(renderer, texture, nullptr, &rect);
  SDL_DestroyTexture(texture);
  SDL_FreeSurface(surface);

  if (sdl_hack.IsStarted() && -1 != render_driver) {
    sdl_hack.CopyTexture(renderer);
  }

  SDL_RenderPresent(renderer);
}

void Mouse(SdlHack& sdl_hack, std::string_view message, int x, int y) {
  SDL_Rect rect = {30, 20, 150, 100};
  SDL_Color color = {255, 0, 0, 128};
  SDL_Surface* surface = TTF_RenderText_Blended(font, message.data(), color);
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

  char text[40];
  snprintf(text, std::size(text) - 1, "%d,%d", x, y);

  rect = {190, 20, 420, 100};
  color = {255, 255, 255, 128};
  surface = TTF_RenderText_Blended(font, text, color);
  texture = SDL_CreateTextureFromSurface(renderer, surface);
  SDL_RenderCopy(renderer, texture, nullptr, &rect);
  SDL_DestroyTexture(texture);
  SDL_FreeSurface(surface);

  if (sdl_hack.IsStarted() && -1 != render_driver) {
    sdl_hack.CopyTexture(renderer);
  }

  SDL_RenderPresent(renderer);
}

bool ParseCommandLine(_In_ LPWSTR command_line) {
  std::string type;
  po::options_description desc("Usage");
  desc.add_options()("help,h", "Produce help message")(
      "global-mode", po::value<bool>(&global_mode)->default_value(false),
      "Set global mode")("frame-type",
                         po::value<std::string>(&type)->default_value("yuv"),
                         "Set video frame type, can be one of {yuv, tex}");
  po::variables_map vm;
  auto parser = po::wcommand_line_parser(po::split_winmain(command_line));
  parser.options(desc);
  po::store(parser.run(), vm);
  po::notify(vm);

  if (vm.count("help")) {
    std::stringstream ss;
    ss << desc;
    MessageBox(nullptr, CA2T(ss.str().data()).m_psz, L"Help",
               MB_ICONINFORMATION);
    return false;
  }

  if (0 == type.compare("tex")) {
    frame_type = VideoFrameType::kTexture;
  }

  return true;
}

}  // namespace

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                      _In_opt_ HINSTANCE hPrevInstance,
                      _In_ LPWSTR command_line,
                      _In_ int nCmdShow) {
  if (!ParseCommandLine(command_line)) {
    return 0;
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

  SdlHack sdl_hack;
  if (!sdl_hack.Initialize()) {
    return -1;
  }
  if (!sdl_hack.Run(global_mode, frame_type)) {
    return -1;
  }

  CStringA title("SDL Text - ");
  sdl_win = SDL_CreateWindow(title, 400, 200, 640, 300, SDL_WINDOW_RESIZABLE);
  if (nullptr == sdl_win) {
    CString error_text;
    error_text.Format(_T("SDL_CreateWindow() failed: %s\n"),
                      CA2T(TTF_GetError()).m_psz);
    MessageBox(nullptr, error_text, nullptr, MB_ICONERROR);
    return -1;
  }
  BOOST_SCOPE_EXIT_ALL(&) { SDL_DestroyWindow(sdl_win); };

  SDL_version linked;
  SDL_GetVersion(&linked);
  title.AppendFormat("SDL %u.%u.%u, ", linked.major, linked.minor,
                     linked.patch);

  int num = SDL_GetNumRenderDrivers();
  for (int i = 0; i < num; ++i) {
    SDL_RendererInfo info{};
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
    MessageBox(nullptr,
               _T("Only support direct3d11.\r\n")
               _T("Please compile SDL2 with SDL_VIDEO_RENDER_D3D11=1."),
               nullptr, MB_ICONERROR);
  }

  if (global_mode) {
    title.Append(", global");
  } else {
    title.Append(", local");
  }

  if (VideoFrameType::kTexture == frame_type) {
    title.Append(", tex");
  } else {
    title.Append(", yuv");
  }

  SDL_SetWindowTitle(sdl_win, title);

  SDL_RenderClear(renderer);
  SDL_RenderPresent(renderer);

  for (;;) {
    SDL_Event sdl_event;
    if (SDL_PollEvent(&sdl_event)) {
      switch (sdl_event.type) {
        case SDL_QUIT:
          sdl_hack.Free();
          return 0;
        case SDL_KEYDOWN:
          Refresh(sdl_hack, sdl_event.key);
          break;
        case SDL_MOUSEMOTION:
          Mouse(sdl_hack, "Motion", sdl_event.motion.xrel,
                sdl_event.motion.yrel);
          break;
        case SDL_MOUSEWHEEL:
          Mouse(sdl_hack, "Wheel", sdl_event.wheel.x, sdl_event.wheel.y);
          break;
        case SDL_MOUSEBUTTONDOWN:
          Mouse(sdl_hack, "Down", sdl_event.button.x, sdl_event.button.y);
          break;
        case SDL_MOUSEBUTTONUP:
          Mouse(sdl_hack, "Up", sdl_event.button.x, sdl_event.button.y);
          break;
      }
    }
  }
  return 0;
}
