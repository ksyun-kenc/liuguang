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

#include <SDKDDKVer.h>
#ifdef _WIN32_WINNT
#undef _WIN32_WINNT
#endif
#define _WIN32_WINNT _WIN32_WINNT_WIN7

#define BOOST_ASIO_NO_DEPRECATED
#define BOOST_URL_NO_LIB
#define BOOST_URL_NO_SOURCE_LOCATION

#include <atlbase.h>
#include <atlfile.h>

#include <array>
#include <chrono>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <mutex>
#include <queue>
#include <span>
#include <string>
#include <thread>

#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>
#include <boost/pool/pool.hpp>
#include <boost/scope_exit.hpp>
