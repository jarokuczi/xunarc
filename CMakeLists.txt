cmake_minimum_required(VERSION 3.13)
project(xunarc C)

set(CMAKE_C_STANDARD 99)
add_executable(xunarc main.c xad.c gui.c common.c)
#target_link_options(xunarc PUBLIC -mcrt=nix20)
target_link_options(xunarc PUBLIC -mcrt=clib2)
target_include_directories(xunarc PUBLIC ${PROJECT_SOURCE_DIR}/deps/xadmaster)
install(PROGRAMS  ${PROJECT_BINARY_DIR}/xunarc DESTINATION ${TARGET_INSTALL_DIR})