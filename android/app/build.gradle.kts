plugins {
    id("com.android.application")
}

java {
    sourceCompatibility = JavaVersion.VERSION_21
    targetCompatibility = JavaVersion.VERSION_21
}

android {
    namespace = "org.libsdl.app"

    compileSdk = 35
    buildToolsVersion = "35.0.1"
    ndkVersion = "27.1.12297006"

    defaultConfig {
        applicationId = "ru.grinlexstydios.skylabs"

        minSdk = 21
        targetSdk = 35
        versionCode = 1
        versionName = "1.0"

        externalNativeBuild {
            cmake {
                arguments += listOf(
                    """
                        -DDEPS_CMAKE_ARGS='
                            -DCMAKE_TOOLCHAIN_FILE=${android.ndkDirectory.absolutePath}/build/cmake/android.toolchain.cmake
                            -DCMAKE_ANDROID_NDK="${android.ndkDirectory.absolutePath}"
                            -DANDROID_NDK="${android.ndkDirectory.absolutePath}"
                            -DANDROID_PLATFORM=android-21
                            -DANDROID_STL=c++_static
                            -DCMAKE_FIND_ROOT_PATH_MODE_PACKAGE=BOTH
                            -DCMAKE_FIND_ROOT_PATH_MODE_PROGRAM=BOTH
                            -DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=BOTH
                            -DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=BOTH
                        '
                    """.trimIndent().replace(Regex("\\s*\n\\s*"), " "), // Remove new lines
                    "-DANDROID_PLATFORM=android-21",
                    "-DANDROID_STL=c++_static",
                    "-DCMAKE_FIND_ROOT_PATH_MODE_PACKAGE=BOTH",
                    "-DCMAKE_FIND_ROOT_PATH_MODE_PROGRAM=BOTH",
                    "-DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=BOTH",
                    "-DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=BOTH",

                    "-DCMAKE_BUILD_PARALLEL_LEVEL=3",
                    "-Wno-deprecated"
                )

                abiFilters += listOf("arm64-v8a")
            }
        }
    }

    sourceSets.getByName("main") {
        java.setSrcDirs(listOf("../../libs/sources/SDL/android-project/app/src/main/java"))
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_21
        targetCompatibility = JavaVersion.VERSION_21
    }

    buildTypes {
        getByName("release") {
            isMinifyEnabled = false
            proguardFiles(getDefaultProguardFile("proguard-android.txt"), "proguard-rules.pro")
        }
    }

    externalNativeBuild {
        cmake {
            path = file("../../CMakeLists.txt")
        }
    }
}
