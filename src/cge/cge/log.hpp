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

#include <format>

#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sources/logger.hpp>
#include <boost/log/trivial.hpp>

namespace logging = boost::log;
namespace sinks = boost::log::sinks;
namespace attrs = boost::log::attributes;
namespace src = boost::log::sources;
namespace expr = boost::log::expressions;
namespace keywords = boost::log::keywords;

enum class SeverityLevel {
  kTrace,
  kDebug,
  kInfo,
  kWarning,
  kError,
  kFatal,
  kLast
};
constexpr std::array<std::string_view, 6> kValidSeverityLevel = {
    "trace", "debug", "info", "warning", "error", "fatal"};
constexpr size_t kDefaultSeverityLevelIndex = 2;

constexpr size_t MaxSeverityLevelNameSize() {
  size_t max_size = 0;
  for (auto& e : kValidSeverityLevel) {
    if (max_size < e.size()) {
      max_size = e.size();
    }
  }
  return max_size;
}

inline const char* ToString(SeverityLevel level) {
  if (level < SeverityLevel::kLast) {
    return kValidSeverityLevel[static_cast<std::size_t>(level)].data();
  }
  return "*";
}

inline bool FromString(const std::string& str, SeverityLevel& level) {
  auto pos =
      std::find(kValidSeverityLevel.cbegin(), kValidSeverityLevel.cend(), str);
  if (kValidSeverityLevel.cend() == pos) {
    return false;
  }
  level = static_cast<SeverityLevel>(
      std::distance(kValidSeverityLevel.cbegin(), pos));
  return true;
}

template <typename CharT, typename TraitsT>
inline std::basic_ostream<CharT, TraitsT>& operator<<(
    std::basic_ostream<CharT, TraitsT>& os,
    SeverityLevel level) {
  if (level < SeverityLevel::kLast) {
    os << ToString(level);
  } else {
    os << static_cast<std::size_t>(level);
  }
  return os;
}

BOOST_LOG_ATTRIBUTE_KEYWORD(Severity, "Severity", SeverityLevel)
