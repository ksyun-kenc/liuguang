/*
SDL 2.0 and newer are available under the zlib license :

This software is provided 'as-is', without any express or implied
warranty.  In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/

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

// Note: Only for SDL 2.0.14

#pragma once

#ifdef _WIN32

#define COBJMACROS
#include <d3d11.h>
#include <d3d11_1.h>

#ifdef _M_X64
/* Use 8-byte alignment on 64-bit architectures, so pointers are aligned */
#pragma pack(push, 8)
#else
#pragma pack(push, 4)
#endif

typedef struct {
  float x;
  float y;
} Float2;

typedef struct {
  float x;
  float y;
  float z;
} Float3;

typedef struct {
  float x;
  float y;
  float z;
  float w;
} Float4;

typedef struct {
  union {
    struct {
      float _11, _12, _13, _14;
      float _21, _22, _23, _24;
      float _31, _32, _33, _34;
      float _41, _42, _43, _44;
    } v;
    float m[4][4];
  };
} Float4X4;

/* Vertex shader, common values */
typedef struct {
  Float4X4 model;
  Float4X4 projectionAndView;
} VertexShaderConstants;

/* D3D11 shader implementation */

typedef enum {
  SHADER_SOLID,
  SHADER_RGB,
  SHADER_YUV_JPEG,
  SHADER_YUV_BT601,
  SHADER_YUV_BT709,
  SHADER_NV12_JPEG,
  SHADER_NV12_BT601,
  SHADER_NV12_BT709,
  SHADER_NV21_JPEG,
  SHADER_NV21_BT601,
  SHADER_NV21_BT709,
  NUM_SHADERS
} D3D11_Shader;

/* Blend mode data */
typedef struct {
  SDL_BlendMode blendMode;
  ID3D11BlendState* blendState;
} D3D11_BlendMode;

/* Private renderer data */
typedef struct {
  void* hDXGIMod;
  void* hD3D11Mod;
  IDXGIFactory2* dxgiFactory;
  IDXGIAdapter* dxgiAdapter;
  ID3D11Device1* d3dDevice;
  ID3D11DeviceContext1* d3dContext;
  IDXGISwapChain1* swapChain;
  DXGI_SWAP_EFFECT swapEffect;
  ID3D11RenderTargetView* mainRenderTargetView;
  ID3D11RenderTargetView* currentOffscreenRenderTargetView;
  ID3D11InputLayout* inputLayout;
  ID3D11Buffer* vertexBuffers[8];
  size_t vertexBufferSizes[8];
  ID3D11VertexShader* vertexShader;
  ID3D11PixelShader* pixelShaders[NUM_SHADERS];
  int blendModesCount;
  D3D11_BlendMode* blendModes;
  ID3D11SamplerState* nearestPixelSampler;
  ID3D11SamplerState* linearSampler;
  D3D_FEATURE_LEVEL featureLevel;

  /* Rasterizers */
  ID3D11RasterizerState* mainRasterizer;
  ID3D11RasterizerState* clippedRasterizer;

  /* Vertex buffer constants */
  VertexShaderConstants vertexShaderConstantsData;
  ID3D11Buffer* vertexShaderConstants;

  /* Cached renderer properties */
  DXGI_MODE_ROTATION rotation;
  ID3D11RenderTargetView* currentRenderTargetView;
  ID3D11RasterizerState* currentRasterizerState;
  ID3D11BlendState* currentBlendState;
  ID3D11PixelShader* currentShader;
  ID3D11ShaderResourceView* currentShaderResource;
  ID3D11SamplerState* currentSampler;
  SDL_bool cliprectDirty;
  SDL_bool currentCliprectEnabled;
  SDL_Rect currentCliprect;
  SDL_Rect currentViewport;
  int currentViewportRotation;
  SDL_bool viewportDirty;
  Float4X4 identity;
  int currentVertexBuffer;
} D3D11_RenderData;

#pragma pack(pop)
#endif  // _WIN32

typedef enum {
  SDL_RENDERCMD_NO_OP,
  SDL_RENDERCMD_SETVIEWPORT,
  SDL_RENDERCMD_SETCLIPRECT,
  SDL_RENDERCMD_SETDRAWCOLOR,
  SDL_RENDERCMD_CLEAR,
  SDL_RENDERCMD_DRAW_POINTS,
  SDL_RENDERCMD_DRAW_LINES,
  SDL_RENDERCMD_FILL_RECTS,
  SDL_RENDERCMD_COPY,
  SDL_RENDERCMD_COPY_EX
} SDL_RenderCommandType;

typedef struct SDL_RenderCommand {
  SDL_RenderCommandType command;
  union {
    struct {
      size_t first;
      SDL_Rect rect;
    } viewport;
    struct {
      SDL_bool enabled;
      SDL_Rect rect;
    } cliprect;
    struct {
      size_t first;
      size_t count;
      Uint8 r, g, b, a;
      SDL_BlendMode blend;
      SDL_Texture* texture;
    } draw;
    struct {
      size_t first;
      Uint8 r, g, b, a;
    } color;
  } data;
  struct SDL_RenderCommand* next;
} SDL_RenderCommand;

/* Define the SDL 2.0.12 renderer structure */
struct SDL_Renderer_2_0_12 {
  const void* magic;

  void (*WindowEvent)(SDL_Renderer* renderer, const SDL_WindowEvent* event);
  int (*GetOutputSize)(SDL_Renderer* renderer, int* w, int* h);
  SDL_bool (*SupportsBlendMode)(SDL_Renderer* renderer,
                                SDL_BlendMode blendMode);
  int (*CreateTexture)(SDL_Renderer* renderer, SDL_Texture* texture);
  int (*QueueSetViewport)(SDL_Renderer* renderer, SDL_RenderCommand* cmd);
  int (*QueueSetDrawColor)(SDL_Renderer* renderer, SDL_RenderCommand* cmd);
  int (*QueueDrawPoints)(SDL_Renderer* renderer,
                         SDL_RenderCommand* cmd,
                         const SDL_FPoint* points,
                         int count);
  int (*QueueDrawLines)(SDL_Renderer* renderer,
                        SDL_RenderCommand* cmd,
                        const SDL_FPoint* points,
                        int count);
  int (*QueueFillRects)(SDL_Renderer* renderer,
                        SDL_RenderCommand* cmd,
                        const SDL_FRect* rects,
                        int count);
  int (*QueueCopy)(SDL_Renderer* renderer,
                   SDL_RenderCommand* cmd,
                   SDL_Texture* texture,
                   const SDL_Rect* srcrect,
                   const SDL_FRect* dstrect);
  int (*QueueCopyEx)(SDL_Renderer* renderer,
                     SDL_RenderCommand* cmd,
                     SDL_Texture* texture,
                     const SDL_Rect* srcquad,
                     const SDL_FRect* dstrect,
                     const double angle,
                     const SDL_FPoint* center,
                     const SDL_RendererFlip flip);
  int (*RunCommandQueue)(SDL_Renderer* renderer,
                         SDL_RenderCommand* cmd,
                         void* vertices,
                         size_t vertsize);
  int (*UpdateTexture)(SDL_Renderer* renderer,
                       SDL_Texture* texture,
                       const SDL_Rect* rect,
                       const void* pixels,
                       int pitch);
  int (*UpdateTextureYUV)(SDL_Renderer* renderer,
                          SDL_Texture* texture,
                          const SDL_Rect* rect,
                          const Uint8* Yplane,
                          int Ypitch,
                          const Uint8* Uplane,
                          int Upitch,
                          const Uint8* Vplane,
                          int Vpitch);
  int (*LockTexture)(SDL_Renderer* renderer,
                     SDL_Texture* texture,
                     const SDL_Rect* rect,
                     void** pixels,
                     int* pitch);
  void (*UnlockTexture)(SDL_Renderer* renderer, SDL_Texture* texture);
  void (*SetTextureScaleMode)(SDL_Renderer* renderer,
                              SDL_Texture* texture,
                              SDL_ScaleMode scaleMode);
  int (*SetRenderTarget)(SDL_Renderer* renderer, SDL_Texture* texture);
  int (*RenderReadPixels)(SDL_Renderer* renderer,
                          const SDL_Rect* rect,
                          Uint32 format,
                          void* pixels,
                          int pitch);
  void (*RenderPresent)(SDL_Renderer* renderer);
  void (*DestroyTexture)(SDL_Renderer* renderer, SDL_Texture* texture);

  void (*DestroyRenderer)(SDL_Renderer* renderer);

  int (*GL_BindTexture)(SDL_Renderer* renderer,
                        SDL_Texture* texture,
                        float* texw,
                        float* texh);
  int (*GL_UnbindTexture)(SDL_Renderer* renderer, SDL_Texture* texture);

  void* (*GetMetalLayer)(SDL_Renderer* renderer);
  void* (*GetMetalCommandEncoder)(SDL_Renderer* renderer);

  /* The current renderer info */
  SDL_RendererInfo info;

  /* The window associated with the renderer */
  SDL_Window* window;
  SDL_bool hidden;

  /* The logical resolution for rendering */
  int logical_w;
  int logical_h;
  int logical_w_backup;
  int logical_h_backup;

  /* Whether or not to force the viewport to even integer intervals */
  SDL_bool integer_scale;

  /* The drawable area within the window */
  SDL_Rect viewport;
  SDL_Rect viewport_backup;

  /* The clip rectangle within the window */
  SDL_Rect clip_rect;
  SDL_Rect clip_rect_backup;

  /* Wether or not the clipping rectangle is used. */
  SDL_bool clipping_enabled;
  SDL_bool clipping_enabled_backup;

  /* The render output coordinate scale */
  SDL_FPoint scale;
  SDL_FPoint scale_backup;

  /* The pixel to point coordinate scale */
  SDL_FPoint dpi_scale;

  /* The list of textures */
  SDL_Texture* textures;
  SDL_Texture* target;
  SDL_mutex* target_mutex;

  Uint8 r, g, b, a;        /**< Color for drawing operations values */
  SDL_BlendMode blendMode; /**< The drawing blend mode */

  SDL_bool always_batch;
  SDL_bool batching;
  SDL_RenderCommand* render_commands;
  SDL_RenderCommand* render_commands_tail;
  SDL_RenderCommand* render_commands_pool;
  Uint32 render_command_generation;
  Uint32 last_queued_color;
  SDL_Rect last_queued_viewport;
  SDL_Rect last_queued_cliprect;
  SDL_bool last_queued_cliprect_enabled;
  SDL_bool color_queued;
  SDL_bool viewport_queued;
  SDL_bool cliprect_queued;

  void* vertex_data;
  size_t vertex_data_used;
  size_t vertex_data_allocation;

  void* driverdata;
};

/* Define the SDL 2.0.14 renderer structure */
struct SDL_Renderer_2_0_14 {
  const void* magic;

  void (*WindowEvent)(SDL_Renderer* renderer, const SDL_WindowEvent* event);
  int (*GetOutputSize)(SDL_Renderer* renderer, int* w, int* h);
  SDL_bool (*SupportsBlendMode)(SDL_Renderer* renderer,
                                SDL_BlendMode blendMode);
  int (*CreateTexture)(SDL_Renderer* renderer, SDL_Texture* texture);
  int (*QueueSetViewport)(SDL_Renderer* renderer, SDL_RenderCommand* cmd);
  int (*QueueSetDrawColor)(SDL_Renderer* renderer, SDL_RenderCommand* cmd);
  int (*QueueDrawPoints)(SDL_Renderer* renderer,
                         SDL_RenderCommand* cmd,
                         const SDL_FPoint* points,
                         int count);
  int (*QueueDrawLines)(SDL_Renderer* renderer,
                        SDL_RenderCommand* cmd,
                        const SDL_FPoint* points,
                        int count);
  int (*QueueFillRects)(SDL_Renderer* renderer,
                        SDL_RenderCommand* cmd,
                        const SDL_FRect* rects,
                        int count);
  int (*QueueCopy)(SDL_Renderer* renderer,
                   SDL_RenderCommand* cmd,
                   SDL_Texture* texture,
                   const SDL_Rect* srcrect,
                   const SDL_FRect* dstrect);
  int (*QueueCopyEx)(SDL_Renderer* renderer,
                     SDL_RenderCommand* cmd,
                     SDL_Texture* texture,
                     const SDL_Rect* srcquad,
                     const SDL_FRect* dstrect,
                     const double angle,
                     const SDL_FPoint* center,
                     const SDL_RendererFlip flip);
  int (*RunCommandQueue)(SDL_Renderer* renderer,
                         SDL_RenderCommand* cmd,
                         void* vertices,
                         size_t vertsize);
  int (*UpdateTexture)(SDL_Renderer* renderer,
                       SDL_Texture* texture,
                       const SDL_Rect* rect,
                       const void* pixels,
                       int pitch);
  int (*UpdateTextureYUV)(SDL_Renderer* renderer,
                          SDL_Texture* texture,
                          const SDL_Rect* rect,
                          const Uint8* Yplane,
                          int Ypitch,
                          const Uint8* Uplane,
                          int Upitch,
                          const Uint8* Vplane,
                          int Vpitch);
  int (*LockTexture)(SDL_Renderer* renderer,
                     SDL_Texture* texture,
                     const SDL_Rect* rect,
                     void** pixels,
                     int* pitch);
  void (*UnlockTexture)(SDL_Renderer* renderer, SDL_Texture* texture);
  void (*SetTextureScaleMode)(SDL_Renderer* renderer,
                              SDL_Texture* texture,
                              SDL_ScaleMode scaleMode);
  int (*SetRenderTarget)(SDL_Renderer* renderer, SDL_Texture* texture);
  int (*RenderReadPixels)(SDL_Renderer* renderer,
                          const SDL_Rect* rect,
                          Uint32 format,
                          void* pixels,
                          int pitch);
  void (*RenderPresent)(SDL_Renderer* renderer);
  void (*DestroyTexture)(SDL_Renderer* renderer, SDL_Texture* texture);

  void (*DestroyRenderer)(SDL_Renderer* renderer);

  int (*GL_BindTexture)(SDL_Renderer* renderer,
                        SDL_Texture* texture,
                        float* texw,
                        float* texh);
  int (*GL_UnbindTexture)(SDL_Renderer* renderer, SDL_Texture* texture);

  void* (*GetMetalLayer)(SDL_Renderer* renderer);
  void* (*GetMetalCommandEncoder)(SDL_Renderer* renderer);

  /* The current renderer info */
  SDL_RendererInfo info;

  /* The window associated with the renderer */
  SDL_Window* window;
  SDL_bool hidden;

  /* The logical resolution for rendering */
  int logical_w;
  int logical_h;
  int logical_w_backup;
  int logical_h_backup;

  /* Whether or not to force the viewport to even integer intervals */
  SDL_bool integer_scale;

  /* The drawable area within the window */
  SDL_Rect viewport;
  SDL_Rect viewport_backup;

  /* The clip rectangle within the window */
  SDL_Rect clip_rect;
  SDL_Rect clip_rect_backup;

  /* Wether or not the clipping rectangle is used. */
  SDL_bool clipping_enabled;
  SDL_bool clipping_enabled_backup;

  /* The render output coordinate scale */
  SDL_FPoint scale;
  SDL_FPoint scale_backup;

  /* The pixel to point coordinate scale */
  SDL_FPoint dpi_scale;

  /* Whether or not to scale relative mouse motion */
  SDL_bool relative_scaling;

  /* Remainder from scaled relative motion */
  float xrel;
  float yrel;

  /* The list of textures */
  SDL_Texture* textures;
  SDL_Texture* target;
  SDL_mutex* target_mutex;

  Uint8 r, g, b, a;        /**< Color for drawing operations values */
  SDL_BlendMode blendMode; /**< The drawing blend mode */

  SDL_bool always_batch;
  SDL_bool batching;
  SDL_RenderCommand* render_commands;
  SDL_RenderCommand* render_commands_tail;
  SDL_RenderCommand* render_commands_pool;
  Uint32 render_command_generation;
  Uint32 last_queued_color;
  SDL_Rect last_queued_viewport;
  SDL_Rect last_queued_cliprect;
  SDL_bool last_queued_cliprect_enabled;
  SDL_bool color_queued;
  SDL_bool viewport_queued;
  SDL_bool cliprect_queued;

  void* vertex_data;
  size_t vertex_data_used;
  size_t vertex_data_allocation;

  void* driverdata;
};

// UMU: Texture

struct SDL_SW_YUVTexture {
  Uint32 format;
  Uint32 target_format;
  int w, h;
  Uint8* pixels;

  /* These are just so we don't have to allocate them separately */
  Uint16 pitches[3];
  Uint8* planes[3];

  /* This is a temporary surface in case we have to stretch copy */
  SDL_Surface* stretch;
  SDL_Surface* display;
};

typedef struct SDL_SW_YUVTexture SDL_SW_YUVTexture;

/* Define the SDL texture structure */
struct SDL_Texture {
  const void* magic;
  Uint32 format;           /**< The pixel format of the texture */
  int access;              /**< SDL_TextureAccess */
  int w;                   /**< The width of the texture */
  int h;                   /**< The height of the texture */
  int modMode;             /**< The texture modulation mode */
  SDL_BlendMode blendMode; /**< The texture blend mode */
  SDL_ScaleMode scaleMode; /**< The texture scale mode */
  Uint8 r, g, b, a;        /**< Texture modulation values */

  SDL_Renderer* renderer;

  /* Support for formats not supported directly by the renderer */
  SDL_Texture* native;
  SDL_SW_YUVTexture* yuv;
  void* pixels;
  int pitch;
  SDL_Rect locked_rect;
  SDL_Surface* locked_surface; /**< Locked region exposed as a SDL surface */

  Uint32 last_command_generation; /* last command queue generation this texture
                                     was in. */

  void* driverdata; /**< Driver specific texture representation */

  SDL_Texture* prev;
  SDL_Texture* next;
};

/* Per-texture data */
typedef struct {
  ID3D11Texture2D* mainTexture;
  ID3D11ShaderResourceView* mainTextureResourceView;
  ID3D11RenderTargetView* mainTextureRenderTargetView;
  ID3D11Texture2D* stagingTexture;
  int lockedTexturePositionX;
  int lockedTexturePositionY;
  D3D11_FILTER scaleMode;

  /* YV12 texture support */
  SDL_bool yuv;
  ID3D11Texture2D* mainTextureU;
  ID3D11ShaderResourceView* mainTextureResourceViewU;
  ID3D11Texture2D* mainTextureV;
  ID3D11ShaderResourceView* mainTextureResourceViewV;

  /* NV12 texture support */
  SDL_bool nv12;
  ID3D11Texture2D* mainTextureNV;
  ID3D11ShaderResourceView* mainTextureResourceViewNV;

  Uint8* pixels;
  int pitch;
  SDL_Rect locked_rect;
} D3D11_TextureData;

/* Define the SDL window-shaper structure */
struct SDL_WindowShaper {
  /* The window associated with the shaper */
  SDL_Window* window;

  /* The user's specified coordinates for the window, for once we give it a
   * shape. */
  Uint32 userx, usery;

  /* The parameters for shape calculation. */
  SDL_WindowShapeMode mode;

  /* Has this window been assigned a shape? */
  SDL_bool hasshape;

  void* driverdata;
};

/* Define the SDL shape driver structure */
struct SDL_ShapeDriver {
  SDL_WindowShaper* (*CreateShaper)(SDL_Window* window);
  int (*SetWindowShape)(SDL_WindowShaper* shaper,
                        SDL_Surface* shape,
                        SDL_WindowShapeMode* shape_mode);
  int (*ResizeWindowShape)(SDL_Window* window);
};

typedef struct SDL_WindowUserData {
  char* name;
  void* data;
  struct SDL_WindowUserData* next;
} SDL_WindowUserData;

/* Define the SDL window structure, corresponding to toplevel windows */
struct SDL_Window {
  const void* magic;
  Uint32 id;
  char* title;
  SDL_Surface* icon;
  int x, y;
  int w, h;
  int min_w, min_h;
  int max_w, max_h;
  Uint32 flags;
  Uint32 last_fullscreen_flags;

  /* Stored position and size for windowed mode */
  SDL_Rect windowed;

  SDL_DisplayMode fullscreen_mode;

  float opacity;

  float brightness;
  Uint16* gamma;
  Uint16* saved_gamma; /* (just offset into gamma) */

  SDL_Surface* surface;
  SDL_bool surface_valid;

  SDL_bool is_hiding;
  SDL_bool is_destroying;
  SDL_bool is_dropping; /* drag/drop in progress, expecting
                           SDL_SendDropComplete(). */

  SDL_WindowShaper* shaper;

  SDL_HitTest hit_test;
  void* hit_test_data;

  SDL_WindowUserData* data;

  void* driverdata;

  SDL_Window* prev;
  SDL_Window* next;
};

#if SDL_VIDEO_OPENGL_EGL
typedef void* EGLSurface;
#endif

typedef struct {
  SDL_Window* window;
  HWND hwnd;
  HWND parent;
  HDC hdc;
  HDC mdc;
  HINSTANCE hinstance;
  HBITMAP hbm;
  WNDPROC wndproc;
  SDL_bool created;
  WPARAM mouse_button_flags;
  SDL_bool initializing;
  SDL_bool expected_resize;
  SDL_bool in_border_change;
  SDL_bool in_title_click;
  Uint8 focus_click_pending;
  SDL_bool skip_update_clipcursor;
  SDL_bool windowed_mode_was_maximized;
  SDL_bool in_window_deactivation;
  RECT cursor_clipped_rect;
  struct SDL_VideoData* videodata;
#if SDL_VIDEO_OPENGL_EGL
  EGLSurface egl_surface;
#endif
} SDL_WindowData;
