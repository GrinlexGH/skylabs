# Searches for https://github.com/julianxhokaxhiu/SteamworksSDKCI/releases
include(FindPackageHandleStandardArgs)

if(NOT SteamworksSDK_FOUND)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(_arch_suffix "64")
        set(_steam_api_lib_names steam_api64 steam_api)
        set(_steam_api_dll_names steam_api64.dll)
        set(_appticket_lib_names sdkencryptedappticket64 sdkencryptedappticket)
        set(_appticket_dll_names sdkencryptedappticket64.dll)
    elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
        set(_arch_suffix "32")
        set(_steam_api_lib_names steam_api)
        set(_steam_api_dll_names steam_api.dll)
        set(_appticket_lib_names sdkencryptedappticket)
        set(_appticket_dll_names sdkencryptedappticket.dll)
    endif()

    find_library(
        SteamworksSDK_LIBRARY
        NAMES
        ${_steam_api_lib_names}
        PATH_SUFFIXES
        SteamworksSDK/lib/
        SteamworksSDK/lib/win64/
        SteamworksSDK/lib/linux${_arch_suffix}/
        SteamworksSDK/lib/osx/
    )

    find_file(
        SteamworksSDK_DLL
        ${_steam_api_dll_names}
        PATH_SUFFIXES
        SteamworksSDK/bin/
        SteamworksSDK/bin/win64/
    )

    find_path(
        SteamworksSDK_INCLUDE_DIR
        steam/steam_api.h
        PATH_SUFFIXES
        SteamworksSDK/include/
    )

    add_library(SteamworksSDK::SteamworksSDK SHARED IMPORTED)

    if(WIN32)
        set_target_properties(SteamworksSDK::SteamworksSDK PROPERTIES
            IMPORTED_IMPLIB "${SteamworksSDK_LIBRARY}"
            IMPORTED_LOCATION "${SteamworksSDK_DLL}"
            INTERFACE_INCLUDE_DIRECTORIES "${SteamworksSDK_INCLUDE_DIR}"
        )
    else()
        set_target_properties(SteamworksSDK::SteamworksSDK PROPERTIES
            IMPORTED_LOCATION "${SteamworksSDK_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${SteamworksSDK_INCLUDE_DIR}"
        )
    endif()

    #-----------------------------------------

    find_library(
        SteamworksSDK_AppTicket_LIBRARY
        ${_appticket_lib_names}
        PATH_SUFFIXES
        SteamworksSDK/lib/win${_arch_suffix}/
        SteamworksSDK/lib/linux${_arch_suffix}/
        SteamworksSDK/lib/osx/
    )

    find_file(
        SteamworksSDK_AppTicket_DLL
        ${_appticket_dll_names}
        PATH_SUFFIXES
        SteamworksSDK/bin/win${_arch_suffix}/
    )

    add_library(SteamworksSDK::AppTicket SHARED IMPORTED)

    if(WIN32)
        set_target_properties(SteamworksSDK::AppTicket PROPERTIES
            IMPORTED_IMPLIB "${SteamworksSDK_AppTicket_LIBRARY}"
            IMPORTED_LOCATION "${SteamworksSDK_AppTicket_DLL}"
            INTERFACE_INCLUDE_DIRECTORIES "${SteamworksSDK_INCLUDE_DIR}"
        )
    else()
        set_target_properties(SteamworksSDK::AppTicket PROPERTIES
            IMPORTED_LOCATION "${SteamworksSDK_AppTicket_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${SteamworksSDK_INCLUDE_DIR}"
            IMPORTED_NO_SONAME 1
        )
    endif()

    #-----------------------------------------

    find_package_handle_standard_args(SteamworksSDK DEFAULT_MSG
        SteamworksSDK_LIBRARY
        SteamworksSDK_AppTicket_LIBRARY
        SteamworksSDK_INCLUDE_DIR
    )
endif()
