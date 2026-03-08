# Author: Grinlex
#
# Imported targets:
# ``Slangc::slangc``
# ``Vulkan::slangc``
#
# Result variables:
# ``Slangc_FOUND``
# ``Slangc_EXECUTABLE``
#
# Searches slangc executable in these folders:
#  $ENV{VULKAN_SDK}/bin
#  $ENV{VULKAN_SDK}/bin32

find_program(Slangc_EXECUTABLE
    NAMES slangc
    HINTS
        $ENV{VULKAN_SDK}/bin
        $ENV{VULKAN_SDK}/bin32
    DOC "Path to the slangc shader compiler"
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Slangc
    REQUIRED_VARS Slangc_EXECUTABLE
)

if(Slangc_FOUND AND NOT TARGET Slangc::slangc)
    add_executable(Slangc::slangc IMPORTED)
    set_target_properties(Slangc::slangc PROPERTIES
        IMPORTED_LOCATION "${Slangc_EXECUTABLE}"
    )

    if(NOT TARGET Vulkan::slangc)
        add_executable(Vulkan::slangc ALIAS Slangc::slangc)
    endif()
endif()

mark_as_advanced(Slangc_EXECUTABLE)
