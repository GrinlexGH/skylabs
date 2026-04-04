set(CONAN_ARCH "unknown")
if(${ANDROID_ABI} STREQUAL "arm64-v8a")
    set(CONAN_ARCH "armv8")
elseif(${ANDROID_ABI} STREQUAL "armeabi-v7a")
    set(CONAN_ARCH "armv7")
elseif(${ANDROID_ABI} STREQUAL "x86_64" OR ${ANDROID_ABI} STREQUAL "x86")
    set(CONAN_ARCH ${ANDROID_ABI})
endif()

message(STATUS "Toolchain: ABI=${ANDROID_ABI}, BuildType=${CMAKE_BUILD_TYPE}")

set(CONAN_TARGET_FILE "${CMAKE_CURRENT_LIST_DIR}/../build/${CONAN_ARCH}/${CMAKE_BUILD_TYPE}/generators/conan_toolchain.cmake")

if(EXISTS "${CONAN_TARGET_FILE}")
    include("${CONAN_TARGET_FILE}")
else()
    message(STATUS "Conan toolchain NOT FOUND at ${CONAN_TARGET_FILE} (This is normal during CMake probes)")
endif()
