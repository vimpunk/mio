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

#ifndef MIO_BASIC_MMAP_IMPL
#define MIO_BASIC_MMAP_IMPL

#include "basic_mmap.hpp"
#include "string_util.hpp"
#include "../page.hpp"

#include <algorithm>
#include <cstdint>

#ifndef _WIN32
# include <unistd.h>
# include <fcntl.h>
# include <sys/mman.h>
# include <sys/stat.h>
#endif

namespace mio {
namespace detail {

// Generic handle type for use by free functions.
using handle_type = basic_mmap<char>::handle_type;

#ifdef _WIN32
inline DWORD int64_high(int64_t n) noexcept
{
    return n >> 32;
}

inline DWORD int64_low(int64_t n) noexcept
{
    return n & 0xffffffff;
}
#endif

inline std::error_code last_error() noexcept
{
    std::error_code error;
#ifdef _WIN32
    error.assign(GetLastError(), std::system_category());
#else
    error.assign(errno, std::system_category());
#endif
    return error;
}

template<typename Path>
handle_type open_file(const Path& path, const access_mode mode, std::error_code& error)
{
    error.clear();
    if(detail::empty(path))
    {
        error = std::make_error_code(std::errc::invalid_argument);
        return INVALID_HANDLE_VALUE;
    }
#ifdef _WIN32
    const auto handle = ::CreateFile(c_str(path),
        mode == access_mode::read ? GENERIC_READ : GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        0,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        0);
#else
    const auto handle = ::open(c_str(path),
        mode == access_mode::read ? O_RDONLY : O_RDWR);
#endif
    if(handle == INVALID_HANDLE_VALUE)
    {
        error = last_error();
    }
    return handle;
}

inline int64_t query_file_size(handle_type handle, std::error_code& error)
{
    error.clear();
#ifdef _WIN32
    LARGE_INTEGER file_size;
    if(::GetFileSizeEx(handle, &file_size) == 0)
    {
        error = last_error();
        return 0;
    }
	return static_cast<int64_t>(file_size.QuadPart);
#else
    struct stat sbuf;
    if(::fstat(handle, &sbuf) == -1)
    {
        error = last_error();
        return 0;
    }
    return sbuf.st_size;
#endif
}

// -- basic_mmap --

template<typename CharT>
basic_mmap<CharT>::~basic_mmap()
{
    unmap();
}

template<typename CharT>
basic_mmap<CharT>::basic_mmap(basic_mmap<CharT>&& other)
    : data_(std::move(other.data_))
    , num_bytes_(std::move(other.num_bytes_))
    , num_mapped_bytes_(std::move(other.num_mapped_bytes_))
    , file_handle_(std::move(other.file_handle_))
#ifdef _WIN32
    , file_mapping_handle_(std::move(other.file_mapping_handle_))
#endif
    , is_handle_internal_(std::move(other.is_handle_internal_))
{
    other.data_ = nullptr;
    other.num_bytes_ = other.num_mapped_bytes_ = 0;
    other.file_handle_ = INVALID_HANDLE_VALUE;
#ifdef _WIN32
    other.file_mapping_handle_ = INVALID_HANDLE_VALUE;
#endif
}

template<typename CharT>
basic_mmap<CharT>& basic_mmap<CharT>::operator=(basic_mmap<CharT>&& other)
{
    if(this != &other)
    {
        // First the existing mapping needs to be removed.
        unmap();
        data_ = std::move(other.data_);
        num_bytes_ = std::move(other.num_bytes_);
        num_mapped_bytes_ = std::move(other.num_mapped_bytes_);
        file_handle_ = std::move(other.file_handle_);
#ifdef _WIN32
        file_mapping_handle_ = std::move(other.file_mapping_handle_);
#endif
        is_handle_internal_ = std::move(other.is_handle_internal_);

        // The moved from basic_mmap's fields need to be reset, because otherwise other's
        // destructor will unmap the same mapping that was just moved into this.
        other.data_ = nullptr;
        other.num_bytes_ = other.num_mapped_bytes_ = 0;
        other.file_handle_ = INVALID_HANDLE_VALUE;
#ifdef _WIN32
        other.file_mapping_handle_ = INVALID_HANDLE_VALUE;
#endif
        other.is_handle_internal_ = false;
    }
    return *this;
}

template<typename CharT>
typename basic_mmap<CharT>::handle_type basic_mmap<CharT>::mapping_handle() const noexcept
{
#ifdef _WIN32
    return file_mapping_handle_;
#else
    return file_handle_;
#endif
}

template<typename CharT>
template<typename String>
void basic_mmap<CharT>::map(String& path, size_type offset,
    size_type num_bytes, access_mode mode, std::error_code& error)
{
    error.clear();
    if(detail::empty(path))
    {
        error = std::make_error_code(std::errc::invalid_argument);
        return;
    }
    if(!is_open())
    {
        const auto handle = open_file(path, mode, error);
        if(error) { return; }
        map(handle, offset, num_bytes, mode, error);
        // MUST be after the call to map, as that sets this to true.
        is_handle_internal_ = true;
    }
}

template<typename CharT>
void basic_mmap<CharT>::map(handle_type handle, size_type offset,
    size_type num_bytes, access_mode mode, std::error_code& error)
{
    error.clear();
    if(handle == INVALID_HANDLE_VALUE)
    {
        error = std::make_error_code(std::errc::bad_file_descriptor);
        return;
    }

    const auto file_size = query_file_size(handle, error);
    if(error) { return; }

    if(num_bytes <= map_entire_file)
    {
        num_bytes = file_size;
    }
    else if(offset + num_bytes > file_size)
    {
        error = std::make_error_code(std::errc::invalid_argument);
        return;
    }

    file_handle_ = handle;
    is_handle_internal_ = false;
    map(offset, num_bytes, mode, error);
}

template<typename CharT>
void basic_mmap<CharT>::map(const size_type offset, const size_type num_bytes,
    const access_mode mode, std::error_code& error)
{
    const size_type aligned_offset = make_offset_page_aligned(offset);
    const size_type num_bytes_to_map = offset - aligned_offset + num_bytes;
#ifdef _WIN32
    const size_type max_file_size = offset + num_bytes;
    file_mapping_handle_ = ::CreateFileMapping(
        file_handle_,
        0,
        mode == access_mode::read ? PAGE_READONLY : PAGE_READWRITE,
        int64_high(max_file_size),
        int64_low(max_file_size),
        0);
    if(file_mapping_handle_ == INVALID_HANDLE_VALUE)
    {
        error = last_error();
        return;
    }

    const pointer mapping_start = static_cast<pointer>(::MapViewOfFile(
        file_mapping_handle_,
        mode == access_mode::read ? FILE_MAP_READ : FILE_MAP_WRITE,
        int64_high(aligned_offset),
        int64_low(aligned_offset),
        num_bytes_to_map));
    if(mapping_start == nullptr)
    {
        error = last_error();
        return;
    }
#else
    const pointer mapping_start = static_cast<pointer>(::mmap(
        0, // Don't give hint as to where to map.
        num_bytes_to_map,
        mode == access_mode::read ? PROT_READ : PROT_WRITE,
        MAP_SHARED,
        file_handle_,
        aligned_offset));
    if(mapping_start == MAP_FAILED)
    {
        error = last_error();
        return;
    }
#endif
    data_ = mapping_start + offset - aligned_offset;
    num_bytes_ = num_bytes;
    num_mapped_bytes_ = num_bytes_to_map;
}

template<typename CharT>
void basic_mmap<CharT>::sync(std::error_code& error)
{
    error.clear();
    if(!is_open())
    {
        error = std::make_error_code(std::errc::bad_file_descriptor);
        return;
    }

    if(data() != nullptr)
    {
#ifdef _WIN32
        if(::FlushViewOfFile(get_mapping_start(), num_mapped_bytes_) == 0
           || ::FlushFileBuffers(file_handle_) == 0)
#else
        if(::msync(get_mapping_start(), num_mapped_bytes_, MS_SYNC) != 0)
#endif
        {
            error = last_error();
            return;
        }
    }
#ifdef _WIN32
    if(::FlushFileBuffers(file_handle_) == 0)
    {
        error = last_error();
    }
#endif
}

template<typename CharT>
void basic_mmap<CharT>::unmap()
{
    if(!is_open()) { return; }
    // TODO do we care about errors here?
#ifdef _WIN32
    if(is_mapped())
    {
        ::UnmapViewOfFile(get_mapping_start());
        ::CloseHandle(file_mapping_handle_);
        file_mapping_handle_ = INVALID_HANDLE_VALUE;
    }
#else
    if(data_) { ::munmap(const_cast<pointer>(get_mapping_start()), num_mapped_bytes_); }
#endif

    // If file handle was obtained by our opening it (when map is called with a path,
    // rather than an existing file handle), we need to close it, otherwise it must not
    // be closed as it may still be used outside of this basic_mmap instance.
    if(is_handle_internal_)
    {
#ifdef _WIN32
        ::CloseHandle(file_handle_);
#else
        ::close(file_handle_);
#endif
    }

    // Reset fields to their default values.
    data_ = nullptr;
    num_bytes_ = num_mapped_bytes_ = 0;
    file_handle_ = INVALID_HANDLE_VALUE;
#ifdef _WIN32
    file_mapping_handle_ = INVALID_HANDLE_VALUE;
#endif
}

template<typename CharT>
typename basic_mmap<CharT>::pointer
basic_mmap<CharT>::get_mapping_start() noexcept
{
    if(!data()) { return nullptr; }
    return data() - offset();
}

template<typename CharT>
bool basic_mmap<CharT>::is_open() const noexcept
{
    return file_handle_ != INVALID_HANDLE_VALUE;
}

template<typename CharT>
bool basic_mmap<CharT>::is_mapped() const noexcept
{
#ifdef _WIN32
    return file_mapping_handle_ != INVALID_HANDLE_VALUE;
#else
    return is_open();
#endif
}

template<typename CharT>
void basic_mmap<CharT>::set_length(const size_type length) noexcept
{
    if(length > this->length() - offset()) { throw std::invalid_argument(""); } 
    num_bytes_ = to_byte_size(length);
}

template<typename CharT>
void basic_mmap<CharT>::set_offset(const size_type offset) noexcept
{
    if(offset >= mapped_length()) { throw std::invalid_argument(""); }
    const auto diff = offset - this->offset();
    data_ += diff;
    num_bytes_ += -1 * to_byte_size(diff);
}

template<typename CharT>
void basic_mmap<CharT>::swap(basic_mmap<CharT>& other)
{
    if(this != &other)
    {
        using std::swap;
        swap(data_, other.data_); 
        swap(file_handle_, other.file_handle_); 
#ifdef _WIN32
        swap(file_mapping_handle_, other.file_mapping_handle_); 
#endif
        swap(num_bytes_, other.num_bytes_); 
        swap(num_mapped_bytes_, other.num_mapped_bytes_); 
        swap(is_handle_internal_, other.is_handle_internal_); 
    }
}

template<typename CharT>
bool operator==(const basic_mmap<CharT>& a, const basic_mmap<CharT>& b)
{
    return a.data() == b.data()
        && a.size() == b.size();
}

template<typename CharT>
bool operator!=(const basic_mmap<CharT>& a, const basic_mmap<CharT>& b)
{
    return !(a == b);
}

template<typename CharT>
bool operator<(const basic_mmap<CharT>& a, const basic_mmap<CharT>& b)
{
    if(a.data() == b.data()) { return a.size() < b.size(); }
    return a.data() < b.data();
}

template<typename CharT>
bool operator<=(const basic_mmap<CharT>& a, const basic_mmap<CharT>& b)
{
    return !(a > b);
}

template<typename CharT>
bool operator>(const basic_mmap<CharT>& a, const basic_mmap<CharT>& b)
{
    if(a.data() == b.data()) { return a.size() > b.size(); }
    return a.data() > b.data();
}

template<typename CharT>
bool operator>=(const basic_mmap<CharT>& a, const basic_mmap<CharT>& b)
{
    return !(a < b);
}

} // namespace detail
} // namespace mio

#endif // MIO_BASIC_MMAP_IMPL
