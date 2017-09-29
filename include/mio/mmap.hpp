#ifndef MIO_MMAP_HEADER
#define MIO_MMAP_HEADER

#include "detail/mmap_impl.hpp"

#include <system_error>

namespace mio {

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

    /**
     * This value may be provided as the `length` parameter to the constructor or
     * `map`, in which case a memory mapping of the entire file is created.
     */
    static constexpr size_type use_full_file_size = impl_type::use_full_file_size;

    /**
     * The default constructed mmap object is in a non-mapped state, that is, any
     * operations that attempt to access nonexistent underlying date will result in
     * undefined behaviour/segmentation faults.
     */
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
     * This class has single-ownership semantics, so transferring ownership may only be
     * accomplished by moving the object.
     */
    mmap_source(mmap_source&&) = default;
    mmap_source& operator=(mmap_source&&) = default;

    /** The destructor invokes unmap. */
    ~mmap_source() = default;

    /**
     * On UNIX systems 'file_handle' and 'mapping_handle' are the same. On Windows,
     * however, a mapped region of a file gets its own handle, which is returned by
     * 'mapping_handle'.
     */
    handle_type file_handle() const noexcept { return impl_.file_handle(); }
    handle_type mapping_handle() const noexcept { return impl_.mapping_handle(); }

    /** Returns whether a valid memory mapping has been created. */
    bool is_open() const noexcept { return impl_.is_open(); }

    /**
     * Returns if the length that was mapped was 0, in which case no mapping was
     * established, i.e. `is_open` returns false. This function is provided so that
     * this class has some Container semantics.
     */
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
     * by `data`). If this is invoked when no valid memory mapping has been created
     * prior to this call, undefined behaviour ensues.
     */
    const_reference operator[](const size_type i) const noexcept { return impl_[i]; }

    /**
     * Establishes a read-only memory mapping. If the mapping is unsuccesful, the
     * reason is reported via `error` and the object remains in a state as if this
     * function hadn't been called.
     *
     * `path`, which must be a path to an existing file, is used to retrieve a file
     * handle (which is closed when the object destructs or `unmap` is called), which is
     * then used to memory map the requested region. Upon failure, `error` is set to
     * indicate the reason and the object remains in an unmapped state.
     *
     * When specifying `offset`, there is no need to worry about providing
     * a value that is aligned with the operating system's page allocation granularity.
     * This is adjusted by the implementation such that the first requested byte (as
     * returned by `data` or `begin`), so long as `offset` is valid, will be at `offset`
     * from the start of the file.
     *
     * If `length` is `use_full_file_size`, a mapping of the entire file is created.
     */
    template<typename String>
    void map(const String& path, const size_type offset,
        const size_type length, std::error_code& error)
    {
        impl_.map(path, offset, length, impl_type::access_mode::read_only, error);
    }

    /**
     * Establishes a read-only memory mapping. If the mapping is unsuccesful, the
     * reason is reported via `error` and the object remains in a state as if this
     * function hadn't been called.
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
     *
     * If `length` is `use_full_file_size`, a mapping of the entire file is created.
     */
    void map(const handle_type handle, const size_type offset,
        const size_type length, std::error_code& error)
    {
        impl_.map(handle, offset, length, impl_type::access_mode::read_only, error);
    }

    /**
     * If a valid memory mapping has been created prior to this call, this call
     * instructs the kernel to unmap the memory region and disassociate this object
     * from the file.
     *
     * The file handle associated with the file that is mapped is only closed if the
     * mapping was created using a file path. If, on the other hand, an existing
     * file handle was used to create the mapping, the file handle is not closed.
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
    
    /**
     * This value may be provided as the `length` parameter to the constructor or
     * `map`, in which case a memory mapping of the entire file is created.
     */
    static constexpr size_type use_full_file_size = impl_type::use_full_file_size;

    /**
     * The default constructed mmap object is in a non-mapped state, that is, any
     * operations that attempt to access nonexistent underlying date will result in
     * undefined behaviour/segmentation faults.
     */
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
     * This class has single-ownership semantics, so transferring ownership may only be
     * accomplished by moving the object.
     */
    mmap_sink(mmap_sink&&) = default;
    mmap_sink& operator=(mmap_sink&&) = default;

    /**
     * The destructor invokes unmap, but does NOT invoke `sync`. Thus, if the mapped
     * region has been written to, `sync` needs to be called in order to persist the
     * changes to disk.
     */
    ~mmap_sink() = default;

    /**
     * On UNIX systems 'file_handle' and 'mapping_handle' are the same. On Windows,
     * however, a mapped region of a file gets its own handle, which is returned by
     * 'mapping_handle'.
     */
    handle_type file_handle() const noexcept { return impl_.file_handle(); }
    handle_type mapping_handle() const noexcept { return impl_.mapping_handle(); }

    /** Returns whether a valid memory mapping has been created. */
    bool is_open() const noexcept { return impl_.is_open(); }

    /**
     * Returns if the length that was mapped was 0, in which case no mapping was
     * established, i.e. `is_open` returns false. This function is provided so that
     * this class has some Container semantics.
     */
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
     * by `data`). If this is invoked when no valid memory mapping has been created
     * prior to this call, undefined behaviour ensues.
     */
    reference operator[](const size_type i) noexcept { return impl_[i]; }
    const_reference operator[](const size_type i) const noexcept { return impl_[i]; }

    /**
     * Establishes a read-write memory mapping. If the mapping is unsuccesful, the
     * reason is reported via `error` and the object remains in a state as if this
     * function hadn't been called.
     *
     * `path`, which must be a path to an existing file, is used to retrieve a file
     * handle (which is closed when the object destructs or `unmap` is called), which is
     * then used to memory map the requested region. Upon failure, `error` is set to
     * indicate the reason and the object remains in an unmapped state.
     *
     * When specifying `offset`, there is no need to worry about providing
     * a value that is aligned with the operating system's page allocation granularity.
     * This is adjusted by the implementation such that the first requested byte (as
     * returned by `data` or `begin`), so long as `offset` is valid, will be at `offset`
     * from the start of the file.
     *
     * If `length` is `use_full_file_size`, a mapping of the entire file is created.
     */
    template<typename String>
    void map(const String& path, const size_type offset,
        const size_type length, std::error_code& error)
    {
        impl_.map(path, offset, length, impl_type::access_mode::read_only, error);
    }

    /**
     * Establishes a read-write memory mapping. If the mapping is unsuccesful, the
     * reason is reported via `error` and the object remains in a state as if this
     * function hadn't been called.
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
     *
     * If `length` is `use_full_file_size`, a mapping of the entire file is created.
     */
    void map(const handle_type handle, const size_type offset,
        const size_type length, std::error_code& error)
    {
        impl_.map(handle, offset, length, impl_type::access_mode::read_write, error);
    }

    /**
     * If a valid memory mapping has been created prior to this call, this call
     * instructs the kernel to unmap the memory region and disassociate this object
     * from the file.
     *
     * The file handle associated with the file that is mapped is only closed if the
     * mapping was created using a file path. If, on the other hand, an existing
     * file handle was used to create the mapping, the file handle is not closed.
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
 * Convenience factory method.
 *
 * MappingToken may be a String (`std::string`, `std::string_view`, `const char*`,
 * `std::filesystem::path`, `std::vector<char>`, or similar), or a
 * mmap_source::file_handle.
 */
template<typename MappingToken>
mmap_source make_mmap_source(const MappingToken& token, mmap_source::size_type offset,
    mmap_source::size_type length, std::error_code& error)
{
    mmap_source mmap;
    mmap.map(token, offset, length, error);
    return mmap;
}

/**
 * Convenience factory method.
 *
 * MappingToken may be a String (`std::string`, `std::string_view`, `const char*`,
 * `std::filesystem::path`, `std::vector<char>`, or similar), or a
 * mmap_sink::file_handle.
 */
template<typename MappingToken>
mmap_sink make_mmap_sink(const MappingToken& token, mmap_sink::size_type offset,
    mmap_sink::size_type length, std::error_code& error)
{
    mmap_sink mmap;
    mmap.map(token, offset, length, error);
    return mmap;
}

} // namespace mio

#endif // MIO_MMAP_HEADER
