#ifndef MIO_MMAP_HEADER
#define MIO_MMAP_HEADER

#include "detail/mmap_impl.hpp"
#include <system_error>

namespace mio {

/**
 * When specifying a file to map, there is no need to worry about providing
 * offsets that are aligned with the operating system's page granularity, this is taken
 * care of within the class in both cases.
 *
 * Both classes have single-ownership semantics, and transferring ownership may be
 * accomplished by moving the mmap instance.
 *
 * Remapping a file is possible, but unmap must be called before that.
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

    /**
     * `handle` must be a valid file handle, which is then used to memory map the
     * requested region. Upon failure a `std::error_code` is thrown, detailing the
     * cause of the error, and the object remains in an unmapped state.
     *
     * When specifying `offset`, there is no need to worry about providing
     * a value that is aligned with the operating system's page allocation granularity.
     * This is adjusted by the implementation such that the first requested byte (as
     * returned by `data` or `begin`), so long as `offset` is valid, will be at `offset`
     * from the start of the file.
     */
    mmap_source(const handle_type handle, const size_type offset, const size_type length)
    {
        std::error_code error;
        map(handle, offset, length, error);
        if(error) { throw error; }
    }

    /**
     * `mmap_source` has single-ownership semantics, so transferring ownership may only
     * be accomplished by moving the instance.
     */
    mmap_source(const mmap_source&) = delete;
    mmap_source(mmap_source&&) = default;

    /** The destructor invokes unmap. */
    ~mmap_source() = default;

    /**
     * On UNIX systems `is_open` and `is_mapped` are the same and don't actually say if
     * the file itself is open or closed, they only refer to the mapping. This is
     * because a mapping remains valid (as long as it's not unmapped) even if another
     * entity closes the file which is being mapped.
     *
     * On Windows, however, in order to map a file, both an active file handle and a
     * mapping handle is required, so `is_open` checks for a valid file handle, while
     * `is_mapped` checks for a valid mapping handle.
     */
    bool is_open() const noexcept { return impl_.is_open(); }
    bool is_mapped() const noexcept { return impl_.is_mapped(); }
    bool empty() const noexcept { return impl_.empty(); }

    /**
     * `size` and `length` both return the logical length (i.e. the one user requested),
     * while `mapped_length` returns the actual mapped length, which is usually a
     * multiple of the underlying operating system's page allocation granularity.
     */
    size_type size() const noexcept { return impl_.size(); }
    size_type length() const noexcept { return impl_.length(); }
    size_type mapped_length() const noexcept { return impl_.mapped_length(); }

    /**
     * Returns a pointer to the first requested byte, or `nullptr` if no memory mapping
     * exists.
     */
    const_pointer data() const noexcept { return impl_.data(); }

    /**
     * Returns an iterator to the first requested byte, if a valid memory mapping
     * exists, otherwise this function call is equivalent to invoking `end`.
     */
    const_iterator begin() const noexcept { return impl_.begin(); }
    const_iterator cbegin() const noexcept { return impl_.cbegin(); }

    /** Returns an iterator one past the last requested byte. */
    const_iterator end() const noexcept { return impl_.end(); }
    const_iterator cend() const noexcept { return impl_.cend(); }

    const_reverse_iterator rbegin() const noexcept { return impl_.rbegin(); }
    const_reverse_iterator crbegin() const noexcept { return impl_.crbegin(); }

    const_reverse_iterator rend() const noexcept { return impl_.rend(); }
    const_reverse_iterator crend() const noexcept { return impl_.crend(); }

    /**
     * Returns a reference to the `i`th byte from the first requested byte (as returned
     * by `data`). If this is invoked when no valid memory mapping has been established
     * prior to this call, undefined behaviour ensues.
     */
    const_reference operator[](const size_type i) const noexcept { return impl_[i]; }

    /**
     * Establishes a read-only memory mapping.
     *
     * `handle` must be a valid file handle, which is then used to memory map the
     * requested region. Upon failure, `error` is set to indicate the reason and the
     * object remains in an unmapped state.
     *
     * When specifying `offset`, there is no need to worry about providing
     * a value that is aligned with the operating system's page allocation granularity.
     * This is adjusted by the implementation such that the first requested byte (as
     * returned by `data` or `begin`), so long as `offset` is valid, will be at `offset`
     * from the start of the file.
     */
    void map(const handle_type handle, const size_type offset,
        const size_type length, std::error_code& error)
    {
        impl_.map(handle, offset, length, impl_type::access_mode::read_only, error);
    }

    /**
     * If a valid memory mapping has been established prior to this call, this call
     * instructs the kernel to unmap the memory region and dissasociate this object
     * from the file.
     */
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

    /**
     * `handle` must be a valid file handle, which is then used to memory map the
     * requested region. Upon failure a `std::error_code` is thrown, detailing the
     * cause of the error, and the object remains in an unmapped state.
     *
     * When specifying `offset`, there is no need to worry about providing
     * a value that is aligned with the operating system's page allocation granularity.
     * This is adjusted by the implementation such that the first requested byte (as
     * returned by `data` or `begin`), so long as `offset` is valid, will be at `offset`
     * from the start of the file.
     */
    mmap_sink(const handle_type handle, const size_type offset, const size_type length)
    {
        std::error_code error;
        map(handle, offset, length, error);
        if(error) { throw error; }
    }

    /**
     * `mmap_sink` has single-ownership semantics, so transferring ownership may only
     * be accomplished by moving the instance.
     */
    mmap_sink(const mmap_sink&) = delete;
    mmap_sink(mmap_sink&&) = default;

    /**
     * The destructor invokes unmap, but does NOT invoke `sync`. Thus, if changes have
     * been made to the mapped region, `sync` needs to be called in order to persist
     * any writes to disk.
     */
    ~mmap_sink() = default;

    /**
     * On UNIX systems `is_open` and `is_mapped` are the same and don't actually say if
     * the file itself is open or closed, they only refer to the mapping. This is
     * because a mapping remains valid (as long as it's not unmapped) even if another
     * entity closes the file which is being mapped.
     *
     * On Windows, however, in order to map a file, both an active file handle and a
     * mapping handle is required, so `is_open` checks for a valid file handle, while
     * `is_mapped` checks for a valid mapping handle.
     */
    bool is_open() const noexcept { return impl_.is_open(); }
    bool is_mapped() const noexcept { return impl_.is_mapped(); }
    bool empty() const noexcept { return impl_.empty(); }

    /**
     * `size` and `length` both return the logical length (i.e. the one user requested),
     * while `mapped_length` returns the actual mapped length, which is usually a
     * multiple of the underlying operating system's page allocation granularity.
     */
    size_type size() const noexcept { return impl_.size(); }
    size_type length() const noexcept { return impl_.length(); }
    size_type mapped_length() const noexcept { return impl_.mapped_length(); }

    /**
     * Returns a pointer to the first requested byte, or `nullptr` if no memory mapping
     * exists.
     */
    pointer data() noexcept { return impl_.data(); }
    const_pointer data() const noexcept { return impl_.data(); }

    /**
     * Returns an iterator to the first requested byte, if a valid memory mapping
     * exists, otherwise this function call is equivalent to invoking `end`.
     */
    iterator begin() noexcept { return impl_.begin(); }
    const_iterator begin() const noexcept { return impl_.begin(); }
    const_iterator cbegin() const noexcept { return impl_.cbegin(); }

    /** Returns an iterator one past the last requested byte. */
    iterator end() noexcept { return impl_.end(); }
    const_iterator end() const noexcept { return impl_.end(); }
    const_iterator cend() const noexcept { return impl_.cend(); }

    reverse_iterator rbegin() noexcept { return impl_.rbegin(); }
    const_reverse_iterator rbegin() const noexcept { return impl_.rbegin(); }
    const_reverse_iterator crbegin() const noexcept { return impl_.crbegin(); }

    reverse_iterator rend() noexcept { return impl_.rend(); }
    const_reverse_iterator rend() const noexcept { return impl_.rend(); }
    const_reverse_iterator crend() const noexcept { return impl_.crend(); }

    /**
     * Returns a reference to the `i`th byte from the first requested byte (as returned
     * by `data`). If this is invoked when no valid memory mapping has been established
     * prior to this call, undefined behaviour ensues.
     */
    reference operator[](const size_type i) noexcept { return impl_[i]; }
    const_reference operator[](const size_type i) const noexcept { return impl_[i]; }

    /**
     * Establishes a read-only memory mapping.
     *
     * `handle` must be a valid file handle, which is then used to memory map the
     * requested region. Upon failure, `error` is set to indicate the reason and the
     * object remains in an unmapped state.
     *
     * When specifying `offset`, there is no need to worry about providing
     * a value that is aligned with the operating system's page allocation granularity.
     * This is adjusted by the implementation such that the first requested byte (as
     * returned by `data` or `begin`), so long as `offset` is valid, will be at `offset`
     * from the start of the file.
     */
    void map(const handle_type handle, const size_type offset,
        const size_type length, std::error_code& error)
    {
        impl_.map(handle, offset, length, impl_type::access_mode::read_write, error);
    }

    /**
     * If a valid memory mapping has been established prior to this call, this call
     * instructs the kernel to unmap the memory region and dissasociate this object
     * from the file.
     */
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

/**
 * Since `mmap_source` works on the file descriptor/handle level of abstraction, a
 * factory method is provided for the case when a file needs to be mapped using a file
 * path.
 *
 * Path may be `std::string`, `std::string_view`, `const char*`,
 * `std::filesystem::path`, `std::vector<char>`, or similar.
 */
template<typename Path>
mmap_source make_mmap_source(const Path& path, mmap_source::size_type offset,
    mmap_source::size_type length, std::error_code& error)
{
    const auto handle = detail::open_file(path,
        detail::mmap::access_mode::read_only, error);
    mmap_source mmap;
    if(!error) { mmap.map(handle, offset, length, error); }
    return mmap;
}

/**
 * Since `mmap_sink` works on the file descriptor/handle level of abstraction, a factory
 * method is provided for the case when a file needs to be mapped using a file path.
 *
 * Path may be `std::string`, `std::string_view`, `const char*`,
 * `std::filesystem::path`, `std::vector<char>`, or similar.
 */
template<typename Path>
mmap_sink make_mmap_sink(const Path& path, mmap_sink::size_type offset,
    mmap_sink::size_type length, std::error_code& error)
{
    const auto handle = detail::open_file(path,
        detail::mmap::access_mode::read_write, error);
    mmap_sink mmap;
    if(!error) { mmap.map(handle, offset, length, error); }
    return mmap;
}

} // namespace mio

#endif // MIO_MMAP_HEADER
