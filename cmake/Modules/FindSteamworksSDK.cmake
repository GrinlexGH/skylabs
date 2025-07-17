# Author: Grinlex
#
# Components:
# ``SteamAPI``
# ``AppTicket``
#
# Imported targets:
# ``SteamworksSDK::SteamAPI``
# ``SteamworksSDK::AppTicket``
#
# Result variables:
# ``SteamworksSDK_FOUND``
# ``SteamworksSDK_INCLUDE_DIR``
# ``SteamworksSDK_SteamAPI_LIBRARY``
# ``SteamworksSDK_SteamAPI_DLL``
# ``SteamworksSDK_AppTicket_LIBRARY``
# ``SteamworksSDK_AppTicket_DLL``
#
# Searches for such folder structure:
# SteamworksSDK/
# ├── bin
# │   ├── steam_api.dll
# │   ├── win32
# │   │   └── sdkencryptedappticket.dll
# │   └── win64
# │       ├── sdkencryptedappticket64.dll
# │       └── steam_api64.dll
# ├── include
# │   └── steam
# │       ├── ...
# │       ├── steam_api.h
# │       ├── ...
# │       ├── steamencryptedappticket.h
# │       └── ...
# └── lib
#     ├── linux32
#     │   ├── libsdkencryptedappticket.so
#     │   └── libsteam_api.so
#     ├── linux64
#     │   ├── libsdkencryptedappticket.so
#     │   └── libsteam_api.so
#     ├── osx
#     │   ├── libsdkencryptedappticket.dylib
#     │   └── libsteam_api.dylib
#     ├── steam_api.lib
#     ├── win32
#     │   └── sdkencryptedappticket.lib
#     └── win64
#         ├── sdkencryptedappticket64.lib
#         └── steam_api64.lib

include(FindPackageHandleStandardArgs)

set(_SteamworksSDK_known_components SteamAPI AppTicket)

foreach(_comp IN LISTS SteamworksSDK_FIND_COMPONENTS)
    list(FIND _SteamworksSDK_known_components "${_comp}" _index)
    if(_index EQUAL -1)
        set(SteamworksSDK_FOUND FALSE)
        set(SteamworksSDK_NOT_FOUND_MESSAGE "Unsupported component: ${_comp}")
        return()
    endif()
endforeach()

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(_arch_suffix "64")
    set(_steam_api_lib_names steam_api64)
    set(_steam_api_dll_names steam_api64.dll)
    set(_appticket_lib_names sdkencryptedappticket64)
    set(_appticket_dll_names sdkencryptedappticket64.dll)
    if(NOT WIN32)
        list(APPEND _steam_api_lib_names steam_api)
        list(APPEND _appticket_lib_names sdkencryptedappticket)
    endif()
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
    set(_arch_suffix "32")
    set(_steam_api_lib_names steam_api)
    set(_steam_api_dll_names steam_api.dll)
    set(_appticket_lib_names sdkencryptedappticket)
    set(_appticket_dll_names sdkencryptedappticket.dll)
else()
    set(SteamworksSDK_FOUND FALSE)
    set(SteamworksSDK_NOT_FOUND_MESSAGE "Unknown architecture size")
    return()
endif()

set(_SteamworksSDK_required_vars SteamworksSDK_INCLUDE_DIR)

# --- Include directory ---
find_path(SteamworksSDK_INCLUDE_DIR
    NAMES
        steam/steam_api.h
        steam/steamencryptedappticket.h
    PATH_SUFFIXES
        include
        SteamworksSDK/include
)

# --- SteamAPI ---
if("SteamAPI" IN_LIST SteamworksSDK_FIND_COMPONENTS OR NOT SteamworksSDK_FIND_COMPONENTS)
    find_library(SteamworksSDK_SteamAPI_LIBRARY
        NAMES
            ${_steam_api_lib_names}
        PATH_SUFFIXES
            lib/
            lib/win64/
            lib/linux${_arch_suffix}/
            lib/osx/
            SteamworksSDK/lib/
            SteamworksSDK/lib/win${_arch_suffix}/
            SteamworksSDK/lib/linux${_arch_suffix}/
            SteamworksSDK/lib/osx/
    )
    list(APPEND _SteamworksSDK_required_vars SteamworksSDK_SteamAPI_LIBRARY)

    if(WIN32)
        find_file(SteamworksSDK_SteamAPI_DLL
            NAMES
                ${_steam_api_dll_names}
            PATH_SUFFIXES
                bin/
                bin/win64/
                SteamworksSDK/bin/
                SteamworksSDK/bin/win${_arch_suffix}/
        )
        list(APPEND _SteamworksSDK_required_vars SteamworksSDK_SteamAPI_DLL)
    endif()

    if(SteamworksSDK_SteamAPI_LIBRARY AND SteamworksSDK_INCLUDE_DIR AND NOT TARGET SteamworksSDK::SteamAPI)
        if(WIN32)
            if(SteamworksSDK_SteamAPI_DLL)
                add_library(SteamworksSDK::SteamAPI SHARED IMPORTED)
                set_target_properties(SteamworksSDK::SteamAPI PROPERTIES
                    IMPORTED_IMPLIB "${SteamworksSDK_SteamAPI_LIBRARY}"
                    IMPORTED_LOCATION "${SteamworksSDK_SteamAPI_DLL}"
                    INTERFACE_INCLUDE_DIRECTORIES "${SteamworksSDK_INCLUDE_DIR}"
                )
            endif()
        else()
            add_library(SteamworksSDK::SteamAPI SHARED IMPORTED)
            set_target_properties(SteamworksSDK::SteamAPI PROPERTIES
                IMPORTED_LOCATION "${SteamworksSDK_SteamAPI_LIBRARY}"
                INTERFACE_INCLUDE_DIRECTORIES "${SteamworksSDK_INCLUDE_DIR}"
            )
        endif()
    endif()
endif()

# --- AppTicket ---
if("AppTicket" IN_LIST SteamworksSDK_FIND_COMPONENTS OR NOT SteamworksSDK_FIND_COMPONENTS)
    find_library(SteamworksSDK_AppTicket_LIBRARY
        NAMES
            ${_appticket_lib_names}
        PATH_SUFFIXES
            lib/win${_arch_suffix}/
            lib/linux${_arch_suffix}/
            lib/osx/
            SteamworksSDK/lib/win${_arch_suffix}/
            SteamworksSDK/lib/linux${_arch_suffix}/
            SteamworksSDK/lib/osx/
    )
    list(APPEND _SteamworksSDK_required_vars SteamworksSDK_AppTicket_LIBRARY)

    if(WIN32)
        find_file(SteamworksSDK_AppTicket_DLL
            NAMES
                ${_appticket_dll_names}
            PATH_SUFFIXES
                bin/win${_arch_suffix}
                SteamworksSDK/bin/win${_arch_suffix}
        )
        list(APPEND _SteamworksSDK_required_vars SteamworksSDK_AppTicket_DLL)
    endif()

    if(SteamworksSDK_AppTicket_LIBRARY AND SteamworksSDK_INCLUDE_DIR AND NOT TARGET SteamworksSDK::AppTicket)
        if(WIN32)
            if(SteamworksSDK_AppTicket_DLL)
                add_library(SteamworksSDK::AppTicket SHARED IMPORTED)
                set_target_properties(SteamworksSDK::AppTicket PROPERTIES
                    IMPORTED_IMPLIB "${SteamworksSDK_AppTicket_LIBRARY}"
                    IMPORTED_LOCATION "${SteamworksSDK_AppTicket_DLL}"
                    INTERFACE_INCLUDE_DIRECTORIES "${SteamworksSDK_INCLUDE_DIR}"
                )
            endif()
        else()
            add_library(SteamworksSDK::AppTicket SHARED IMPORTED)
            set_target_properties(SteamworksSDK::AppTicket PROPERTIES
                IMPORTED_LOCATION "${SteamworksSDK_AppTicket_LIBRARY}"
                INTERFACE_INCLUDE_DIRECTORIES "${SteamworksSDK_INCLUDE_DIR}"
                IMPORTED_NO_SONAME 1
            )
        endif()
    endif()
endif()

#-----------------------------------------

find_package_handle_standard_args(SteamworksSDK
    REQUIRED_VARS
        ${_SteamworksSDK_required_vars}
    HANDLE_COMPONENTS
)

if(SteamworksSDK_FOUND)
    mark_as_advanced(SteamworksSDK_INCLUDE_DIR)
    mark_as_advanced(SteamworksSDK_SteamAPI_LIBRARY)
    mark_as_advanced(SteamworksSDK_SteamAPI_DLL)
    mark_as_advanced(SteamworksSDK_AppTicket_LIBRARY)
    mark_as_advanced(SteamworksSDK_AppTicket_DLL)
endif()
