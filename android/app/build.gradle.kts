@file:Suppress("UnstableApiUsage")


plugins {
    id("com.android.application")
}

abstract class ConanInstallTask @Inject constructor(
    private val execOperations: ExecOperations
) : DefaultTask() {

    @get:InputFile
    @get:PathSensitive(PathSensitivity.RELATIVE)
    abstract val conanFile: RegularFileProperty

    @get:Input abstract val arch: Property<String>
    @get:Input abstract val apiLevel: Property<Int>
    @get:Input abstract val buildType: Property<String>
    @get:Input abstract val ndkPath: Property<String>

    @get:OutputDirectory
    abstract val outputDir: DirectoryProperty

    @TaskAction
    fun run() {
        val conanfileDir = conanFile.get().asFile.parentFile

        val args = listOf(
            "install", conanfileDir.absolutePath,
            "-r", "skylabs", "-r", "conancenter",           // You can modify this line for your remotes like artifactory
            "-pr", "android",
            "-s", "arch=${arch.get()}",
            "-s", "os.api_level=${apiLevel.get()}",
            "-s", "build_type=${buildType.get()}",
            "-s", "compiler.version=21",                    //!!! Depends on this file configuration, dont forget to update !!!
            "-s", "compiler.libcxx=c++_static",             //!!! Depends on this file configuration, dont forget to update !!!
            "-c", "tools.android:ndk_path=${ndkPath.get()}",
            "--build", "missing",
            "-c", "tools.cmake.cmake_layout:build_folder_vars=['settings.arch']",
        )

        logger.lifecycle(">> conan ${args.joinToString(" ")}")

        execOperations.exec {
            commandLine("conan")
            args(args)
            workingDir = conanfileDir
        }
    }
}

val projectRootFile = file("../../")
var toolchainFile = projectRootFile.resolve("cmake/ConanAndroidToolchain.cmake")

android {
    namespace = "org.libsdl.app"

    compileSdk = 37
    ndkVersion = "30.0.14904198"

    defaultConfig {
        applicationId = "ru.grinlexstudios.skylabs"

        minSdk = 30
        targetSdk = 37

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
        assets.directories += "src/main/assets"
        assets.directories += "../../assets"
    }

    lint {
        lintConfig = file("lint.xml") // Disable some warnings from SDL shitty code
    }

    buildTypes {
        getByName("release") {
            isMinifyEnabled = true
            isShrinkResources = true
            proguardFiles(
                getDefaultProguardFile("proguard-android-optimize.txt"),
                "proguard-rules.pro"
            )
        }
    }
}

androidComponents {
    tasks.register("prepareKotlinBuildScriptModel")

    onVariants { variant ->
        val variantName = variant.name.replaceFirstChar { it.uppercase() }
        val configName = if (variant.buildType == "release") "RelWithDebInfo" else "Debug"
        val abis = variant.externalNativeBuild?.abiFilters?.get() ?: listOf("arm64-v8a")

        // Setup conan tasks
        val conanTaskProviders = abis.map { abi ->
            tasks.register<ConanInstallTask>("conanInstall${configName}[${abi}]") {
                val conanfileTxt = projectRootFile.resolve("conanfile.txt")
                val conanfilePy = projectRootFile.resolve("conanfile.py")
                val conanArch = when (abi) {
                    "arm64-v8a" -> "armv8"
                    "armeabi-v7a" -> "armv7"
                    else -> abi
                }

                conanFile.set(if (conanfilePy.exists()) conanfilePy else conanfileTxt)
                arch.set(conanArch)
                apiLevel.set(android.defaultConfig.minSdk!!)
                buildType.set(configName)
                ndkPath.set(androidComponents.sdkComponents.ndkDirectory.map { it.asFile.absolutePath })

                outputDir.set(projectRootFile.resolve("build/$conanArch/$configName"))
            }
        }

        // Run conan before cmake configure
        tasks.matching { it.name.startsWith("configureCMake$configName") }.configureEach {
            dependsOn(conanTaskProviders)
        }

        // Android studio hack
        tasks.findByName("prepareKotlinBuildScriptModel")?.dependsOn(conanTaskProviders)

        // Merge assets after build
        tasks.matching { it.name == "merge${variantName}Assets" }.configureEach {
            dependsOn(tasks.matching { it.name.startsWith("buildCMake$configName") })
        }
    }
}
