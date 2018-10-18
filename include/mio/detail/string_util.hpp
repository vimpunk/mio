/* Copyright 2017 https://github.com/mandreyel
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this
 * software and associated documentation files (the "Software"), to deal in the Software
 * without restriction, including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies
 * or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef MIO_STRING_UTIL_HEADER
#define MIO_STRING_UTIL_HEADER

#include <type_traits>

namespace mio {
namespace detail {

template <typename String>
struct is_c_str
{
    static constexpr bool value = std::is_same<
        char*,
        // TODO: I'm so sorry for this... Can this be made cleaner?
        typename std::add_pointer<
            typename std::remove_cv<
                typename std::remove_pointer<
                    typename std::decay<
                        String
                    >::type
                >::type
            >::type
        >::type
    >::value;
};

template<
    typename String,
    typename = decltype(std::declval<String>().data()),
    typename = typename std::enable_if<!is_c_str<String>::value>::type
> const char* c_str(const String& path)
{
    return path.data();
}

template<
    typename String,
    typename = decltype(std::declval<String>().empty()),
    typename = typename std::enable_if<!is_c_str<String>::value>::type
> bool empty(const String& path)
{
    return path.empty();
}

template<
    typename String,
    typename = typename std::enable_if<is_c_str<String>::value>::type
> const char* c_str(String path)
{
    return path;
}

template<
    typename String,
    typename = typename std::enable_if<is_c_str<String>::value>::type
> bool empty(String path)
{
    return !path || (*path == 0);
}

} // namespace detail
} // namespace mio

#endif // MIO_STRING_UTIL_HEADER
