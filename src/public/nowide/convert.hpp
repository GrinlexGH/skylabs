//
// Copyright (c) 2012 Artyom Beilis (Tonkikh)
// Copyright (c) 2020 Alexander Grund
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#pragma once

#define REPLACEMENT_CHARACTER 0xFFFD
#include "utf.hpp"

#include <cstddef>
#include <iterator>
#include <string>
#include <type_traits>

namespace detail {
    template <class...> struct make_void {
        typedef void type;
    };

    template <class... Ts> using void_t = typename make_void<Ts...>::type;

    template <typename T> struct is_char_type : std::false_type {};
    template <> struct is_char_type<char> : std::true_type {};
    template <> struct is_char_type<wchar_t> : std::true_type {};
    template <> struct is_char_type<char16_t> : std::true_type {};
    template <> struct is_char_type<char32_t> : std::true_type {};
#ifdef __cpp_char8_t
    template <> struct is_char_type<char8_t> : std::true_type {};
#endif

    template <typename T> struct is_c_string : std::false_type {};
    template <typename T> struct is_c_string<const T *> : is_char_type<T> {};

    template <typename T>
    using const_data_result = decltype(std::declval<const T>().data());
    /// Return the size of the char type returned by the data() member function
    template <typename T>
    using get_data_width = std::integral_constant<
        std::size_t,
        sizeof(typename std::remove_pointer<const_data_result<T>>::type)>;
    template <typename T>
    using size_result = decltype(std::declval<T>().size());
    /// Return true if the data() member function returns a pointer to a type of
    /// size 1
    template <typename T>
    using has_narrow_data =
        std::integral_constant<bool, (get_data_width<T>::value == 1)>;

    /// Return true if T is a string container, e.g. std::basic_string,
    /// std::basic_string_view Requires a static value `npos`, a member function
    /// `size()` returning an integral, and a member function `data()` returning
    /// a C string
    template <typename T, bool isNarrow, typename = void>
    struct is_string_container : std::false_type {};
    // clang-format off
        template<typename T, bool isNarrow>
        struct is_string_container<T, isNarrow, void_t<decltype(T::npos), size_result<T>, const_data_result<T>>>
            : std::integral_constant<bool,
                                     std::is_integral<decltype(T::npos)>::value
                                       && std::is_integral<size_result<T>>::value
                                       && is_c_string<const_data_result<T>>::value
                                       && isNarrow == has_narrow_data<T>::value>
        {};
    // clang-format on
    template <typename T>
    using requires_narrow_string_container =
        typename std::enable_if<is_string_container<T, true>::value>::type;
    template <typename T>
    using requires_wide_string_container =
        typename std::enable_if<is_string_container<T, false>::value>::type;

    template <typename T>
    using requires_narrow_char =
        typename std::enable_if<sizeof(T) == 1 && is_char_type<T>::value>::type;
    template <typename T>
    using requires_wide_char =
        typename std::enable_if<(sizeof(T) > 1) &&
                                is_char_type<T>::value>::type;

} // namespace detail

namespace utf {
    /// Return the length of the given string in code units.
    /// That is the number of elements of type Char until the first NULL
    /// character. Equivalent to `std::strlen(s)` but can handle wide-strings
    template <typename Char> std::size_t strlen(const Char *s) {
        const Char *end = s;
        while (*end)
            end++;
        return end - s;
    }

    /// Convert a buffer of UTF sequences in the range [source_begin,
    /// source_end) from \a CharIn to \a CharOut to the output \a buffer of size
    /// \a buffer_size.
    ///
    /// \return original buffer containing the NULL terminated string or NULL
    ///
    /// If there is not enough room in the buffer NULL is returned, and the
    /// content of the buffer is undefined. Any illegal sequences are replaced
    /// with the replacement character, see #REPLACEMENT_CHARACTER
    template <typename CharOut, typename CharIn>
    CharOut *convert_buffer(CharOut *buffer, std::size_t buffer_size,
                            const CharIn *source_begin,
                            const CharIn *source_end) {
        CharOut *rv = buffer;
        if (buffer_size == 0)
            return nullptr;
        buffer_size--;
        while (source_begin != source_end) {
            code_point c = utf_traits<CharIn>::decode(source_begin, source_end);
            if (c == illegal || c == incomplete) {
                c = REPLACEMENT_CHARACTER;
            }
            std::size_t width = utf_traits<CharOut>::width(c);
            if (buffer_size < width) {
                rv = nullptr;
                break;
            }
            buffer = utf_traits<CharOut>::encode(c, buffer);
            buffer_size -= width;
        }
        *buffer++ = 0;
        return rv;
    }

    /// Convert the UTF sequences in range [begin, end) from \a CharIn to \a
    /// CharOut and return it as a string
    ///
    /// Any illegal sequences are replaced with the replacement character, see
    /// #REPLACEMENT_CHARACTER \tparam CharOut Output character type
    template <typename CharOut, typename CharIn>
    std::basic_string<CharOut> convert_string(const CharIn *begin,
                                              const CharIn *end) {
        std::basic_string<CharOut> result;
        result.reserve(end - begin);
        using inserter_type =
            std::back_insert_iterator<std::basic_string<CharOut>>;
        inserter_type inserter(result);
        code_point c;
        while (begin != end) {
            c = utf_traits<CharIn>::decode(begin, end);
            if (c == illegal || c == incomplete) {
                c = REPLACEMENT_CHARACTER;
            }
            utf_traits<CharOut>::encode(c, inserter);
        }
        return result;
    }

    /// Convert the UTF sequence in the input string from \a CharIn to \a
    /// CharOut and return it as a string
    ///
    /// Any illegal sequences are replaced with the replacement character, see
    /// #REPLACEMENT_CHARACTER \tparam CharOut Output character type
    template <typename CharOut, typename CharIn>
    std::basic_string<CharOut>
    convert_string(const std::basic_string<CharIn> &s) {
        return convert_string<CharOut>(s.data(), s.data() + s.size());
    }

} // namespace utf
