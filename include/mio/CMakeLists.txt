target_sources(mio INTERFACE
  $<BUILD_INTERFACE:
    ${CMAKE_CURRENT_LIST_DIR}/mmap.hpp
    ${CMAKE_CURRENT_LIST_DIR}/page.hpp
    ${CMAKE_CURRENT_LIST_DIR}/shared_mmap.hpp>
  $<INSTALL_INTERFACE:
    include/mio/mmap.hpp
    include/mio/page.hpp
    include/mio/shared_mmap.hpp>)

add_subdirectory(detail)
