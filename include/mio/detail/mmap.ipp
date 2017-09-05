#ifndef MIO_MMAP_IMPL
#define MIO_MMAP_IMPL

#include "../mmap.hpp"

namespace mio {

// -- basic_mmap_source --

template<typename CharT>
basic_mmap_source<CharT>::basic_mmap_source(const std::string& path,
    const size_type offset, const size_type length)
{
    std::error_code error;
    map(path, offset, length, error);
    if(error) { throw error; }
}

template<typename CharT>
basic_mmap_source<CharT>::basic_mmap_source(const handle_type handle,
    const size_type offset, const size_type length)
{
    std::error_code error;
    map(handle, offset, length, error);
    if(error) { throw error; }
}

template<typename CharT>
void basic_mmap_source<CharT>::map(const std::string& path, const size_type offset,
    const size_type length, std::error_code& error)
{
    impl_.map(path, offset, length, impl_type::access_mode::read_only, error);
}

template<typename CharT>
void basic_mmap_source<CharT>::map(const handle_type handle, const size_type offset,
    const size_type length, std::error_code& error)
{
    impl_.map(handle, offset, length, impl_type::access_mode::read_only, error);
}

// -- basic_mmap_sink --

template<typename CharT>
basic_mmap_sink<CharT>::basic_mmap_sink(const std::string& path,
    const size_type offset, const size_type length)
{
    std::error_code error;
    map(path, offset, length, error);
    if(error) { throw error; }
}

template<typename CharT>
basic_mmap_sink<CharT>::basic_mmap_sink(const handle_type handle,
    const size_type offset, const size_type length)
{
    std::error_code error;
    map(handle, offset, length, error);
    if(error) { throw error; }
}

template<typename CharT>
void basic_mmap_sink<CharT>::map(const std::string& path, const size_type offset,
    const size_type length, std::error_code& error)
{
    impl_.map(path, offset, length, impl_type::access_mode::read_only, error);
}

template<typename CharT>
void basic_mmap_sink<CharT>::map(const handle_type handle, const size_type offset,
    const size_type length, std::error_code& error)
{
    impl_.map(handle, offset, length, impl_type::access_mode::read_write, error);
}

template<typename CharT>
void basic_mmap_sink<CharT>::sync(std::error_code& error) { impl_.sync(error); }

template<typename CharT>
bool operator==(const basic_mmap_source<CharT>& a, const basic_mmap_source<CharT>& b)
{
    return a.impl_ == b.impl_;
}

template<typename CharT>
bool operator!=(const basic_mmap_source<CharT>& a, const basic_mmap_source<CharT>& b)
{
    return a.impl_ != b.impl_;
}

template<typename CharT>
bool operator==(const basic_mmap_sink<CharT>& a, const basic_mmap_sink<CharT>& b)
{
    return a.impl_ == b.impl_;
}

template<typename CharT>
bool operator!=(const basic_mmap_sink<CharT>& a, const basic_mmap_sink<CharT>& b)
{
    return a.impl_ != b.impl_;
}

} // namespace mio

#endif // MIO_MMAP_IMPL
