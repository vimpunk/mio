#
# While not strictly necessary to specify header files as target sources,
# doing so populates these files in the source listing when CMake is used
# to generate XCode and Visual Studios projects
#
target_sources(mio-headers INTERFACE
  "${prefix}/mio/mmap.hpp"
  "${prefix}/mio/page.hpp"
  "${prefix}/mio/shared_mmap.hpp")

add_subdirectory(detail)
