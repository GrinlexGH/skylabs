@file:Suppress("UnstableApiUsage")

import com.android.build.gradle.tasks.ExternalNativeBuildTask
import java.io.ByteArrayOutputStream
import java.nio.file.Files

plugins {
    id("com.android.application")
}

val projectRootFile = file("../../")

abstract class ConanInstallTask @Inject constructor(
    private val execOperations: ExecOperations
) : DefaultTask() {
    @get:InputFile
    @get:PathSensitive(PathSensitivity.RELATIVE)
    abstract val conanFile: RegularFileProperty

    @get:Input abstract val arch: Property<String>
    @get:Input abstract val buildType: Property<String>
    @get:Input abstract val ndkPath: Property<String>
    @get:Internal abstract val projectRoot: DirectoryProperty

    @get:OutputDirectory
    val outputDir: Provider<Directory> = projectRoot.flatMap { root ->
        val path = "build/${arch.get()}/${buildType.get()}"
        project.objects.directoryProperty().fileValue(root.asFile.resolve(path))
    }

    @TaskAction
    fun run() {
        val outputStream = ByteArrayOutputStream()
        val outFolder = outputDir.get().asFile

        val args = listOf(
            "install", projectRoot.get().asFile.absolutePath,
            "-r", "skylabs", "-r", "conancenter",
            "-pr", "android",
            "-c", "tools.android:ndk_path=${ndkPath.get()}",
            "-s", "build_type=${buildType.get()}",
            "-s", "arch=${arch.get()}",
            "--build", "missing",
            "-c", "tools.cmake.cmake_layout:build_folder_vars=['settings.arch']",
        )

        logger.lifecycle(">> conan ${args.joinToString(" ")}")

        execOperations.exec {
            commandLine("conan")
            args(args)
            workingDir = projectRoot.get().asFile
            standardOutput = outputStream
            errorOutput = outputStream
            isIgnoreExitValue = true
        }

        outputStream.toString().lines().forEach { line ->
            if (line.isNotBlank()) logger.lifecycle("CONAN: $line")
        }
    }
}

var toolchainFile = projectRootFile.resolve("cmake/ConanAndroidToolchain.cmake")

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

fun mapAndroidAbiToConan(abi: String): String = when (abi) {
    "arm64-v8a" -> "armv8"
    "armeabi-v7a" -> "armv7"
    else -> abi
}

androidComponents {
    onVariants { variant ->
        val variantName = variant.buildType.toString()
        val configName = if (variant.buildType?.equals("release", ignoreCase = true) == true) "RelWithDebInfo" else "Debug"
        val abis = variant.externalNativeBuild?.abiFilters?.get() ?: listOf("arm64-v8a")

        // Setup conan tasks
        val conanTaskProviders = abis.map { abi ->
            val conanArch = mapAndroidAbiToConan(abi)
            tasks.register<ConanInstallTask>("conanInstall${configName}_$abi") {
                val conanTxt = projectRootFile.resolve("conanfile.txt")
                val conanPy = projectRootFile.resolve("conanfile.py")
                conanFile.set(if (conanPy.exists()) conanPy else conanTxt)
                buildType.set(configName)
                arch.set(conanArch)
                ndkPath.set(androidComponents.sdkComponents.ndkDirectory.get().asFile.absolutePath)
                projectRoot.set(projectRootFile)
            }
        }

        // Copy debug symbols for vscode
        val copySymbolsProvider = if (variant.name.contains("debug", ignoreCase = true)) {
            tasks.register("copySymbols$variantName") {
                group = "developer"
                doLast {
                    val nativeTask = tasks.withType<ExternalNativeBuildTask>()
                        .firstOrNull { it.name.contains(variantName, ignoreCase = true) }

                    val soDir = nativeTask?.soFolder?.get()?.asFile ?: return@doLast
                    val linkPath = projectDir.resolve("build/symbols_latest/debug_libs").toPath()

                    Files.createDirectories(linkPath.parent)
                    if (Files.exists(linkPath)) Files.delete(linkPath)

                    try {
                        Files.createSymbolicLink(linkPath, soDir.toPath())
                        logger.lifecycle(">> Symlink updated: $linkPath -> $soDir")
                    } catch (e: Exception) {
                        logger.warn(">> Fallback to copy: ${e.message}")
                        soDir.copyRecursively(linkPath.toFile(), overwrite = true)
                    }
                }
            }
        } else null

        // Setup dependencies
        tasks.configureEach {
            if (name.startsWith("configureCMake$configName")) {
                conanTaskProviders.forEach { dependsOn(it) }
            }
            if (name.startsWith("buildCMake$configName")) {
                copySymbolsProvider?.let { finalizedBy(it) }
            }
        }

        // Generate assets before merge
        val generatedAssetsDir = layout.projectDirectory.dir("../../build/assets")
        variant.sources.assets?.addStaticSourceDirectory(generatedAssetsDir.toString())

        tasks.configureEach {
            if (name == "merge${variantName}Assets") {
                dependsOn(tasks.matching { it.name.startsWith("buildCMake$configName") })

                logger.lifecycle(">> Linked merge${variantName}Assets to CMake build")
            }
        }
    }
}
