#pragma once

#include <algorithm>
#include <string>
#include <vector>

#include <stdint.h>

namespace umu {
namespace string {
#if UNICODE
#define tstring std::wstring
#else
#define tstring std::string
#endif
#pragma endregion

#pragma region "Join"
template <size_t N>
constexpr std::string ArrayJoin(
    const std::array<std::string_view, N>& array) {
  auto buffer = std::string("{").append(array.at(0));
  for (size_t i = 1; i < N; ++i) {
    buffer.append(", ").append(array.at(i));
  }
  return buffer.append("}");
}
#pragma endregion

#pragma region "Split"
// source_string 末尾如果是 separator，其后的空白不被加入
// container，只有中间的空白会被加入 container
template <class StringType>
inline typename StringType::size_type Split(std::vector<StringType>* container,
                                            const StringType& source_string,
                                            const StringType& separator) {
  if (NULL != container) {
    container->clear();
  }

  size_t size = 0;
  typename StringType::size_type legth = source_string.length();
  typename StringType::size_type start = 0;
  typename StringType::size_type end;

  do {
    end = source_string.find(separator, start);
    StringType e(source_string.substr(start, end - start));
    if (NULL != container) {
      container->push_back(e);
    }
    start = end + separator.length();
    ++size;
  } while (start < legth && StringType::npos != end);

  return size;
}
template <class StringType, typename CharType>
inline typename StringType::size_type Split(std::vector<StringType>* container,
                                            const StringType& source_string,
                                            const CharType* separator) {
  return Split(container, source_string, StringType(separator));
}
template <class StringType, typename CharType>
inline typename StringType::size_type Split(std::vector<StringType>* container,
                                            const CharType* source_string,
                                            const CharType* separator) {
  return Split(container, StringType(source_string), StringType(separator));
}
#pragma endregion

#pragma region "SplitAnyOf"
template <class StringType>
inline typename StringType::size_type SplitAnyOf(
    std::vector<StringType>* container,
    const StringType& source_string,
    const StringType& token) {
  if (nullptr != container) {
    container->clear();
  }

  size_t size = 0;
  typename StringType::size_type legth = source_string.length();
  typename StringType::size_type start = 0;
  typename StringType::size_type end;

  do {
    end = source_string.find_first_of(token, start);
    StringType e(source_string.substr(start, end - start));
    if (nullptr != container) {
      container->push_back(e);
    }
    start = end + 1;
    ++size;
  } while (start < legth && StringType::npos != end);

  return size;
}
template <class StringType, typename CharType>
inline typename StringType::size_type SplitAnyOf(
    std::vector<StringType>* container,
    const StringType& source_string,
    const CharType token) {
  return SplitAnyOf(container, source_string, StringType(token));
}
template <class StringType, typename CharType>
inline typename StringType::size_type SplitAnyOf(
    std::vector<StringType>* container,
    const CharType source_string,
    const CharType token) {
  return SplitAnyOf(container, StringType(source_string), StringType(token));
}
#pragma endregion

#pragma region "Replace"
template <class StringType>
inline typename StringType::size_type Replace(StringType& source_string,
                                              const StringType& find,
                                              const StringType& replace_with) {
  typename StringType::size_type replace_times = 0;
  typename StringType::size_type pos = 0;
  for (;;) {
    pos = source_string.find(find, pos);
    if (StringType::npos == pos) {
      break;
    }
    source_string.replace(pos, find.size(), replace_with);
    ++replace_times;
    pos += replace_with.size();
  }
  return replace_times;
}
template <class StringType, typename CharType>
inline typename StringType::size_type Replace(StringType& source_string,
                                              const StringType& find,
                                              const CharType* replace_with) {
  return Replace(source_string, find, StringType(replace_with));
}
template <class StringType, typename CharType>
inline typename StringType::size_type Replace(StringType& source_string,
                                              const CharType* find,
                                              const CharType* replace_with) {
  return Replace(source_string, StringType(find), StringType(replace_with));
}

template <class StringType, typename CharType>
inline typename StringType::size_type Replace(StringType& source_string,
                                              const CharType find,
                                              const CharType replace_with) {
  typename StringType::size_type replace_times = 0;
  for (auto it = source_string.begin(); it != source_string.end(); ++it) {
    if (*it == find) {
      *it = replace_with;
      ++replace_times;
    }
  }
  return replace_times;
}
#pragma endregion

#pragma region "Trim"
template <class StringType>
inline StringType Trim(StringType& str) {
  typename StringType::size_type pos = str.find_last_not_of(' ');
  if (pos == StringType::npos) {
    str.clear();
  } else {
    str.erase(pos + 1);
    pos = str.find_first_not_of(' ');
    if (0 < pos) {
      str.erase(0, pos);
    }
  }
  return str;
}
#pragma endregion

#pragma region "Hex helper function"
template <typename CharType, bool uppercase_hex_chars = false>
inline ::std::basic_string<CharType,
                           ::std::char_traits<CharType>,
                           ::std::allocator<CharType>>
HexEncodeT(_In_bytecount_(size) const void* data, size_t size) {
  static const CharType HEX_CHARS[16] = {'0', '1', '2', '3', '4', '5',
                                         '6', '7', '8', '9', 'A', 'B',
                                         'C', 'D', 'E', 'F'};
  static const CharType hex_chars[16] = {'0', '1', '2', '3', '4', '5',
                                         '6', '7', '8', '9', 'a', 'b',
                                         'c', 'd', 'e', 'f'};
  static const CharType* hex_chars_t =
      uppercase_hex_chars ? HEX_CHARS : hex_chars;

  ::std::basic_string<CharType, ::std::char_traits<CharType>,
                      ::std::allocator<CharType>>
      result;
  if (!data || 0 == size) {
    return result;
  }

  result.resize(size * 2);
  CharType* buffer = const_cast<CharType*>(result.c_str());
  size_t read = 0;
  size_t written = 0;
  while (read < size) {
    uint8_t ch = static_cast<const uint8_t*>(data)[read];
    ++read;
    *buffer++ = hex_chars_t[(ch >> 4) & 0x0F];
    *buffer++ = hex_chars_t[ch & 0x0F];
    written += 2;
  }

  return result;
}

#define HexEncodeW HexEncodeT<wchar_t>
#define HexEncodeUppercaseW HexEncodeT<wchar_t, true>

#define HexEncodeA HexEncodeT<char>
#define HexEncodeUppercaseA HexEncodeT<char, true>

#if UNICODE
#define HexEncode HexEncodeW
#define HexEncodeUppercase HexEncodeUppercaseW
#else
#define HexEncode HexEncodeA
#define HexEncodeUppercase HexEncodeUppercaseA
#endif
#pragma endregion

#pragma region "version helper function"
inline uint64_t StringVersionToBinary(const char* string_version) {
  uint64_t v = 0;

  if (nullptr == string_version) {
    return v;
  }

  // 去空格
  const char* p(string_version);
  while (isspace(*p)) {
    ++p;
  }
  size_t length = strlen(p);
  while (isspace(p[length - 1])) {
    --length;
  }

  ::std::string s(p, length);
  ::std::vector<::std::string> parts;
  Split(&parts, s, ".");
  int i = 0;
  for (const auto& e : parts) {
    if (!e.empty()) {
      v <<= 16;
      v |= uint16_t(atoi(e.c_str()));
      ++i;
    }
  }
  // UMU: 1.2 表示 1.2.0.0，所以下面语句块是必要的
  for (; i < 4; ++i) {
    v <<= 16;
  }

  return v;
}

inline ::std::string StringVersionFromBinaryA(uint64_t version) {
  // MAX uint16_t = 65535
  char buffer[5 * 4 + 3 + 1];

  sprintf_s(buffer, "%hu.%hu.%hu.%hu",
            reinterpret_cast<const uint16_t*>(&version)[3],
            reinterpret_cast<const uint16_t*>(&version)[2],
            reinterpret_cast<const uint16_t*>(&version)[1],
            reinterpret_cast<const uint16_t*>(&version)[0]);
  return ::std::string(buffer);
}

inline ::std::wstring StringVersionFromBinaryW(uint64_t version) {
  wchar_t buffer[5 * 4 + 3 + 1];

  swprintf_s(buffer, L"%hu.%hu.%hu.%hu",
             reinterpret_cast<const uint16_t*>(&version)[3],
             reinterpret_cast<const uint16_t*>(&version)[2],
             reinterpret_cast<const uint16_t*>(&version)[1],
             reinterpret_cast<const uint16_t*>(&version)[0]);
  return ::std::wstring(buffer);
}

#if UNICODE
#define StringVersionFromBinary StringVersionFromBinaryW
#else
#define StringVersionFromBinary StringVersionFromBinaryA
#endif
#pragma endregion

#pragma region "compare function"
// 1, true, yes
inline bool IsTrue(const char* str) {
  return 0 == strcmp(str, "1") || 0 == _stricmp(str, "TRUE") ||
         0 == _stricmp(str, "YES");
}

// 0, false, no
inline bool IsFalse(const char* str) {
  return 0 == strcmp(str, "0") || 0 == _stricmp(str, "FALSE") ||
         0 == _stricmp(str, "NO");
}

inline bool IsEndWith(const char* str,
                      const char* tail,
                      bool case_sensitive = true) {
  size_t string_length = strlen(str);
  size_t tail_length = strlen(tail);
  if (string_length >= tail_length) {
    return 0 == (case_sensitive ? strncmp(str + string_length - tail_length,
                                          tail, tail_length)
                                : _strnicmp(str + string_length - tail_length,
                                            tail, tail_length));
  }
  return false;
}

inline bool IsStartWith(const char* str,
                        const char* header,
                        bool case_sensitive = true) {
  size_t string_length = strlen(str);
  size_t header_length = strlen(header);
  if (string_length >= header_length) {
    return 0 == (case_sensitive ? strncmp(str, header, header_length)
                                : _strnicmp(str, header, header_length));
  }
  return false;
}
#pragma endregion
}  // end of namespace string
}  // end of namespace umu
