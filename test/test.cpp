#include <mio/mmap.hpp>
#include <mio/shared_mmap.hpp>

#include <string>
#include <fstream>
#include <cstdlib>
#include <iostream>
#include <cassert>
#include <system_error>
#include <numeric>

int handle_error(const std::error_code& error)
{
    const auto& errmsg = error.message();
    std::printf("error mapping file: %s, exiting...\n", errmsg.c_str());
    return error.value();
}

// Just make sure this compiles.
#ifdef CXX17
# include <cstddef>
using mmap_source = mio::basic_mmap_source<std::byte>;
#endif

int main()
{
    const char _path[] = "test-file";
    // Make sure mio compiles with non-const char* strings too.
    const int path_len = sizeof(_path);
    char* path = new char[path_len];
    std::copy(_path, _path + path_len, path);
    std::error_code error;
    // Fill buffer, then write it to file.
    const int file_size = 0x4000 - 250;
    std::string buffer(file_size, 0);
    std::iota(buffer.begin(), buffer.end(), 1);
    std::ofstream file(path);
    file << buffer;
    file.close();

    const size_t offset = 300;
    assert(offset < buffer.size());

    {
        // Map the region of the file to which buffer was written.
        mio::mmap_source file_view = mio::make_mmap_source(
                path, offset, mio::map_entire_file, error);
        if(error) { return handle_error(error); }

        const size_t mapped_size = buffer.size() - offset;
        assert(file_view.is_open());
        assert(file_view.size() == mapped_size);

        // Then verify that mmap's bytes correspond to that of buffer.
        for(size_t buf_idx = offset, view_idx = 0;
                buf_idx < buffer.size() && view_idx < mapped_size;
                ++buf_idx, ++view_idx) {
            if(file_view[view_idx] != buffer[buf_idx]) {
                std::printf("%luth byte mismatch: expected(%d) <> actual(%d)",
                        buf_idx, buffer[buf_idx], file_view[view_idx]);
                std::cout << std::flush;
                assert(0);
            }
        }

        // Turn file_view into a shared mmap.
        mio::shared_mmap_source shared_file_view(std::move(file_view));

        assert(!file_view.is_open());
        assert(shared_file_view.is_open());
        assert(shared_file_view.size() == mapped_size);

        // Then verify that shared_mmap's bytes correspond to that of buffer.
        for(size_t buf_idx = offset, view_idx = 0;
                buf_idx < buffer.size() && view_idx < mapped_size;
                ++buf_idx, ++view_idx) {
            if(shared_file_view[view_idx] != buffer[buf_idx]) {
                std::printf("%luth byte mismatch: expected(%d) <> actual(%d)",
                        buf_idx, buffer[buf_idx], shared_file_view[view_idx]);
                std::cout << std::flush;
                assert(0);
            }
        }
    }

    {
#define CHECK_INVALID_MMAP(m) do { \
        assert(error); \
        assert(m.empty()); \
        assert(!m.is_open()); \
        error.clear(); } while(0)

        mio::mmap_source m;

        // See if mapping an invalid file results in an error.
        m = mio::make_mmap_source("garbage-that-hopefully-doesnt-exist", 0, 0, error);
        CHECK_INVALID_MMAP(m);

        // Empty path?
        m = mio::make_mmap_source(static_cast<const char*>(0), 0, 0, error);
        CHECK_INVALID_MMAP(m);
        m = mio::make_mmap_source(std::string(), 0, 0, error);
        CHECK_INVALID_MMAP(m);

        // Invalid handle?
        m = mio::make_mmap_source(mio::invalid_handle, 0, 0, error);
        CHECK_INVALID_MMAP(m);

        // Invalid offset?
        m = mio::make_mmap_source(path, 100 * buffer.size(), buffer.size(), error);
        CHECK_INVALID_MMAP(m);
    }

    {
        // Make sure these compile.
        mio::ummap_source _1;
        mio::shared_ummap_source _2;
        // Make sure shared_mmap mapping compiles as all testing was done on
        // normal mmaps.
        mio::shared_mmap_source _3(path, 0, mio::map_entire_file);
        auto _4 = mio::make_mmap_source(path, error);
        auto _5 = mio::make_mmap<mio::shared_mmap_source>(path, 0, mio::map_entire_file, error);
#ifdef _WIN32
        const wchar_t* wpath1 = L"dasfsf";
        auto _6 = mio::make_mmap_source(wpath1, error);
        mio::mmap_source _7;
        _7.map(wpath1, error);
        const std::wstring wpath2 = wpath1;
        auto _8 = mio::make_mmap_source(wpath2, error);
        mio::mmap_source _9;
        _9.map(wpath1, error);
#endif
    }

    std::printf("all tests passed!\n");
}
