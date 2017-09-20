#ifndef MIO_BASIC_MMAP_HEADER
#define MIO_BASIC_MMAP_HEADER

#include <iterator>
#include <string>

#ifdef _WIN32
# ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
# endif // WIN32_LEAN_AND_MEAN
# include <windows.h>
#else // ifdef _WIN32
# define INVALID_HANDLE_VALUE -1
#endif // ifdef _WIN32

namespace mio {
namespace detail {

size_t page_size();

struct mmap
{
    using value_type = char;
    using size_type = int64_t;
    using reference = value_type&;
    using const_reference = const value_type&;
    using pointer = value_type*;
    using const_pointer = const value_type*;
    using difference_type = std::ptrdiff_t;
    using iterator = pointer;
    using const_iterator = const_pointer;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    using iterator_category = std::random_access_iterator_tag;
#ifdef _WIN32
    using handle_type = HANDLE;
#else
    using handle_type = int;
#endif

    enum class access_mode
    {
        read_only,
        read_write
    };

private:

    // Points to the first requested byte, and not to the actual start of the mapping.
    pointer data_ = nullptr;

    // On POSIX, we only need a file handle to create a mapping, while on Windows
    // systems the file handle is necessary to retrieve a file mapping handle, but any
    // subsequent operations on the mapped region must be done through the latter.
    handle_type file_handle_ = INVALID_HANDLE_VALUE;
#if defined(_WIN32)
    handle_type file_mapping_handle_ = INVALID_HANDLE_VALUE;
#endif

    // Length, in bytes, requested by user, which may not be the length of the full
    // mapping.
    size_type length_ = 0;
    size_type mapped_length_ = 0;

public:

    mmap() = default;
    mmap(const mmap&) = delete;
    mmap& operator=(const mmap&) = delete;
    mmap(mmap&&);
    mmap& operator=(mmap&&);
    ~mmap();

    /**
     * On *nix systems is_open and is_mapped are the same and don't actually say if
     * the file itself is open or closed, they only refer to the mapping. This is
     * because a mapping remains valid (as long as it's not unmapped) even if another
     * entity closes the file which is being mapped.
     *
     * On Windows, however, in order to map a file, both an active file handle and a
     * mapping handle is required, so is_open checks for a valid file handle, while
     * is_mapped checks for a valid mapping handle.
     */
    bool is_open() const noexcept;
    bool is_mapped() const noexcept;
    bool empty() const noexcept { return length() == 0; }

    /**
     * size/length returns the logical length (i.e. the one user requested), while
     * mapped_length returns the actual mapped length, which is usually a multiple of
     * the OS' page size.
     */
    size_type size() const noexcept { return length_; }
    size_type length() const noexcept { return length_; }
    size_type mapped_length() const noexcept { return mapped_length_; }

    pointer data() noexcept { return data_; }
    const_pointer data() const noexcept { return data_; }

    iterator begin() noexcept { return data(); }
    const_iterator begin() const noexcept { return data(); }
    const_iterator cbegin() const noexcept { return data(); }

    iterator end() noexcept { return begin() + length(); }
    const_iterator end() const noexcept { return begin() + length(); }
    const_iterator cend() const noexcept { return begin() + length(); }

    reverse_iterator rbegin() { return reverse_iterator(end()); }
    const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
    const_reverse_iterator crbegin() const { return const_reverse_iterator(end()); }

    reverse_iterator rend() { return reverse_iterator(begin()); }
    const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }
    const_reverse_iterator crend() const { return const_reverse_iterator(begin()); }

    reference operator[](const size_type i) noexcept { return data_[i]; }
    const_reference operator[](const size_type i) const noexcept { return data_[i]; }

    void map(const std::string& path, const size_type offset, const size_type length,
        const access_mode mode, std::error_code& error);
    void map(const handle_type handle, const size_type offset, const size_type length,
        const access_mode mode, std::error_code& error);
    void unmap();

    void sync(std::error_code& error);

    void swap(mmap& other);

    friend bool operator==(const mmap& a, const mmap& b);
    friend bool operator!=(const mmap& a, const mmap& b);

private:

    static handle_type open_file(const std::string& path,
        const access_mode mode, std::error_code& error);

    pointer get_mapping_start() noexcept;

    /** NOTE: file_handle_ must be valid. */
    size_type query_file_size(std::error_code& error) noexcept;

    void map(const size_type offset, const size_type length,
        const access_mode mode, std::error_code& error);

    void verify_file_handle(std::error_code& error) const noexcept;
};

} // namespace detail
} // namespace mio

#include "mmap_impl.ipp"

#endif // MIO_BASIC_MMAP_HEADER
