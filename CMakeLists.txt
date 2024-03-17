cmake_minimum_required(VERSION 3.5)

project(dump_hbcifs LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(dump_hbcifs main.cpp
    dump_hbcifs.h dump_hbcifs.cpp
    elf.h
    elftypes.h
    image.h
    startup.h
    compat.h
    hdr.h
    hbcifs.h
)

target_link_libraries(dump_hbcifs -llzo2 -lucl -lz)

include(GNUInstallDirs)
install(TARGETS dump_hbcifs
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
