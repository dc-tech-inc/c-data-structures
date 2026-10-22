cmake_minimum_required(VERSION 3.16)
project(dcds VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(DCDS_SANITIZE "Build with AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
option(DCDS_COVERAGE "Build with gcov instrumentation (gcc/clang)" OFF)
option(DCDS_WERROR "Treat warnings as errors" ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release)
endif()

add_library(dcds
  src/common.c src/list.c src/vec.c src/hashmap.c src/heap.c src/avl.c)
target_include_directories(dcds PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)

set(dcds_warnings -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wstrict-prototypes -Wmissing-prototypes)
if(DCDS_WERROR)
  list(APPEND dcds_warnings -Werror)
endif()

function(dcds_configure target)
  target_compile_options(${target} PRIVATE ${dcds_warnings})
  if(DCDS_SANITIZE)
    target_compile_options(${target} PRIVATE -fsanitize=address,undefined -fno-sanitize-recover=undefined -fno-omit-frame-pointer)
    target_link_options(${target} PRIVATE -fsanitize=address,undefined)
  endif()
  if(DCDS_COVERAGE)
    target_compile_options(${target} PRIVATE --coverage -O0)
    target_link_options(${target} PRIVATE --coverage)
  endif()
endfunction()
dcds_configure(dcds)

enable_testing()
foreach(name list vec hashmap heap avl)
  add_executable(test_${name} tests/test_${name}.c)
  target_link_libraries(test_${name} PRIVATE dcds)
  dcds_configure(test_${name})
  add_test(NAME ${name} COMMAND test_${name})
endforeach()

add_executable(bench bench/bench.c)
target_link_libraries(bench PRIVATE dcds)
dcds_configure(bench)

install(TARGETS dcds ARCHIVE DESTINATION lib)
install(DIRECTORY include/ DESTINATION include)
