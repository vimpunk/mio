cmake_minimum_required( VERSION 3.2.2 )
project( mio )

### Standard
set( CMAKE_CXX_STANDARD 11 )
set( CMAKE_CXX_STANDARD_REQUIRED ON )
set( CMAKE_CXX_EXTENSIONS ON )

### Verbosity
set( CMAKE_COLOR_MAKEFILE ON )
set( CMAKE_VERBOSE_MAKEFILE ON )

# Generate 'compile_commands.json' for clang_complete
set( CMAKE_EXPORT_COMPILE_COMMANDS ON )

### Flags
if( MSVC )
	add_compile_options( /W3 )
elseif( CMAKE_COMPILER_IS_GNUCXX )
	add_compile_options( -Wall )
	add_compile_options( -Wextra )
endif()

### Library targets
add_library( mio INTERFACE)
target_include_directories( mio INTERFACE include )

### Test targets

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
