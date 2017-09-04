#ifndef MIO_MMAP_HEADER
#define MIO_MMAP_HEADER

#include "detail/basic_mmap.hpp"

namespace mio {

/**
 * When specifying a file to map, there is no need to worry about providing
 * offsets that are aligned with the operating system's page granularity, this is taken
 * care of within the class in both cases.
 *
 * Both classes have std::unique_ptr<> semantics, thus only a single entity may own
 * a mapping to a file at any given time.
 *
 * Remapping a file is possible, but unmap must be called before that.
 *
 * For now, both classes may only be used with an existing open file by providing the 
 * file's handle.
 *
 * Both classes' destructors unmap the file. However, mmap_sink's destructor does not
 * sync the mapped file view to disk, this has to be done manually with a call tosink.
 */

/** A read-only file memory mapping. */
template<typename CharT> struct basic_mmap_source
{
    using impl_type = detail::basic_mmap<CharT>;
    using value_type = typename impl_type::value_type;
    using size_type = typename impl_type::size_type;
    using reference = typename impl_type::reference;
    using const_reference = typename impl_type::const_reference;
    using pointer = typename impl_type::pointer;
    using const_pointer = typename impl_type::const_pointer;
    using difference_type = typename impl_type::difference_type;
    using iterator = typename impl_type::iterator;
    using const_iterator = typename impl_type::const_iterator;
    //using reverse_iterator = std::reverse_iterator<iterator>;
    //using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    using iterator_category = typename impl_type::iterator_category;
    using handle_type = typename impl_type::handle_type;

private:
    impl_type impl_;
public:

    basic_mmap_source() = default;
    basic_mmap_source(const std::string& path,
        const size_type offset, const size_type length);
    basic_mmap_source(const handle_type handle,
        const size_type offset, const size_type length);

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
    bool is_open() const noexcept { return impl_.is_open(); }
    bool is_mapped() const noexcept { return impl_.is_mapped(); }
    bool empty() const noexcept { return impl_.is_empty(); }

    /**
     * size/length returns the logical length (i.e. the one user requested), while
     * mapped_length returns the actual mapped length, which is usually a multiple of
     * the OS' page size.
     */
    size_type size() const noexcept { return impl_.size(); }
    size_type length() const noexcept { return impl_.length(); }
    size_type mapped_length() const noexcept { return impl_.mapped_length(); }

    const_pointer data() const noexcept { return impl_.data(); }

    const_iterator begin() const noexcept { return impl_.begin(); }
    const_iterator cbegin() const noexcept { return impl_.cbegin(); }
    const_iterator end() const noexcept { return impl_.end(); }
    const_iterator cend() const noexcept { return impl_.cend(); }

    const_reference operator[](const size_type i) const noexcept { return impl_[i]; }

    void map(const std::string& path, const size_type offset,
        const size_type length, std::error_code& error);
    void map(const handle_type handle, const size_type offset,
        const size_type length, std::error_code& error);

    void unmap() { impl_.unmap(); }
};

/** A read-write file memory mapping. */
template<typename CharT> struct basic_mmap_sink
{
    using impl_type = detail::basic_mmap<CharT>;
    using value_type = typename impl_type::value_type;
    using size_type = typename impl_type::size_type;
    using reference = typename impl_type::reference;
    using const_reference = typename impl_type::const_reference;
    using pointer = typename impl_type::pointer;
    using const_pointer = typename impl_type::const_pointer;
    using difference_type = typename impl_type::difference_type;
    using iterator = typename impl_type::iterator;
    using const_iterator = typename impl_type::const_iterator;
    //using reverse_iterator = std::reverse_iterator<iterator>;
    //using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    using iterator_category = typename impl_type::iterator_category;
    using handle_type = typename impl_type::handle_type;

private:
    impl_type impl_;
public:

    basic_mmap_sink() = default;
    basic_mmap_sink(const std::string& path,
        const size_type offset, const size_type length);
    basic_mmap_sink(const handle_type handle,
        const size_type offset, const size_type length);

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
    bool is_open() const noexcept { return impl_.is_open(); }
    bool is_mapped() const noexcept { return impl_.is_mapped(); }
    bool empty() const noexcept { return impl_.is_empty(); }

    /**
     * size/length returns the logical length (i.e. the one user requested), while
     * mapped_length returns the actual mapped length, which is usually a multiple of
     * the OS' page size.
     */
    size_type size() const noexcept { return impl_.size(); }
    size_type length() const noexcept { return impl_.length(); }
    size_type mapped_length() const noexcept { return impl_.mapped_length(); }

    pointer data() noexcept { return impl_.data(); }
    const_pointer data() const noexcept { return impl_.data(); }

    iterator begin() noexcept;
    const_iterator begin() const noexcept { return impl_.begin(); }
    const_iterator cbegin() const noexcept { return impl_.cbegin(); }

    iterator end() noexcept;
    const_iterator end() const noexcept { return impl_.end(); }
    const_iterator cend() const noexcept { return impl_.cend(); }

    reference operator[](const size_type i) noexcept { return impl_[i]; }
    const_reference operator[](const size_type i) const noexcept { return impl_[i]; }

    void map(const std::string& path, const size_type offset,
        const size_type length, std::error_code& error);
    void map(const handle_type handle, const size_type offset,
        const size_type length, std::error_code& error);

    /** Flushes the memory mapped page to disk. */
    void sync(std::error_code& error);

    void unmap() { impl_.unmap(); }
};

using mmap_sink = basic_mmap_sink<char>;
using mmap_source = basic_mmap_source<char>;

using wmmap_sink = basic_mmap_sink<wchar_t>;
using wmmap_source = basic_mmap_source<wchar_t>;

using u16mmap_sink = basic_mmap_sink<char16_t>;
using u16mmap_source = basic_mmap_source<char16_t>;

using u32mmap_sink = basic_mmap_sink<char32_t>;
using u32mmap_source = basic_mmap_source<char32_t>;

} // namespace mio

#include "detail/mmap.ipp"

#endif // MIO_MMAP_HEADER
