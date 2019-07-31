cmake_minimum_required(VERSION 3.14)
project(winsvc_continuum C)

set(CMAKE_C_STANDARD 11)

add_executable(winsvc_continuum src/main.c res/version.rc src/version.h)

target_compile_options(winsvc_continuum PRIVATE /Os)
target_link_options(winsvc_continuum PRIVATE /VERSION:1.00)
target_link_options(winsvc_continuum PRIVATE "/MANIFESTUAC:level='requireAdministrator' uiAccess='false'")
target_link_options(winsvc_continuum PRIVATE /NXCOMPAT)
target_link_options(winsvc_continuum PRIVATE /DYNAMICBASE)
target_link_options(winsvc_continuum PRIVATE /RELEASE)
target_link_options(winsvc_continuum PRIVATE /LARGEADDRESSAWARE)
target_link_options(winsvc_continuum PRIVATE /OPT:REF)
target_link_options(winsvc_continuum PRIVATE /OPT:ICF)
