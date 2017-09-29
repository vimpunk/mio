#ifndef MIO_TYPE_TRAITS_HEADER
#define MIO_TYPE_TRAITS_HEADER

#include <type_traits>

namespace mio {
namespace detail {

template<typename String> struct is_c_str
{
    static constexpr bool value = std::is_same<
        const char*, typename std::decay<String>::type
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

#endif // MIO_TYPE_TRAITS_HEADER
