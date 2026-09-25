# engine
set(CURRENT_TARGET_NAME engine)

add_library(${CURRENT_TARGET_NAME} MODULE)
set_target_properties(${CURRENT_TARGET_NAME} PROPERTIES PREFIX "" IMPORT_PREFIX "")

configure_file(${PROJECT_SOURCE_DIR}/cmake/project_info.hpp.in project_info.hpp @ONLY)

target_sources(${CURRENT_TARGET_NAME}
    PRIVATE
    FILE_SET SOURCES FILES
        vma_implementation.cpp
        main.cpp
        filesystem.cpp
        logging.cpp
        os.cpp
        sdl/context.cpp
        sdl/sdl.cpp
        sdl/window.cpp
        sdl/event_pump.cpp
        sdl/filesystem.cpp
        sdl/log_sink.cpp
        sdl/vulkan_adapter.cpp
        sdl/vulkan.cpp
        render/vulkan/renderer.cpp
        render/vulkan/swapchain.cpp
        render/vulkan/image.cpp
        render/vulkan/context/context.cpp
        render/vulkan/context/allocator.cpp
    FILE_SET HEADERS FILES
        utils.hpp
        window.hpp
        event_pump.hpp
        filesystem.hpp
        logging.hpp
        os.hpp
        sdl/context.hpp
        sdl/sdl.hpp
        sdl/window.hpp
        sdl/event_pump.hpp
        sdl/filesystem.hpp
        sdl/log_sink.hpp
        sdl/vulkan_adapter.hpp
        sdl/vulkan.hpp
        render/renderer.hpp
        render/vulkan/in_flight.hpp
        render/vulkan/os_adapter.hpp
        render/vulkan/renderer.hpp
        render/vulkan/image.hpp
        render/vulkan/swapchain.hpp
        render/vulkan/context/instance.hpp
        render/vulkan/context/surface.hpp
        render/vulkan/context/physical_device.hpp
        render/vulkan/context/device.hpp
        render/vulkan/context/allocator.hpp
    FILE_SET generated TYPE HEADERS
    BASE_DIRS ${CMAKE_CURRENT_BINARY_DIR}
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/sk_${CURRENT_TARGET_NAME}_export.h
        ${CMAKE_CURRENT_BINARY_DIR}/project_info.hpp
)

target_precompile_headers(${CURRENT_TARGET_NAME} PRIVATE pch.hpp)

add_dependencies(${CURRENT_TARGET_NAME} shaders)

target_link_libraries(${CURRENT_TARGET_NAME} PRIVATE
    SDL3::SDL3
    Boost::nowide
    fmt::fmt
    frozen::frozen
    glm::glm
    Vulkan::Headers
    vk-bootstrap::vk-bootstrap
    VulkanMemoryAllocator-Hpp::VulkanMemoryAllocator-Hpp
)

skylabs_configure_target(${CURRENT_TARGET_NAME} LIBRARY_DESTINATION $<IF:$<PLATFORM_ID:Windows>,bin,lib>)
