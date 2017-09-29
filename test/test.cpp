#include "../include/mio/mmap.hpp"

#include <string>
#include <fstream>
#include <cstdlib>
#include <cassert>
#include <system_error>

int main()
{
    // Both are 40 bytes on UNIX, 48 on Windows (TODO verify).
    //std::printf("sizeof(mio::mmap_source) = %i bytes\n", sizeof(mio::mmap_source));
    //std::printf("sizeof(mio::mmap_sink) = %i bytes\n", sizeof(mio::mmap_source));

    const char* file_path = "test-file";

    // Fill buffer, then write it to file.
    std::string buffer(0x4000 - 250, 'M');
    std::ofstream file(file_path);
    file << buffer;
    file.close();

    // Map the region of the file to which buffer was written.
    std::error_code error;
    mio::mmap_source file_view = mio::make_mmap_source(file_path,
        0, mio::mmap_source::use_full_file_size, error);
    if(error)
    {
        const auto& errmsg = error.message();
        std::printf("error mapping file: %s, exiting...\n", errmsg.c_str());
        return error.value();
    }

    assert(file_view.is_open());
    assert(file_view.size() == buffer.size());

    // Then verify that mmap's bytes correspond to that of buffer.
    for(auto i = 0; i < buffer.size(); ++i)
    {
        if(file_view[i] != buffer[i])
        {
            std::printf("%ith byte mismatch: expected(%i) <> actual(%i)",
                i, buffer[i], file_view[i]);
            assert(0);
        }
    }

#define CHECK_INVALID_MMAP(m) do { \
    assert(error); \
    assert(m.empty()); \
    assert(!m.is_open()); \
    error.clear(); } while(0)

    mio::mmap_source m;

    // FIXME move assignment DOES NOT WORK!
    // See if mapping an invalid file results in an error.
    m = mio::make_mmap_source("garbage-that-hopefully-doesnt-exist", 0, 0, error);
    CHECK_INVALID_MMAP(m);

    // Empty path?
    m = mio::make_mmap_source(static_cast<const char*>(0), 0, 0, error);
    CHECK_INVALID_MMAP(m);
    m = mio::make_mmap_source(std::string(), 0, 0, error);
    CHECK_INVALID_MMAP(m);

    // Invalid handle?
    m = mio::make_mmap_source(
        INVALID_HANDLE_VALUE/*Psst... This is an implementation detail!*/, 0, 0, error);
    CHECK_INVALID_MMAP(m);

    // Invalid offset?
    m = mio::make_mmap_source(file_path, 100 * buffer.size(), buffer.size(), error);
    CHECK_INVALID_MMAP(m);

    std::printf("all tests passed!\n");
}

// TODO consider the following API: type safe length and offset parameters, as these
// two are easily interchanged due to the same type, with the optional feature of
// arbitrary ordering.
//mio::mmap_source file_view = mio::make_mmap_source(file_path,
//    mio::offset(0), mio::length(buffer.size()), error);
