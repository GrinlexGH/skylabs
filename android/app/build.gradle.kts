@file:Suppress("UnstableApiUsage")

import com.android.build.gradle.tasks.ExternalNativeBuildTask
import java.nio.file.Files

plugins {
    id("com.android.application")
}

tasks.register("conanInstall") {
    val conanExecutable = "conan"
    val buildDir = file("../..")

    val buildTypes = listOf("Debug", "RelWithDebInfo")
    val architectures = listOf("armv8")

    val ndkPath = androidComponents.sdkComponents.ndkDirectory.get().asFile

    doLast {
        buildTypes.forEach { buildType ->
            architectures.forEach { arch ->
                val cmd = listOf(
                    conanExecutable,
                    "install", "${buildDir.absolutePath}",
                    "-r", "skylabs",
                    "-r", "conancenter",
                    "-pr", "android",
                    "-c", "tools.android:ndk_path=${ndkPath.absolutePath}",
                    "-s", "build_type=$buildType",
                    "-s", "arch=$arch",
                    "--build", "missing",
                    "-c", "tools.cmake.cmake_layout:build_folder_vars=['settings.arch']"
                )

                println(">> ${cmd.joinToString(" ")}")

                val process = ProcessBuilder(cmd)
                    .directory(buildDir)
                    .redirectErrorStream(true)
                    .start()

                process.inputStream.bufferedReader().forEachLine { line ->
                    println(line)
                }

                val exitCode = process.waitFor()
                if (exitCode != 0) {
                    throw GradleException("Conan failed with exit code $exitCode")
                }
            }
        }
    }
}

tasks.register("copyDebugSymbols") {
    doLast {
        val cmakeTask = tasks.withType<ExternalNativeBuildTask>()
            .firstOrNull { it.name.contains("Debug", ignoreCase = true) }

        if (cmakeTask != null) {
            val soDir = cmakeTask.soFolder
            val symlinkDir = File(projectDir, "build/symbols_latest")

            if (!symlinkDir.exists()) symlinkDir.mkdirs()

            val link = File(symlinkDir, "debug_libs")
            if (link.exists()) link.delete()

            Files.createSymbolicLink(
                link.toPath(),
                soDir.asFile.get().absoluteFile.toPath()
            )
            println("Symlink updated: ${link.absolutePath} -> ${soDir.asFile.get().absolutePath}")
        }
    }
}

tasks.named("preBuild").configure {
    dependsOn("conanInstall")
}

tasks.withType<ExternalNativeBuildTask>().configureEach {
    finalizedBy("copyDebugSymbols")
}

var toolchainFile = file("../../cmake/ConanAndroidToolchain.cmake")

android {
    namespace = "org.libsdl.app"

    compileSdk = 36
    ndkVersion = "30.0.14904198"

    defaultConfig {
        applicationId = "ru.grinlexstudios.skylabs"

        minSdk = 23
        targetSdk = 36

        versionCode = 1
        versionName = "1.0"

        externalNativeBuild {
            cmake {
                targets += listOf("launcher", "core", "shaders")

                arguments += listOf(
                    "-DCMAKE_TOOLCHAIN_FILE=${toolchainFile.absolutePath}",
                    "-DANDROID_STL=c++_static"
                )

                abiFilters += listOf("arm64-v8a")
            }
        }
    }

    externalNativeBuild {
        cmake {
            path = file("../../CMakeLists.txt")
            version = "4.1.0+"
        }
    }

    sourceSets.getByName("main") {
        java.directories += "src/main/java"
        assets.directories += "../../assets"
        assets.directories += "../../build/assets"
    }

    lint {
        lintConfig = file("lint.xml") // Disable some warnings from SDL shitty code
    }

    buildTypes {
        getByName("release") {
            isMinifyEnabled = true
            proguardFiles(
                getDefaultProguardFile("proguard-android-optimize.txt"),
                "proguard-rules.pro"
            )
        }
    }
}
