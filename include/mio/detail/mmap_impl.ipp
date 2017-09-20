#ifndef MIO_BASIC_MMAP_IMPL
#define MIO_BASIC_MMAP_IMPL

#include "mmap_impl.hpp"

#include <algorithm>

#ifndef _WIN32
# include <unistd.h>
# include <fcntl.h>
# include <sys/mman.h>
# include <sys/stat.h>
#endif

namespace mio {
namespace detail {

#if defined(_WIN32)
inline DWORD int64_high(int64_t n) noexcept
{
    return n >> 32;
}

inline DWORD int64_low(int64_t n) noexcept
{
    return n & 0xff'ff'ff'ff;
}
#endif

inline size_t page_size()
{
    static const size_t page_size = []
    {
#ifdef _WIN32
        SYSTEM_INFO SystemInfo;
        GetSystemInfo(&SystemInfo);
        return SystemInfo.dwAllocationGranularity;
#else
        return sysconf(_SC_PAGE_SIZE);
#endif
    }();
    return page_size;
}

inline size_t make_page_aligned(size_t offset) noexcept
{
    const static size_t page_size_ = page_size();
    // use integer division to round down to the nearest page alignment
    return offset / page_size_ * page_size_;
}

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

// -- mmap --

inline mmap::~mmap()
{
    unmap();
}

inline mmap::mmap(mmap&& other)
    : data_(std::move(other.data_))
    , length_(std::move(other.length_))
    , mapped_length_(std::move(other.mapped_length_))
    , file_handle_(std::move(other.file_handle_))
#ifdef _WIN32
    , file_mapping_handle_(std::move(other.file_mapping_handle_))
#endif
{
    other.data_ = nullptr;
    other.length_ = other.mapped_length_ = 0;
    other.file_handle_ = INVALID_HANDLE_VALUE;
#ifdef _WIN32
    other.file_mapping_handle_ = INVALID_HANDLE_VALUE;
#endif
}

inline mmap& mmap::operator=(mmap&& other)
{
    if(this != &other)
    {
        data_ = std::move(other.data_);
        length_ = std::move(other.length_);
        mapped_length_ = std::move(other.mapped_length_);
        file_handle_ = std::move(other.file_handle_);
#ifdef _WIN32
        file_mapping_handle_ = std::move(other.file_mapping_handle_);
#endif
        other.data_ = nullptr;
        other.length_ = other.mapped_length_ = 0;
        other.file_handle_ = INVALID_HANDLE_VALUE;
#ifdef _WIN32
        other.file_mapping_handle_ = INVALID_HANDLE_VALUE;
#endif
    }
    return *this;
}

inline void mmap::map(const std::string& path, const size_type offset,
    const size_type length, const access_mode mode, std::error_code& error)
{
    if(!path.empty() && !is_open())
    {
        error.clear();
        const auto handle = open_file(path, mode, error);
        if(error) { return; }
        map(handle, offset, length, mode, error);
    }
}

inline mmap::handle_type mmap::open_file(const std::string& path,
    const mmap::access_mode mode, std::error_code& error)
{
#if defined(_WIN32)
    const auto handle = ::CreateFile(path.c_str(),
        mode == mmap::access_mode::read_only
            ? GENERIC_READ : GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        0,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        0);
    if(handle == INVALID_HANDLE_VALUE)
    {
        error = last_error();
    }
#else
    const auto handle = ::open(path.c_str(),
        mode == mmap::access_mode::read_only ? O_RDONLY : O_RDWR);
    if(handle == -1)
    {
        error = last_error();
    }
#endif
    return handle;
}

inline void mmap::map(const handle_type handle, const size_type offset,
    const size_type length, const access_mode mode, std::error_code& error)
{
    error.clear();
    if(handle == INVALID_HANDLE_VALUE)
    {
        error = std::make_error_code(std::errc::bad_file_descriptor);
        return;
    }

    file_handle_ = handle;

    const auto file_size = query_file_size(error);
    if(error) { return; }

    if(offset + length > file_size)
    {
        error = std::make_error_code(std::errc::invalid_argument);
        return;
    }

    map(offset, length, mode, error);
}

inline void mmap::map(const size_type offset, const size_type length,
    const access_mode mode, std::error_code& error)
{
    const size_type aligned_offset = make_page_aligned(offset);
    const size_type length_to_map = offset - aligned_offset + length;
#if defined(_WIN32)
    const size_type max_file_size = offset + length;
    file_mapping_handle_ = ::CreateFileMapping(
        file_handle_,
        0,
        mode == access_mode::read_only ? PAGE_READONLY : PAGE_READWRITE,
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
        mode == access_mode::read_only ? FILE_MAP_READ : FILE_MAP_WRITE,
        int64_high(aligned_offset),
        int64_low(aligned_offset),
        length_to_map));
    if(mapping_start == nullptr)
    {
        error = last_error();
        return;
    }
#else
    const pointer mapping_start = static_cast<pointer>(::mmap(
        0, // don't give hint as to where to map
        length_to_map,
        mode == mmap::access_mode::read_only ? PROT_READ : PROT_WRITE,
        MAP_SHARED, // TODO do we want to share it?
        file_handle_,
        aligned_offset));
    if(mapping_start == MAP_FAILED)
    {
        error = last_error();
        return;
    }
#endif
    data_ = mapping_start + offset - aligned_offset;
    length_ = length;
    mapped_length_ = length_to_map;
}

inline void mmap::sync(std::error_code& error)
{
    error.clear();
    verify_file_handle(error);
    if(error) { return; }

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

inline void mmap::unmap()
{
    // TODO do we care about errors here?
    if((data_ != nullptr) && (file_handle_ != INVALID_HANDLE_VALUE))
    {
#ifdef _WIN32
        ::UnmapViewOfFile(get_mapping_start());
        ::CloseHandle(file_mapping_handle_);
        file_mapping_handle_ = INVALID_HANDLE_VALUE;
#else
        ::munmap(const_cast<pointer>(get_mapping_start()), mapped_length_);
#endif
    }
    data_ = nullptr;
    length_ = mapped_length_ = 0;
    file_handle_ = INVALID_HANDLE_VALUE;
#ifdef _WIN32
    file_mapping_handle_ = INVALID_HANDLE_VALUE;
#endif
}

inline mmap::size_type mmap::query_file_size(std::error_code& error) noexcept
{
#ifdef _WIN32
    PLARGE_INTEGER file_size;
    if(::GetFileSizeEx(file_handle_, &file_size) == 0)
    {
        error = last_error();
        return 0;
    }
    return file_size;
#else
    struct stat sbuf;
    if(::fstat(file_handle_, &sbuf) == -1)
    {
        error = last_error();
        return 0;
    }
    return sbuf.st_size;
#endif
}

inline mmap::pointer mmap::get_mapping_start() noexcept
{
    if(!data_) { return nullptr; }
    const auto offset = mapped_length_ - length_;
    return data_ - offset;
}

inline void mmap::verify_file_handle(std::error_code& error) const noexcept
{
    if(!is_open() || !is_mapped())
    {
        error = std::make_error_code(std::errc::bad_file_descriptor);
    }
}

inline bool mmap::is_open() const noexcept
{
    return file_handle_ != INVALID_HANDLE_VALUE;
}

inline bool mmap::is_mapped() const noexcept
{
#ifdef _WIN32
    return file_mapping_handle_ != INVALID_HANDLE_VALUE;
#else
    return is_open();
#endif
}

inline void mmap::swap(mmap& other)
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
    }
}

inline bool operator==(const mmap& a, const mmap& b)
{
    if(a.is_mapped() && b.is_mapped())
    {
        return (a.size() == b.size()) && std::equal(a.begin(), a.end(), b.begin());
    }
    return !a.is_mapped() && !b.is_mapped();
}

inline bool operator!=(const mmap& a, const mmap& b)
{
    return !(a == b);
}

} // namespace detail
} // namespace mio

#endif // MIO_BASIC_MMAP_IMPL
