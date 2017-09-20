#ifndef MIO_MMAP_HEADER
#define MIO_MMAP_HEADER

#include "detail/mmap_impl.hpp"

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
class mmap_source
{
    using impl_type = detail::mmap;
    impl_type impl_;

public:

    using value_type = impl_type::value_type;
    using size_type = impl_type::size_type;
    using reference = impl_type::reference;
    using const_reference = impl_type::const_reference;
    using pointer = impl_type::pointer;
    using const_pointer = impl_type::const_pointer;
    using difference_type = impl_type::difference_type;
    using iterator = impl_type::iterator;
    using const_iterator = impl_type::const_iterator;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    using iterator_category = impl_type::iterator_category;
    using handle_type = impl_type::handle_type;

    mmap_source() = default;
    mmap_source(const std::string& path, const size_type offset, const size_type length)
    {
        std::error_code error;
        map(path, offset, length, error);
        if(error) { throw error; }
    }

    mmap_source(const handle_type handle, const size_type offset, const size_type length)
    {
        std::error_code error;
        map(handle, offset, length, error);
        if(error) { throw error; }
    }

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
    bool empty() const noexcept { return impl_.empty(); }

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

    const_reverse_iterator rbegin() const noexcept { return impl_.rbegin(); }
    const_reverse_iterator crbegin() const noexcept { return impl_.crbegin(); }
    const_reverse_iterator rend() const noexcept { return impl_.rend(); }
    const_reverse_iterator crend() const noexcept { return impl_.crend(); }

    const_reference operator[](const size_type i) const noexcept { return impl_[i]; }

    void map(const std::string& path, const size_type offset,
        const size_type length, std::error_code& error)
    {
        impl_.map(path, offset, length, impl_type::access_mode::read_only, error);
    }

    void map(const handle_type handle, const size_type offset,
        const size_type length, std::error_code& error)
    {
        impl_.map(handle, offset, length, impl_type::access_mode::read_only, error);
    }

    void unmap() { impl_.unmap(); }

    void swap(mmap_source& other) { impl_.swap(other.impl_); }

    friend bool operator==(const mmap_source& a, const mmap_source& b)
    {
        return a.impl_ == b.impl_;
    }

    friend bool operator!=(const mmap_source& a, const mmap_source& b)
    {
        return !(a == b);
    }
};

/** A read-write file memory mapping. */
class mmap_sink
{
    using impl_type = detail::mmap;
    impl_type impl_;

public:

    using value_type = impl_type::value_type;
    using size_type = impl_type::size_type;
    using reference = impl_type::reference;
    using const_reference = impl_type::const_reference;
    using pointer = impl_type::pointer;
    using const_pointer = impl_type::const_pointer;
    using difference_type = impl_type::difference_type;
    using iterator = impl_type::iterator;
    using const_iterator = impl_type::const_iterator;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    using iterator_category = impl_type::iterator_category;
    using handle_type = impl_type::handle_type;

    mmap_sink() = default;
    mmap_sink(const std::string& path, const size_type offset, const size_type length)
    {
        std::error_code error;
        map(path, offset, length, error);
        if(error) { throw error; }
    }

    mmap_sink(const handle_type handle, const size_type offset, const size_type length)
    {
        std::error_code error;
        map(handle, offset, length, error);
        if(error) { throw error; }
    }

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
    bool empty() const noexcept { return impl_.empty(); }

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

    iterator begin() noexcept { return impl_.begin(); }
    const_iterator begin() const noexcept { return impl_.begin(); }
    const_iterator cbegin() const noexcept { return impl_.cbegin(); }

    iterator end() noexcept { return impl_.end(); }
    const_iterator end() const noexcept { return impl_.end(); }
    const_iterator cend() const noexcept { return impl_.cend(); }

    reverse_iterator rbegin() noexcept { return impl_.rbegin(); }
    const_reverse_iterator rbegin() const noexcept { return impl_.rbegin(); }
    const_reverse_iterator crbegin() const noexcept { return impl_.crbegin(); }

    reverse_iterator rend() noexcept { return impl_.rend(); }
    const_reverse_iterator rend() const noexcept { return impl_.rend(); }
    const_reverse_iterator crend() const noexcept { return impl_.crend(); }

    reference operator[](const size_type i) noexcept { return impl_[i]; }
    const_reference operator[](const size_type i) const noexcept { return impl_[i]; }

    void map(const std::string& path, const size_type offset,
        const size_type length, std::error_code& error)
    {
        impl_.map(path, offset, length, impl_type::access_mode::read_only, error);
    }

    void map(const handle_type handle, const size_type offset,
        const size_type length, std::error_code& error)
    {
        impl_.map(handle, offset, length, impl_type::access_mode::read_write, error);
    }

    void unmap() { impl_.unmap(); }

    /** Flushes the memory mapped page to disk. */
    void sync(std::error_code& error) { impl_.sync(error); }

    void swap(mmap_sink& other) { impl_.swap(other.impl_); }

    friend bool operator==(const mmap_sink& a, const mmap_sink& b)
    {
        return a.impl_ == b.impl_;
    }

    friend bool operator!=(const mmap_sink& a, const mmap_sink& b)
    {
        return !(a == b);
    }
};

} // namespace mio

#endif // MIO_MMAP_HEADER
