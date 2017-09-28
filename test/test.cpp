#include "../include/mio/mmap.hpp"

#include <string>
#include <fstream>
#include <cstdlib>
#include <cassert>
#include <system_error>

int main()
{
    const char* test_file_name = "test-file";

    std::string buffer(0x4000 - 250, 'M');

    std::ofstream file(test_file_name);
    file << buffer;
    file.close();

    std::error_code error;
    mio::mmap_source file_view = mio::make_mmap_source(test_file_name, 0, buffer.size(), error);
    if(error)
    {
        const auto& errmsg = error.message();
        std::printf("error: %s, exiting...\n", errmsg.c_str());
        return error.value();
    }

    assert(file_view.is_open());
    assert(file_view.is_mapped());
    assert(file_view.size() == buffer.size());

    for(auto i = 0; i < buffer.size(); ++i)
    {
        if(file_view[i] != buffer[i])
        {
            std::printf("%ith byte mismatch: expected(%i) <> actual(%i)",
                i, buffer[i], file_view[i]);
            assert(0);
        }
    }

    // see if mapping an invalid file results in an error
    mio::make_mmap_source("garbage-that-hopefully-doesn't exist", 0, 0, error);
    assert(error);

    std::printf("all tests passed!\n");
}
