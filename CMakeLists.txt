cmake_minimum_required(VERSION 3.10)
project(vlc-discordrpc-plugin C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

file(GLOB SOURCES "src/*.c")

add_compile_options(-Wall -Wextra -O2 -DNDEBUG)

if(APPLE)
    message(FATAL_ERROR "MacOS is not supported by this build configuration.")
elseif(WIN32)
    message(STATUS "Configuring for Windows...")

    find_package(PkgConfig REQUIRED)
    pkg_check_modules(VLC vlc-plugin IMPORTED_TARGET)

    if(NOT VLC_FOUND)
    message(FATAL_ERROR 
        " ERROR: 'vlc-plugin' module not found.\n"
        " Please install: pacman -S mingw-w64-x86_64-vlc\n"
    )
    endif()

    add_library(discordrpc_plugin SHARED ${SOURCES})

    target_compile_definitions(discordrpc_plugin PRIVATE 
        poll=WSAPoll 
        pollfd=WSAPOLLFD
        _WIN32_WINNT=0x0600
    )
    
    target_compile_options(discordrpc_plugin PRIVATE -include winsock2.h)

    set_target_properties(discordrpc_plugin PROPERTIES 
        PREFIX "lib"
        SUFFIX ".dll"
    )
    
    target_link_libraries(discordrpc_plugin PRIVATE PkgConfig::VLC)

    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        target_link_options(discordrpc_plugin PRIVATE -Wl,-s)
    endif()

    get_filename_component(VLC_INSTALL_DIR 
        "[HKEY_LOCAL_MACHINE\\SOFTWARE\\VideoLAN\\VLC;InstallDir]" 
        ABSOLUTE
    )

    file(TO_CMAKE_PATH "${VLC_INSTALL_DIR}/plugins/misc" VLC_PLUGIN_DIR)

    if(NOT VLC_INSTALL_DIR STREQUAL "" AND EXISTS "${VLC_INSTALL_DIR}")
        install(TARGETS discordrpc_plugin 
            RUNTIME DESTINATION "${VLC_PLUGIN_DIR}"
        )

        message(STATUS "Plugin installed to ${VLC_PLUGIN_DIR}/libdiscordrpc_plugin.dll")
    else()
        message(WARNING "VLC installation directory not found. Please install the plugin manually.")
    endif()

elseif(UNIX)
    message(STATUS "Configuring for Linux...")
    
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(VLC vlc-plugin IMPORTED_TARGET)

    if(NOT VLC_FOUND)
    message(FATAL_ERROR 
        " ERROR: 'vlc-plugin' module not found.\n"
        " For Ubuntu/Debian, please install: sudo apt install libvlccore-dev\n"
        " For Fedora, please install: sudo dnf install vlc-devel"
    )
    endif()

    add_library(discordrpc_plugin SHARED ${SOURCES})

    set_target_properties(discordrpc_plugin PROPERTIES 
        PREFIX "lib"
        SUFFIX ".so"
    )
    
    target_link_libraries(discordrpc_plugin PRIVATE PkgConfig::VLC)

    set_target_properties(discordrpc_plugin PROPERTIES POSITION_INDEPENDENT_CODE ON)

    pkg_get_variable(VLC_PLUGIN_DIR vlc-plugin pluginsdir)

    if(VLC_PLUGIN_DIR AND EXISTS "${VLC_PLUGIN_DIR}")
        install(TARGETS discordrpc_plugin 
            LIBRARY DESTINATION "${VLC_PLUGIN_DIR}/misc"
        )

        message(STATUS "Plugin installed to ${VLC_PLUGIN_DIR}/misc/libdiscordrpc_plugin.so")
    else()
        message(WARNING "VLC installation directory not found. Please install the plugin manually.")
    endif()
else()
    message(FATAL_ERROR "Unsupported platform.")
endif()
