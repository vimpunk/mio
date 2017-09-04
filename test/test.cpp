#include "../include/mio/mmap.hpp"

#include <string>
#include <fstream>
#include <cstdlib>
#include <cassert>
#include <system_error>

int main(int argc, char** argv)
{
    const char* path = argc >= 2 ? argv[1] : ".";

    std::error_code error;
    std::string buffer(0x4000 - 250, 'M');

    std::ofstream file(path);
    file << buffer;
    file.close();

    mio::mmap_source file_view;
    file_view.map(path, 0, buffer.size(), error);
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
}
