cmake_minimum_required(VERSION 3.16.0)
project(scte35-insert)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_FLAGS_DEBUG "-ggdb -O0 -DDEBUG -Wall -Wuninitialized -Wsign-compare -Wno-missing-braces")
set(CMAKE_CXX_FLAGS_RELEASE "-ggdb -O3 -DNDEBUG -Wall -Wuninitialized -Wsign-compare -Wno-missing-braces")

find_package(PkgConfig)
pkg_search_module(GLIB REQUIRED glib-2.0)
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0)
pkg_check_modules(GSTREAMER_MPEGTS REQUIRED gstreamer-mpegts-1.0)

set(FILES
        main.cpp
        utils/ScopedGLibObject.h
        utils/ScopedGstObject.h
        Pipeline.cpp
        Pipeline.h
        Logger.h
        Logger.cpp)

add_executable(${PROJECT_NAME} ${FILES})

target_include_directories(${PROJECT_NAME} PRIVATE
        ${PROJECT_SOURCE_DIR}
        ${GLIB_INCLUDE_DIRS}
        ${GSTREAMER_INCLUDE_DIRS}
        ${GSTREAMER_MPEGTS_INCLUDE_DIRS})

target_link_libraries(${PROJECT_NAME}
        ${GLIB_LIBRARIES}
        ${GSTREAMER_LDFLAGS}
        ${GSTREAMER_MPEGTS_LDFLAGS})
