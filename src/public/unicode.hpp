//
// Copyright (c) 2012 Artyom Beilis (Tonkikh)
// Copyright (c) 2020 Alexander Grund
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#pragma once

#define REPLACEMENT_CHARACTER 0xFFFD
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <string>
#include <type_traits>

///
/// \brief Namespace that holds basic operations on UTF encoded sequences
///
/// All functions defined in this namespace do not require linking with
/// Boost.Nowide library. Extracted from Boost.Locale
///
namespace utf {

    ///
    /// \brief The integral type that can hold a Unicode code point
    ///
    using code_point = uint32_t;

    ///
    /// \brief Special constant that defines illegal code point
    ///
    static const code_point illegal = 0xFFFFFFFFu;

    ///
    /// \brief Special constant that defines incomplete code point
    ///
    static const code_point incomplete = 0xFFFFFFFEu;

    ///
    /// \brief the function checks if \a v is a valid code point
    ///
    inline bool is_valid_codepoint(code_point v) {
        if (v > 0x10FFFF)
            return false;
        if (0xD800 <= v && v <= 0xDFFF) // surrogates
            return false;
        return true;
    }

    template <typename CharType, int size = sizeof(CharType)> struct utf_traits;

    template <typename CharType> struct utf_traits<CharType, 1> {
        using char_type = CharType;

        static int trail_length(char_type ci) {
            unsigned char c = ci;
            if (c < 128)
                return 0;
            if (c < 194) [[unlikely]]
                return -1;
            if (c < 224)
                return 1;
            if (c < 240)
                return 2;
            if (c <= 244) [[likely]]
                return 3;
            return -1;
        }

        static const int max_width = 4;

        static int width(code_point value) {
            if (value <= 0x7F) {
                return 1;
            } else if (value <= 0x7FF) {
                return 2;
            } else if (value <= 0xFFFF) [[likely]] {
                return 3;
            } else {
                return 4;
            }
        }

        static bool is_trail(char_type ci) {
            unsigned char c = ci;
            return (c & 0xC0) == 0x80;
        }

        static bool is_lead(char_type ci) { return !is_trail(ci); }

        template <typename Iterator>
        static code_point decode(Iterator &p, Iterator e) {
            if (p == e) [[unlikely]]
                return incomplete;

            unsigned char lead = *p++;

            // First byte is fully validated here
            int trail_size = trail_length(lead);

            if (trail_size < 0) [[unlikely]]
                return illegal;

            // OK as only ASCII may be of size = 0
            // also optimize for ASCII text
            if (trail_size == 0)
                return lead;

            code_point c = lead & ((1 << (6 - trail_size)) - 1);

            // Read the rest
            unsigned char tmp;
            switch (trail_size) {
                case 3:
                    if (p == e) [[unlikely]]
                        return incomplete;
                    tmp = *p++;
                    if (!is_trail(tmp))
                        return illegal;
                    c = (c << 6) | (tmp & 0x3F);
                    [[fallthrough]];
                case 2:
                    if (p == e) [[unlikely]]
                        return incomplete;
                    tmp = *p++;
                    if (!is_trail(tmp))
                        return illegal;
                    c = (c << 6) | (tmp & 0x3F);
                    [[fallthrough]];
                case 1:
                    if (p == e) [[unlikely]]
                        return incomplete;
                    tmp = *p++;
                    if (!is_trail(tmp))
                        return illegal;
                    c = (c << 6) | (tmp & 0x3F);
            }

            // Check code point validity:
            // - no surrogates and valid range
            // - most compact representation
            if (!is_valid_codepoint(c) || width(c) != trail_size + 1)
                [[unlikely]] {
                p -= trail_size;
                return illegal;
            }

            return c;
        }

        template <typename Iterator>
        static code_point decode_valid(Iterator &p) {
            unsigned char lead = *p++;
            if (lead < 192)
                return lead;

            int trail_size;

            if (lead < 224)
                trail_size = 1;
            else if (lead < 240) [[likely]] // non-BMP rare
                trail_size = 2;
            else
                trail_size = 3;

            code_point c = lead & ((1 << (6 - trail_size)) - 1);

            switch (trail_size) {
                case 3:
                    c = (c << 6) | (static_cast<unsigned char>(*p++) & 0x3F);
                case 2:
                    c = (c << 6) | (static_cast<unsigned char>(*p++) & 0x3F);
                case 1:
                    c = (c << 6) | (static_cast<unsigned char>(*p++) & 0x3F);
            }

            return c;
        }

        template <typename Iterator>
        static Iterator encode(code_point value, Iterator out) {
            if (value <= 0x7F) {
                *out++ = static_cast<char_type>(value);
            } else if (value <= 0x7FF) {
                *out++ = static_cast<char_type>((value >> 6) | 0xC0);
                *out++ = static_cast<char_type>((value & 0x3F) | 0x80);
            } else if (value <= 0xFFFF) [[likely]] {
                *out++ = static_cast<char_type>((value >> 12) | 0xE0);
                *out++ = static_cast<char_type>(((value >> 6) & 0x3F) | 0x80);
                *out++ = static_cast<char_type>((value & 0x3F) | 0x80);
            } else {
                *out++ = static_cast<char_type>((value >> 18) | 0xF0);
                *out++ = static_cast<char_type>(((value >> 12) & 0x3F) | 0x80);
                *out++ = static_cast<char_type>(((value >> 6) & 0x3F) | 0x80);
                *out++ = static_cast<char_type>((value & 0x3F) | 0x80);
            }
            return out;
        }
    }; // utf8

    template <typename CharType> struct utf_traits<CharType, 2> {
        using char_type = CharType;

        // See RFC 2781
        static bool is_single_codepoint(uint16_t x) {
            // Ranges [U+0000, 0+D7FF], [U+E000, U+FFFF] are numerically equal
            // in UTF-16
            return x <= 0xD7FF || x >= 0xE000;
        }
        static bool is_first_surrogate(uint16_t x) {
            // Range [U+D800, 0+DBFF]: High surrogate
            return 0xD800 <= x && x <= 0xDBFF;
        }
        static bool is_second_surrogate(uint16_t x) {
            // Range [U+DC00, 0+DFFF]: Low surrogate
            return 0xDC00 <= x && x <= 0xDFFF;
        }
        static code_point combine_surrogate(uint16_t w1, uint16_t w2) {
            return ((code_point(w1 & 0x3FF) << 10) | (w2 & 0x3FF)) + 0x10000;
        }
        static int trail_length(char_type c) {
            if (is_first_surrogate(c))
                return 1;
            if (is_second_surrogate(c))
                return -1;
            return 0;
        }
        /// Return true if c is trail code unit, always false for UTF-32
        static bool is_trail(char_type c) { return is_second_surrogate(c); }
        /// Return true if c is lead code unit, always true of UTF-32
        static bool is_lead(char_type c) { return !is_second_surrogate(c); }

        template <typename It> static code_point decode(It &current, It last) {
            if (current == last) [[unlikely]]
                return incomplete;
            uint16_t w1 = *current++;
            if (is_single_codepoint(w1)) [[likely]] {
                return w1;
            }
            // Now it's either a high or a low surrogate, the latter is invalid
            if (w1 >= 0xDC00)
                return illegal;
            if (current == last)
                return incomplete;
            uint16_t w2 = *current++;
            if (!is_second_surrogate(w2))
                return illegal;
            return combine_surrogate(w1, w2);
        }
        template <typename It> static code_point decode_valid(It &current) {
            uint16_t w1 = *current++;
            if (is_single_codepoint(w1)) [[likely]] {
                return w1;
            }
            uint16_t w2 = *current++;
            return combine_surrogate(w1, w2);
        }

        static const int max_width = 2;
        static int width(code_point u) // LCOV_EXCL_LINE
        {
            return u >= 0x10000 ? 2 : 1;
        }
        template <typename It> static It encode(code_point u, It out) {
            if (u <= 0xFFFF) [[likely]] {
                *out++ = static_cast<char_type>(u);
            } else {
                u -= 0x10000;
                *out++ = static_cast<char_type>(0xD800 | (u >> 10));
                *out++ = static_cast<char_type>(0xDC00 | (u & 0x3FF));
            }
            return out;
        }
    }; // utf16;

    template <typename CharType> struct utf_traits<CharType, 4> {
        using char_type = CharType;
        static int trail_length(char_type c) {
            if (is_valid_codepoint(c))
                return 0;
            return -1;
        }
        static bool is_trail(char_type /*c*/) { return false; }
        static bool is_lead(char_type /*c*/) { return true; }

        template <typename It> static code_point decode_valid(It &current) {
            return *current++;
        }

        template <typename It> static code_point decode(It &current, It last) {
            if (current == last) [[unlikely]]
                return incomplete;
            code_point c = *current++;
            if (!is_valid_codepoint(c)) [[unlikely]]
                return illegal;
            return c;
        }
        static const int max_width = 1;
        static int width(code_point /*u*/) { return 1; }
        template <typename It> static It encode(code_point u, It out) {
            *out++ = static_cast<char_type>(u);
            return out;
        }
    }; // utf32

} // namespace utf

namespace detail {
    template <class...> struct make_void {
        typedef void type;
    };

    template <class... Ts> using void_t = typename make_void<Ts...>::type;

    template <typename T> struct is_char_type : std::false_type { };
    template <> struct is_char_type<char> : std::true_type { };
    template <> struct is_char_type<wchar_t> : std::true_type { };
    template <> struct is_char_type<char16_t> : std::true_type { };
    template <> struct is_char_type<char32_t> : std::true_type { };
#ifdef __cpp_char8_t
    template <> struct is_char_type<char8_t> : std::true_type { };
#endif

    template <typename T> struct is_c_string : std::false_type { };
    template <typename T> struct is_c_string<const T *> : is_char_type<T> { };

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
    struct is_string_container : std::false_type { };
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

///
/// Convert wide string (UTF-16/32) in range [begin,end) to NULL terminated
/// narrow string (UTF-8) stored in \a output of size \a output_size (including
/// NULL)
///
/// If there is not enough room NULL is returned, else output is returned.
/// Any illegal sequences are replaced with the replacement character, see
/// #REPLACEMENT_CHARACTER
///
inline char *narrow(char *output, std::size_t output_size, const wchar_t *begin,
                    const wchar_t *end) {
    return utf::convert_buffer(output, output_size, begin, end);
}
///
/// Convert NULL terminated wide string (UTF-16/32) to NULL terminated narrow
/// string (UTF-8) stored in \a output of size \a output_size (including NULL)
///
/// If there is not enough room NULL is returned, else output is returned.
/// Any illegal sequences are replaced with the replacement character, see
/// #REPLACEMENT_CHARACTER
///
inline char *narrow(char *output, std::size_t output_size,
                    const wchar_t *source) {
    return narrow(output, output_size, source, source + utf::strlen(source));
}

///
/// Convert narrow string (UTF-8) in range [begin,end) to NULL terminated wide
/// string (UTF-16/32) stored in \a output of size \a output_size (including
/// NULL)
///
/// If there is not enough room NULL is returned, else output is returned.
/// Any illegal sequences are replaced with the replacement character, see
/// #REPLACEMENT_CHARACTER
///
inline wchar_t *widen(wchar_t *output, std::size_t output_size,
                      const char *begin, const char *end) {
    return utf::convert_buffer(output, output_size, begin, end);
}
///
/// Convert NULL terminated narrow string (UTF-8) to NULL terminated wide string
/// (UTF-16/32) most output_size (including NULL)
///
/// If there is not enough room NULL is returned, else output is returned.
/// Any illegal sequences are replaced with the replacement character, see
/// #REPLACEMENT_CHARACTER
///
inline wchar_t *widen(wchar_t *output, std::size_t output_size,
                      const char *source) {
    return widen(output, output_size, source, source + utf::strlen(source));
}

///
/// Convert wide string (UTF-16/32) to narrow string (UTF-8).
///
/// \param s Input string
/// \param count Number of characters to convert
/// Any illegal sequences are replaced with the replacement character, see
/// #REPLACEMENT_CHARACTER
///
template <typename T_Char, typename = detail::requires_wide_char<T_Char>>
inline std::string narrow(const T_Char *s, std::size_t count) {
    return utf::convert_string<char>(s, s + count);
}
///
/// Convert wide string (UTF-16/32) to narrow string (UTF-8).
///
/// \param s NULL terminated input string
/// Any illegal sequences are replaced with the replacement character, see
/// #REPLACEMENT_CHARACTER
///
template <typename T_Char, typename = detail::requires_wide_char<T_Char>>
inline std::string narrow(const T_Char *s) {
    return narrow(s, utf::strlen(s));
}
///
/// Convert wide string (UTF-16/32) to narrow string (UTF-8).
///
/// \param s Input string
/// Any illegal sequences are replaced with the replacement character, see
/// #REPLACEMENT_CHARACTER
///
template <typename StringOrStringView,
          typename = detail::requires_wide_string_container<StringOrStringView>>
inline std::string narrow(const StringOrStringView &s) {
    return utf::convert_string<char>(s.data(), s.data() + s.size());
}

///
/// Convert narrow string (UTF-8) to wide string (UTF-16/32).
///
/// \param s Input string
/// \param count Number of characters to convert
/// Any illegal sequences are replaced with the replacement character, see
/// #REPLACEMENT_CHARACTER
///
template <typename T_Char, typename = detail::requires_narrow_char<T_Char>>
inline std::wstring widen(const T_Char *s, std::size_t count) {
    return utf::convert_string<wchar_t>(s, s + count);
}
///
/// Convert narrow string (UTF-8) to wide string (UTF-16/32).
///
/// \param s NULL terminated input string
/// Any illegal sequences are replaced with the replacement character, see
/// #REPLACEMENT_CHARACTER
///
template <typename T_Char, typename = detail::requires_narrow_char<T_Char>>
inline std::wstring widen(const T_Char *s) {
    return widen(s, utf::strlen(s));
}
///
/// Convert narrow string (UTF-8) to wide string (UTF-16/32).
///
/// \param s Input string
/// Any illegal sequences are replaced with the replacement character, see
/// #REPLACEMENT_CHARACTER
///
template <
    typename StringOrStringView,
    typename = detail::requires_narrow_string_container<StringOrStringView>>
inline std::wstring widen(const StringOrStringView &s) {
    return utf::convert_string<wchar_t>(s.data(), s.data() + s.size());
}
