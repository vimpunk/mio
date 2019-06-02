#
# iff mio is the highest level project, include the implementation
# detail files in the source listing for CMake-generated IDE projects
#
target_sources(mio-headers INTERFACE
  "${prefix}/mio/detail/mmap.ipp"
  "${prefix}/mio/detail/string_util.hpp")
