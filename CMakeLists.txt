cmake_minimum_required( VERSION 3.2.2 )
project( mio )

### Standard
set( CMAKE_CXX_STANDARD 11 )

# Generate 'compile_commands.json' for clang_complete
set( CMAKE_EXPORT_COMPILE_COMMANDS ON )

### Flags/Options
option( BUILD_TESTS "Enable the building of mio unit tests" OFF )

### Library targets
add_library( mio INTERFACE)
target_include_directories( mio INTERFACE include )
install(
	DIRECTORY include/
	DESTINATION include
)

### Test targets
if( BUILD_TESTS )
	## test
	add_executable(
		test
		test/test.cpp
	)
	target_link_libraries( test PRIVATE mio )
	
	## example
	add_executable(
		example
		test/example.cpp
	)
	target_link_libraries( example PRIVATE mio )
endif()
