# Searches for https://github.com/julianxhokaxhiu/SteamworksSDKCI/releases
include(FindPackageHandleStandardArgs)

if(NOT SteamworksSDK_FOUND)
    set(_steam_api_lib_names steam_api64 steam_api)
    set(_appticket_lib_names sdkencryptedappticket64 sdkencryptedappticket)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(_arch_suffix "x64")
        set(_steam_api_dll_names steam_api64.dll)
        set(_appticket_dll_names sdkencryptedappticket64.dll)
    elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
        set(_arch_suffix "x86")
        set(_steam_api_dll_names steam_api.dll)
        set(_appticket_dll_names sdkencryptedappticket.dll)
    else()
        message(FATAL_ERROR "Unknown pointer size")
    endif()

    find_library(
        SteamworksSDK_LIBRARY
        ${_steam_api_lib_names}
        PATH_SUFFIXES
        steamworks_sdk/lib/steam
        steamworks_sdk/bin/steam
        steamworks_sdk_${_arch_suffix}/lib/steam
        steamworks_sdk_${_arch_suffix}/bin/steam
    )

    find_file(
        SteamworksSDK_DLL
        ${_steam_api_dll_names}
        PATH_SUFFIXES
        steamworks_sdk/bin/steam
        steamworks_sdk_${_arch_suffix}/bin/steam
    )

    find_path(
        SteamworksSDK_INCLUDE_DIR
        steam/steam_api.h
        PATH_SUFFIXES
        steamworks_sdk/include
        steamworks_sdk_${_arch_suffix}/include
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
        steamworks_sdk/bin/steam
        steamworks_sdk/lib/steam
        steamworks_sdk_${_arch_suffix}/bin/steam
        steamworks_sdk_${_arch_suffix}/lib/steam
    )

    find_file(
        SteamworksSDK_AppTicket_DLL
        ${_appticket_dll_names}
        PATH_SUFFIXES
        steamworks_sdk/bin/steam
        steamworks_sdk_${_arch_suffix}/bin/steam
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
            IMPORTED_NO_SONAME TRUE
        )
    endif()

    #-----------------------------------------

    find_package_handle_standard_args(SteamworksSDK DEFAULT_MSG
        SteamworksSDK_LIBRARY
        SteamworksSDK_AppTicket_LIBRARY
        SteamworksSDK_INCLUDE_DIR
    )
endif()