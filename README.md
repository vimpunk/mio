# mio
A simple header-only cross-platform C++14 memory mapping library.

## Example
```c++
#include <mio/mmap.hpp>
#include <mio/page.hpp> // for mio::page_size
#include <system_error> // for std::error_code
#include <cstdio> // for std::printf

// This is just to clarify which is which when using the various mapping methods.
using offset_type = mio::mmap_source::size_type;
using length_type = mio::mmap_source::size_type;

int handle_error(const std::error_code& error)
{
    const auto& errmsg = error.message();
    std::printf("error mapping file: %s, exiting...\n", errmsg.c_str());
    return error.value();
}

int main()
{
    // Read-only memory map the whole file by using `use_full_file_size` where the
    // length of the mapping would otherwise be expected, with the factory method.
    std::error_code error;
    mio::mmap_source mmap1 = mio::make_mmap_source("log.txt",
        offset_type(0), mio::use_full_file_size, error);
    if(error) { return handle_error(error); }

    // Read-write memory map the beginning of some file using the `map` member function.
    mio::mmap_sink mmap2;
    mmap2.map("some/path/to/file", offset_type(0), length_type(2342), error);
    if(error) { return handle_error(error); }

    // Iterate through the memory mapping just as if it were any other container.
    for(auto& c : mmap2) {
        // Since this is a read-write mapping, we can change c's value.
        c += 10;
    }

    // Or just change one value with the subscript operator.
    mmap2[mmap2.size() / 2] = 42;

    // You can also create a mapping using the constructor, which throws a
    // std::error_code upon failure.
    try {
        const auto page_size = mio::page_size();
        mio::mmap_sink mmap3("another/path/to/file",
            offset_type(page_size), length_type(page_size));
        // ...
    } catch(const std::error_code& error) {
        return handle_error(error);
    }

    // mio exposes an interface that abstracts away memory as a string, so it is
    // possible to create mmap objects that have custom underlying character types:
    using wmmap_source = mio::basic_mmap_source<wchar_t>;
}
```

# Installation
mio is a header-only library, so just copy the contents in `mio/include` into your system wide include path, such as `/usr/include`, or into your project's lib folder.

