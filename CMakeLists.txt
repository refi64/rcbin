cmake_minimum_required(VERSION 2.6)
project(rcbin)

if ("${CMAKE_SYSTEM}" MATCHES "Linux")
    find_package(PkgConfig)
    pkg_search_module(LIBELF REQUIRED libelf)

    link_libraries(m)
endif ()

include_directories(include)

add_library(librcbin STATIC src/librcbin.c)
set_target_properties(librcbin PROPERTIES OUTPUT_NAME rcbin)
target_include_directories(librcbin PUBLIC include)

add_executable(rcbin src/rcbin.c)
target_include_directories(rcbin PUBLIC ${LIBELF_INCLUDE_DIRS})
target_link_libraries(rcbin PUBLIC ${LIBELF_LIBRARIES})

add_executable(tst tst.c)
target_link_libraries(tst librcbin)

install(TARGETS librcbin rcbin
        RUNTIME DESTINATION bin
        ARCHIVE DESTINATION lib)
install(FILES include/rcbin.h DESTINATION include)
