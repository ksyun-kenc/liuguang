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

#pragma once

#include <d3d11.h>

HRESULT CaptureTexture(_In_ ID3D11Device* device,
                       _In_ ID3D11DeviceContext* context,
                       _In_ ID3D11Resource* source,
                       D3D11_TEXTURE2D_DESC& desc,
                       CComPtr<ID3D11Texture2D>& staging) noexcept;
