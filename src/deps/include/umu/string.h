#pragma once

#include <algorithm>
#include <string>
#include <vector>

#include <stdint.h>

namespace umu {
namespace string {
#pragma region "Join"
template <size_t N>
#if _HAS_CXX20
constexpr
#endif
std::string ArrayJoin(
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
}  // end of namespace string
}  // end of namespace umu
