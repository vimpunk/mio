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

template<typename String>
file_handle_type open_file(const String& path,
    const access_mode mode, std::error_code& error)
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

inline int64_t query_file_size(file_handle_type handle, std::error_code& error)
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

struct mmap_context
{
    char* data;
    int64_t length;
    int64_t mapped_length;
#ifdef _WIN32
    file_handle_type file_mapping_handle;
#endif
};

mmap_context memory_map(const file_handle_type file_handle, const int64_t offset,
    const int64_t length, const access_mode mode, std::error_code& error)
{
    const int64_t aligned_offset = make_offset_page_aligned(offset);
    const int64_t length_to_map = offset - aligned_offset + length;
#ifdef _WIN32
    const int64_t max_file_size = offset + length;
    const auto file_mapping_handle = ::CreateFileMapping(
        file_handle,
        0,
        mode == access_mode::read ? PAGE_READONLY : PAGE_READWRITE,
        int64_high(max_file_size),
        int64_low(max_file_size),
        0);
    if(file_mapping_handle == INVALID_HANDLE_VALUE)
    {
        error = last_error();
        return {};
    }
    char* mapping_start = static_cast<char*>(::MapViewOfFile(
        file_mapping_handle,
        mode == access_mode::read ? FILE_MAP_READ : FILE_MAP_WRITE,
        int64_high(aligned_offset),
        int64_low(aligned_offset),
        length_to_map));
    if(mapping_start == nullptr)
    {
        error = last_error();
        return {};
    }
#else
    char* mapping_start = static_cast<char*>(::mmap(
        0, // Don't give hint as to where to map.
        length_to_map,
        mode == access_mode::read ? PROT_READ : PROT_WRITE,
        MAP_SHARED,
        file_handle,
        aligned_offset));
    if(mapping_start == MAP_FAILED)
    {
        error = last_error();
        return {};
    }
#endif
    mmap_context ctx;
    ctx.data = mapping_start + offset - aligned_offset;
    ctx.length = length;
    ctx.mapped_length = length_to_map;
#ifdef _WIN32
    ctx.file_mapping_handle = file_mapping_handle;
#endif
    return ctx;
}

// -- basic_mmap --

template<typename ByteT>
basic_mmap<ByteT>::~basic_mmap()
{
    unmap();
}

template<typename ByteT>
basic_mmap<ByteT>::basic_mmap(basic_mmap<ByteT>&& other)
    : data_(std::move(other.data_))
    , length_(std::move(other.length_))
    , mapped_length_(std::move(other.mapped_length_))
    , file_handle_(std::move(other.file_handle_))
#ifdef _WIN32
    , file_mapping_handle_(std::move(other.file_mapping_handle_))
#endif
    , is_handle_internal_(std::move(other.is_handle_internal_))
{
    other.data_ = nullptr;
    other.length_ = other.mapped_length_ = 0;
    other.file_handle_ = INVALID_HANDLE_VALUE;
#ifdef _WIN32
    other.file_mapping_handle_ = INVALID_HANDLE_VALUE;
#endif
}

template<typename ByteT>
basic_mmap<ByteT>& basic_mmap<ByteT>::operator=(basic_mmap<ByteT>&& other)
{
    if(this != &other)
    {
        // First the existing mapping needs to be removed.
        unmap();
        data_ = std::move(other.data_);
        length_ = std::move(other.length_);
        mapped_length_ = std::move(other.mapped_length_);
        file_handle_ = std::move(other.file_handle_);
#ifdef _WIN32
        file_mapping_handle_ = std::move(other.file_mapping_handle_);
#endif
        is_handle_internal_ = std::move(other.is_handle_internal_);

        // The moved from basic_mmap's fields need to be reset, because otherwise other's
        // destructor will unmap the same mapping that was just moved into this.
        other.data_ = nullptr;
        other.length_ = other.mapped_length_ = 0;
        other.file_handle_ = INVALID_HANDLE_VALUE;
#ifdef _WIN32
        other.file_mapping_handle_ = INVALID_HANDLE_VALUE;
#endif
        other.is_handle_internal_ = false;
    }
    return *this;
}

template<typename ByteT>
typename basic_mmap<ByteT>::handle_type basic_mmap<ByteT>::mapping_handle() const noexcept
{
#ifdef _WIN32
    return file_mapping_handle_;
#else
    return file_handle_;
#endif
}

template<typename ByteT>
template<typename String>
void basic_mmap<ByteT>::map(String& path, size_type offset,
    size_type length, access_mode mode, std::error_code& error)
{
    error.clear();
    if(detail::empty(path))
    {
        error = std::make_error_code(std::errc::invalid_argument);
        return;
    }
    const auto handle = open_file(path, mode, error);
    if(error) { return; }
    map(handle, offset, length, mode, error);
    // This MUST be after the call to map, as that sets this to true.
    if(!error)
    {
        is_handle_internal_ = true;
    }
}

template<typename ByteT>
void basic_mmap<ByteT>::map(handle_type handle, size_type offset,
    size_type length, access_mode mode, std::error_code& error)
{
    error.clear();
    if(handle == INVALID_HANDLE_VALUE)
    {
        error = std::make_error_code(std::errc::bad_file_descriptor);
        return;
    }

    const auto file_size = query_file_size(handle, error);
    if(error) { return; }

    if(length <= map_entire_file)
    {
        length = file_size;
    }
    else if(offset + length > file_size)
    {
        error = std::make_error_code(std::errc::invalid_argument);
        return;
    }

    const mmap_context ctx = memory_map(handle, offset, length, mode, error);
    if(!error)
    {
        // We must unmap the previous mapping that may have existed prior to this call.
        // Note that this must only be invoked after a new mapping has been created in
        // order to provide the strong guarantee that, should the new mapping fail, the
        // `map` function leaves this instance in a state as though the function had
        // never been invoked.
        unmap();
        file_handle_ = handle;
        is_handle_internal_ = false;
        data_ = reinterpret_cast<pointer>(ctx.data);
        length_ = ctx.length;
        mapped_length_ = ctx.mapped_length;
#ifdef _WIN32
        file_mapping_handle_ = ctx.file_mapping_handle;
#endif
    }
}

template<typename ByteT>
void basic_mmap<ByteT>::sync(std::error_code& error)
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
        if(::FlushViewOfFile(get_mapping_start(), mapped_length_) == 0
           || ::FlushFileBuffers(file_handle_) == 0)
#else
        if(::msync(get_mapping_start(), mapped_length_, MS_SYNC) != 0)
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

template<typename ByteT>
void basic_mmap<ByteT>::unmap()
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
    if(data_) { ::munmap(const_cast<pointer>(get_mapping_start()), mapped_length_); }
#endif

    // If file_handle_ was obtained by our opening it (when map is called with a path,
    // rather than an existing file handle), we need to close it, otherwise it must not
    // be closed as it may still be used outside this instance.
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
    length_ = mapped_length_ = 0;
    file_handle_ = INVALID_HANDLE_VALUE;
#ifdef _WIN32
    file_mapping_handle_ = INVALID_HANDLE_VALUE;
#endif
}

template<typename ByteT>
bool basic_mmap<ByteT>::is_mapped() const noexcept
{
#ifdef _WIN32
    return file_mapping_handle_ != INVALID_HANDLE_VALUE;
#else
    return is_open();
#endif
}

template<typename ByteT>
void basic_mmap<ByteT>::swap(basic_mmap<ByteT>& other)
{
    if(this != &other)
    {
        using std::swap;
        swap(data_, other.data_); 
        swap(file_handle_, other.file_handle_); 
#ifdef _WIN32
        swap(file_mapping_handle_, other.file_mapping_handle_); 
#endif
        swap(length_, other.length_); 
        swap(mapped_length_, other.mapped_length_); 
        swap(is_handle_internal_, other.is_handle_internal_); 
    }
}

template<typename ByteT>
bool operator==(const basic_mmap<ByteT>& a, const basic_mmap<ByteT>& b)
{
    return a.data() == b.data()
        && a.size() == b.size();
}

template<typename ByteT>
bool operator!=(const basic_mmap<ByteT>& a, const basic_mmap<ByteT>& b)
{
    return !(a == b);
}

template<typename ByteT>
bool operator<(const basic_mmap<ByteT>& a, const basic_mmap<ByteT>& b)
{
    if(a.data() == b.data()) { return a.size() < b.size(); }
    return a.data() < b.data();
}

template<typename ByteT>
bool operator<=(const basic_mmap<ByteT>& a, const basic_mmap<ByteT>& b)
{
    return !(a > b);
}

template<typename ByteT>
bool operator>(const basic_mmap<ByteT>& a, const basic_mmap<ByteT>& b)
{
    if(a.data() == b.data()) { return a.size() > b.size(); }
    return a.data() > b.data();
}

template<typename ByteT>
bool operator>=(const basic_mmap<ByteT>& a, const basic_mmap<ByteT>& b)
{
    return !(a < b);
}

} // namespace detail
} // namespace mio

#endif // MIO_BASIC_MMAP_IMPL
