function(skylabs_configure_target target_name)
    if(NOT TARGET ${target_name})
        return()
    endif()

    # Source groups
    get_target_property(sources ${target_name} SOURCES)

    foreach(source IN LISTS sources)
        cmake_path(GET source PARENT_PATH source_directory)
        source_group("Source Files/${source_directory}" FILES "${source}")
    endforeach()

    # DLL copying
    get_target_property(target_type ${target_name} TYPE)
    set(allowed_types "EXECUTABLE" "SHARED_LIBRARY" "MODULE_LIBRARY")

    if(target_type IN_LIST allowed_types)
        if(WIN32)
            add_custom_command(TARGET ${target_name} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy -t $<TARGET_FILE_DIR:${target_name}> $<TARGET_RUNTIME_DLLS:${target_name}>
                COMMAND_EXPAND_LISTS
                COMMENT "Copying runtime DLLs to ${target_name} output directory"
            )
        else()
            set(cmd "-DOUTPUT_DIR=$<TARGET_FILE_DIR:${target_name}>")
            if(target_type STREQUAL "EXECUTABLE")
                list(APPEND cmd "-DAPPS=$<TARGET_FILE:${target_name}>")
            elseif(target_type STREQUAL "SHARED_LIBRARY")
                list(APPEND cmd "-DLIBS=$<TARGET_FILE:${target_name}>")
            elseif(target_type STREQUAL "MODULE_LIBRARY")
                list(APPEND cmd "-DMODS=$<TARGET_FILE:${target_name}>")
            endif()
            list(APPEND cmd "-P" "${CMAKE_SOURCE_DIR}/cmake/CopyDeps.cmake")
            add_custom_command(TARGET ${target_name} POST_BUILD
                COMMAND ${CMAKE_COMMAND}
                ${cmd}
                COMMENT "Resolving and copying symlinked dependencies..."
            )
        endif()
    endif()
endfunction()
