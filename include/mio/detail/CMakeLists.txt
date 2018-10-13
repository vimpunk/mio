if(NOT subproject)
  target_sources(mio INTERFACE
    $<BUILD_INTERFACE:
      ${CMAKE_CURRENT_LIST_DIR}/basic_mmap.hpp
      ${CMAKE_CURRENT_LIST_DIR}/basic_mmap.ipp
      ${CMAKE_CURRENT_LIST_DIR}/string_util.hpp>)
endif()
