# dlls copying
function(skylabs_dll_copy target_name)
    if(NOT TARGET ${target_name})
        return()
    endif()

    get_target_property(target_type ${target_name} TYPE)
    set(types_requiring_dlls "EXECUTABLE" "SHARED_LIBRARY" "MODULE_LIBRARY")

    if(target_type IN_LIST types_requiring_dlls)
        if(NOT ANDROID)
            add_custom_command(TARGET ${target_name} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy -t $<TARGET_FILE_DIR:${target_name}> $<TARGET_RUNTIME_DLLS:${target_name}>
                COMMAND_EXPAND_LISTS
                COMMENT "Copying runtime DLLs to ${target_name} output directory"
            )
        endif()
    endif()
endfunction()
