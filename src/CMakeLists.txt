cmake_minimum_required(VERSION 2.8)

project(MyBot)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

IF (WIN32)
  # set stuff for windows
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O2 -Wall -pedantic")
ELSE()
  # set stuff for other systems
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O2 -Wall -Wno-unused-function -pedantic")
ENDIF()


include_directories(${CMAKE_SOURCE_DIR}/hlt)
include_directories(${CMAKE_SOURCE_DIR}/bot)

get_property(dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)

foreach(dir ${dirs})
    file(GLOB_RECURSE SOURCE ${dir}/*.[ch]*)
    set(SOURCE_FILES "${SOURCE_FILES}" ${SOURCE})
endforeach()

include_directories(${CMAKE_SOURCE_DIR})
set(SOURCE_FILES "${SOURCE_FILES}" MyBot.cpp)

add_executable(MyBot ${SOURCE_FILES})

if(MINGW)
    target_link_libraries(MyBot -static)
endif()
