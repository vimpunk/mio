add_executable(mio.test test.cpp)
target_link_libraries(mio.test PRIVATE mio::mio)
add_test(NAME mio.test COMMAND mio.test)

add_executable(mio.example example.cpp)
target_link_libraries(mio.example PRIVATE mio::mio)
add_test(NAME mio.example COMMAND mio.example)
