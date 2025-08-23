plugins {
    id("com.android.application")
}

val projectRoot = "../.."
val sdlBase = "$projectRoot/libs/sources/SDL/android-project/app"

android {
    namespace = "org.libsdl.app"

    compileSdk = 36
    ndkVersion = "29.0.13846066"

    defaultConfig {
        applicationId = "ru.grinlexstydios.skylabs"

        minSdk = 21
        targetSdk = 36

        versionCode = 1
        versionName = "0.0"

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

                abiFilters += listOf("arm64-v8a", "armeabi-v7a")
            }
        }
    }

    externalNativeBuild {
        cmake {
            path = file("$projectRoot/CMakeLists.txt")
            version = "4.1.0"
        }
    }

    sourceSets.getByName("main") {
        java.srcDirs("$sdlBase/src/main/java")
        assets.srcDirs("src/main/assets/**")
    }

    lint {
        lintConfig = file("lint.xml") // Disable some warning from SDL shitty code
    }

    buildTypes {
        getByName("release") {
            isMinifyEnabled = true
            proguardFiles(
                getDefaultProguardFile("proguard-android.txt"),
                "$sdlBase/proguard-rules.pro"
            )
        }
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_21
        targetCompatibility = JavaVersion.VERSION_21
    }
}
