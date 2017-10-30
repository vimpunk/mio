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

#ifndef MIO_MMAP_HEADER
#define MIO_MMAP_HEADER

#include "detail/basic_mmap.hpp"

#include <system_error>

namespace mio {

// This is used by basic_mmap to determine whether to create a read-only or a read-write
// memory mapping. The two possible values are `read` and `write`.
using detail::access_mode;

// This value may be provided as the `length` parameter to the constructor or
// `map`, in which case a memory mapping of the entire file is created.
using detail::map_entire_file;

template<
    access_mode AccessMode,
    typename ByteT
> class basic_mmap
{
    static_assert(AccessMode == access_mode::read
        || AccessMode == access_mode::write,
        "AccessMode must be either read or write");

    using impl_type = detail::basic_mmap<ByteT>;
    impl_type impl_;

public:

    using value_type = typename impl_type::value_type;
    using size_type = typename impl_type::size_type;
    using reference = typename impl_type::reference;
    using const_reference = typename impl_type::const_reference;
    using pointer = typename impl_type::pointer;
    using const_pointer = typename impl_type::const_pointer;
    using difference_type = typename impl_type::difference_type;
    using iterator = typename impl_type::iterator;
    using const_iterator = typename impl_type::const_iterator;
    using reverse_iterator = typename impl_type::reverse_iterator;
    using const_reverse_iterator = typename impl_type::const_reverse_iterator;
    using iterator_category = typename impl_type::iterator_category;
    using handle_type = typename impl_type::handle_type;

    /**
     * The default constructed mmap object is in a non-mapped state, that is, any
     * operation that attempts to access nonexistent underlying data will result in
     * undefined behaviour/segmentation faults.
     */
    basic_mmap() = default;

    /**
     * The same as invoking the `map` function, except any error that may occur while
     * establishing the mapping is thrown.
     */
    template<typename String>
    basic_mmap(const String& path, const size_type offset, const size_type length)
    {
        std::error_code error;
        map(path, offset, length, error);
        if(error) { throw error; }
    }

    /**
     * The same as invoking the `map` function, except any error that may occur while
     * establishing the mapping is thrown.
     */
    basic_mmap(const handle_type handle, const size_type offset, const size_type length)
    {
        std::error_code error;
        map(handle, offset, length, error);
        if(error) { throw error; }
    }

    /**
     * This class has single-ownership semantics, so transferring ownership may only be
     * accomplished by moving the object.
     */
    basic_mmap(basic_mmap&&) = default;
    basic_mmap& operator=(basic_mmap&&) = default;

    /** The destructor invokes unmap. */
    ~basic_mmap() = default;

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
     * `size` and `length` both return the logical length, i.e. the number of bytes
     * user requested to be mapped, while `mapped_length` returns the actual number of
     * bytes that were mapped which is a multiple of the underlying operating system's
     * page allocation granularity.
     */
    size_type size() const noexcept { return impl_.length(); }
    size_type length() const noexcept { return impl_.length(); }
    size_type mapped_length() const noexcept { return impl_.mapped_length(); }

    /**
     * Returns the offset, relative to the file's start, at which the mapping was
     * requested to be created.
     */
    size_type offset() const noexcept { return impl_.offset(); }

    /**
     * Returns a pointer to the first requested byte, or `nullptr` if no memory mapping
     * exists.
     */
    template<
        access_mode A = AccessMode,
        typename = typename std::enable_if<A == access_mode::write>::type
    > pointer data() noexcept { return impl_.data(); }
    const_pointer data() const noexcept { return impl_.data(); }

    /**
     * Returns an iterator to the first requested byte, if a valid memory mapping
     * exists, otherwise this function call is equivalent to invoking `end`.
     */
    iterator begin() noexcept { return impl_.begin(); }
    const_iterator begin() const noexcept { return impl_.begin(); }
    const_iterator cbegin() const noexcept { return impl_.cbegin(); }

    /** Returns an iterator one past the last requested byte. */
    template<
        access_mode A = AccessMode,
        typename = typename std::enable_if<A == access_mode::write>::type
    > iterator end() noexcept { return impl_.end(); }
    const_iterator end() const noexcept { return impl_.end(); }
    const_iterator cend() const noexcept { return impl_.cend(); }

    template<
        access_mode A = AccessMode,
        typename = typename std::enable_if<A == access_mode::write>::type
    > reverse_iterator rbegin() noexcept { return impl_.rbegin(); }
    const_reverse_iterator rbegin() const noexcept { return impl_.rbegin(); }
    const_reverse_iterator crbegin() const noexcept { return impl_.crbegin(); }

    template<
        access_mode A = AccessMode,
        typename = typename std::enable_if<A == access_mode::write>::type
    > reverse_iterator rend() noexcept { return impl_.rend(); }
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
     * Establishes a memory mapping with AccessMode. If the mapping is unsuccesful, the
     * reason is reported via `error` and the object remains in a state as if this
     * function hadn't been called.
     *
     * `path`, which must be a path to an existing file, is used to retrieve a file
     * handle (which is closed when the object destructs or `unmap` is called), which is
     * then used to memory map the requested region. Upon failure, `error` is set to
     * indicate the reason and the object remains in an unmapped state.
     *
     * `offset` is the number of bytes, relative to the start of the file, where the
     * mapping should begin. When specifying it, there is no need to worry about
     * providing a value that is aligned with the operating system's page allocation
     * granularity. This is adjusted by the implementation such that the first requested
     * byte (as returned by `data` or `begin`), so long as `offset` is valid, will be at
     * `offset` from the start of the file.
     *
     * `length` is the number of bytes to map. It may be `map_entire_file`, in which
     * case a mapping of the entire file is created.
     */
    template<typename String>
    void map(const String& path, const size_type offset,
        const size_type length, std::error_code& error)
    {
        impl_.map(path, offset, length, AccessMode, error);
    }

    /**
     * Establishes a memory mapping with AccessMode. If the mapping is unsuccesful, the
     * reason is reported via `error` and the object remains in a state as if this
     * function hadn't been called.
     *
     * `handle`, which must be a valid file handle, which is used to memory map the
     * requested region. Upon failure, `error` is set to indicate the reason and the
     * object remains in an unmapped state.
     *
     * `offset` is the number of bytes, relative to the start of the file, where the
     * mapping should begin. When specifying it, there is no need to worry about
     * providing a value that is aligned with the operating system's page allocation
     * granularity. This is adjusted by the implementation such that the first requested
     * byte (as returned by `data` or `begin`), so long as `offset` is valid, will be at
     * `offset` from the start of the file.
     *
     * `length` is the number of bytes to map. It may be `map_entire_file`, in which
     * case a mapping of the entire file is created.
     */
    void map(const handle_type handle, const size_type offset,
        const size_type length, std::error_code& error)
    {
        impl_.map(handle, offset, length, AccessMode, error);
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

    void swap(basic_mmap& other) { impl_.swap(other.impl_); }

    /** Flushes the memory mapped page to disk. Errors are reported via `error`. */
    template<
        access_mode A = AccessMode,
        typename = typename std::enable_if<A == access_mode::write>::type
    > void sync(std::error_code& error) { impl_.sync(error); }

    /**
     * All operators compare the address of the first byte and size of the two mapped
     * regions.
     */

    friend bool operator==(const basic_mmap& a, const basic_mmap& b)
    {
        return a.impl_ == b.impl_;
    }

    friend bool operator!=(const basic_mmap& a, const basic_mmap& b)
    {
        return !(a == b);
    }

    friend bool operator<(const basic_mmap& a, const basic_mmap& b)
    {
        return a.impl_ < b.impl_;
    }

    friend bool operator<=(const basic_mmap& a, const basic_mmap& b)
    {
        return a.impl_ <= b.impl_;
    }

    friend bool operator>(const basic_mmap& a, const basic_mmap& b)
    {
        return a.impl_ > b.impl_;
    }

    friend bool operator>=(const basic_mmap& a, const basic_mmap& b)
    {
        return a.impl_ >= b.impl_;
    }
};

/**
 * This is the basis for all read-only mmap objects and should be preferred over
 * directly using basic_mmap.
 */
template<typename ByteT>
using basic_mmap_source = basic_mmap<access_mode::read, ByteT>;

/**
 * This is the basis for all read-write mmap objects and should be preferred over
 * directly using basic_mmap.
 */
template<typename ByteT>
using basic_mmap_sink = basic_mmap<access_mode::write, ByteT>;

/**
 * These aliases cover the most common use cases, both representing a raw byte stream
 * (either with a char or an unsigned char/uint8_t).
 */
using mmap_source = basic_mmap_source<char>;
using ummap_source = basic_mmap_source<unsigned char>;

using mmap_sink = basic_mmap_sink<char>;
using ummap_sink = basic_mmap_sink<unsigned char>;

/** Convenience factory method that constructs a mapping for any basic_mmap<> type. */
template<
    typename MMap,
    typename MappingToken
> MMap make_mmap(const MappingToken& token,
    int64_t offset, int64_t length, std::error_code& error)
{
    MMap mmap;
    mmap.map(token, offset, length, error);
    return mmap;
}

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
    return make_mmap<mmap_source>(token, offset, length, error);
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
    return make_mmap<mmap_sink>(token, offset, length, error);
}

} // namespace mio

#endif // MIO_MMAP_HEADER
