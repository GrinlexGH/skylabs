# Author: Grinlex

import argparse
import platform
import re
import shlex
import shutil
import subprocess
import sys
from enum import IntEnum
from pathlib import Path
from glob import glob

SOURCES_ROOT = Path()
INSTALL_ROOT = Path()
CMAKE = "cmake"
CMAKE_GLOBAL_ARGS = list[str]()
CMAKE_PERSUBMODULE_ARGS = dict[str, list[str]]()

# ========================================================================================
# region ====== Logging and terminal colors ==============================================

class LogLevel(IntEnum):
    Info = 0
    Success = 1
    Warning = 2
    Error = 3

class TerminalColors:
    OKBLUE = '\033[94m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'

def log(message, log_level:LogLevel=LogLevel.Info):
    if log_level == LogLevel.Error:
        print(f"{TerminalColors.FAIL}{message}{TerminalColors.ENDC}", flush=True)
    elif log_level == LogLevel.Warning:
        print(f"{TerminalColors.WARNING}{message}{TerminalColors.ENDC}", flush=True)
    elif log_level == LogLevel.Success:
        print(f"{TerminalColors.OKGREEN}{message}{TerminalColors.ENDC}", flush=True)
    else:
        print(f"{TerminalColors.OKBLUE}{message}{TerminalColors.ENDC}", flush=True)

# endregion === Logging and terminal colors ==============================================
# ========================================================================================

# ========================================================================================
# region ====== Git utilities ============================================================

def get_git_hash(source_dir: Path) -> str:
    result = subprocess.run(
        ["git", "-C", str(source_dir), "rev-parse", "HEAD"],
        capture_output=True, text=True, check=True
    )
    return result.stdout.strip()

def check_git_hash_match(source_dir: Path, hash_file: Path) -> bool:
    try:
        current_hash = get_git_hash(source_dir)
        if hash_file.exists():
            with open(hash_file, "r", encoding="utf-8") as f:
                existing_hash = f.read().strip()
            return current_hash == existing_hash
    except subprocess.CalledProcessError as e:
        log(f"Failed to get git hash for {source_dir}: {e}", LogLevel.Error)
    return False

# endregion === Git utilities ============================================================
# ========================================================================================

# ========================================================================================
# region ====== CMake libraries building and installation ================================

def build_and_install_library(source_dir_base: Path, install_dir_base: Path, extra_cmake_flags: list[str] = []) -> None:
    global SOURCES_ROOT, INSTALL_ROOT, CMAKE, CMAKE_GLOBAL_ARGS, CMAKE_PERSUBMODULE_ARGS

    lib_name = source_dir_base.name
    source_dir = SOURCES_ROOT / source_dir_base
    install_dir = INSTALL_ROOT / install_dir_base
    hash_file = install_dir / f"git_hash_{lib_name}.txt"

    if hash_file.exists() and check_git_hash_match(source_dir, hash_file):
        log(f"[{lib_name}] is up to date.")
        return

    log(f"Compiling [{lib_name}]...")

    build_dir = source_dir / "build"
    if build_dir.exists():
        shutil.rmtree(build_dir)
    build_dir.mkdir(parents=True, exist_ok=True)

    submodule_args = CMAKE_PERSUBMODULE_ARGS.get(source_dir.name, [])

    cmake_cmd = [
        CMAKE,
        "-DCMAKE_BUILD_TYPE=Release",
        f"-DCMAKE_INSTALL_PREFIX={install_dir}",
        f"-DCMAKE_PREFIX_PATH={INSTALL_ROOT}",
        ".."
    ] + extra_cmake_flags + CMAKE_GLOBAL_ARGS + submodule_args

    subprocess.run(cmake_cmd, cwd=build_dir, check=True)

    build_cmd = [CMAKE, "--build", ".", "--config", "Release", "--parallel"]
    subprocess.run(build_cmd, cwd=build_dir, check=True)
    log(f"[{lib_name}] successfully built.", LogLevel.Success)

    if install_dir.exists():
        shutil.rmtree(install_dir)
    install_dir.mkdir(parents=True)

    install_cmd = [CMAKE, "--install", ".", "--config", "Release"]
    subprocess.run(install_cmd, cwd=build_dir, check=True)

    shutil.rmtree(build_dir)

    current_hash = get_git_hash(source_dir)
    with open(hash_file, "w", encoding="utf-8") as f:
        f.write(current_hash)

    log(f"[{lib_name}] installed.", LogLevel.Success)

# endregion === CMake libraries building and installation ================================
# ========================================================================================

# ========================================================================================
# region ====== Manual installation libraries ============================================

def split_pattern(pattern: str) -> tuple[Path, str]:
    """
    Splits pattern on (fixed_prefix, sub_pattern):
    e.g. "redistributable_bin/**/*.dll" -> (Path("redistributable_bin"), "**/*.dll")
    """
    parts = Path(pattern).parts
    for i, part in enumerate(parts):
        if any(ch in part for ch in ("*", "?", "[")):
            fixed = Path(*parts[:i]) if i > 0 else Path()
            sub = "/".join(parts[i:])
            return fixed, sub
    return Path(*parts), ""

def install_manual_install_libraries(source_dir_base: Path, install_dir_base: Path, rules: list[tuple[str, str]]):
    global SOURCES_ROOT, INSTALL_ROOT

    lib_name = source_dir_base.name
    source_dir = SOURCES_ROOT / source_dir_base
    install_dir = INSTALL_ROOT / install_dir_base
    hash_file = install_dir / f"git_hash_{lib_name}.txt"

    if hash_file.exists() and check_git_hash_match(source_dir, hash_file):
        log(f"[{lib_name}] is up to date.")
        return

    if not install_dir.exists():
        install_dir.mkdir(parents=True)

    for pattern, dst_subdir in rules:
        fixed_prefix, sub_pattern = split_pattern(pattern)
        glob_root = source_dir / fixed_prefix

        if not glob_root.exists():
            log(f"Pattern base path not found: {glob_root}", LogLevel.Warning)
            continue

        search_pattern = str(glob_root / sub_pattern) if sub_pattern else str(glob_root)
        matches = glob(search_pattern, recursive=True)

        for full_path in matches:
            full_path = Path(full_path)

            try:
                rel_path = full_path.relative_to(glob_root)
            except ValueError:
                log(f"Failed to compute relative path for {full_path}", LogLevel.Warning)
                continue

            use_flat = "**" not in sub_pattern
            if use_flat:
                target = install_dir / dst_subdir / full_path.name
            else:
                target = install_dir / dst_subdir / rel_path

            if full_path.is_file():
                target.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(full_path, target)

            elif full_path.is_dir():
                shutil.copytree(full_path, target, dirs_exist_ok=True)

    current_hash = get_git_hash(source_dir)
    with open(hash_file, "w", encoding="utf-8") as f:
        f.write(current_hash)

    log(f"[{lib_name}] installed.", LogLevel.Success)

# endregion === Manual installation libraries ============================================
# ========================================================================================

# ========================================================================================
# region ====== Header only installation =================================================

def install_header_only_library(source_dir_base: Path, install_dir_base: Path, paths: list[str]):
    global SOURCES_ROOT, INSTALL_ROOT

    dst_subdir = Path("header-only") / install_dir_base
    rules = [(path, str(install_dir_base)) for path in paths]

    install_manual_install_libraries(source_dir_base, dst_subdir, rules)

# endregion === Header only installation =================================================
# ========================================================================================

# ========================================================================================
# region ====== Main library installation functions ======================================

def skip_if_missing(lib_folder: str) -> bool:
    global SOURCES_ROOT

    src_path = SOURCES_ROOT / lib_folder
    if not src_path.exists():
        log(f"Source folder not found: {src_path}", LogLevel.Warning)
        return True
    return False

def build_and_install_all_libraries(libraries: list[tuple[str, str, list[str]]]):
    for lib_folder, install_subdir, flags in libraries:
        if skip_if_missing(lib_folder):
            continue
        try:
            build_and_install_library(Path(lib_folder), Path(install_subdir), flags)
        except subprocess.CalledProcessError:
            log(f"Failed to compile {lib_folder}", LogLevel.Error)
            sys.exit(1)

def install_all_header_only_libraries(header_libraries: list[tuple[str, str, list[str]]]):
    for lib_folder, install_subdir, files in header_libraries:
        if skip_if_missing(lib_folder):
            continue

        install_header_only_library(Path(lib_folder), Path(install_subdir), files)

def install_all_manual_install_libraries(manual_install_libraries: list[tuple[str, str, list[tuple[str, str]]]]):
    for lib_folder, install_subdir, rules in manual_install_libraries:
        if skip_if_missing(lib_folder):
            continue

        install_manual_install_libraries(Path(lib_folder), Path(install_subdir), rules)

# endregion === Main library installation functions ======================================
# ========================================================================================

# ========================================================================================
# region ====== CMake argument parsing ===================================================

def parse_cmake_lib_args(lib_arg_str: str) -> dict[str, list[str]]:
    """
    Parse library-specific cmake arguments passed as a string like:
    'SDL=(-G "Ninja Multi-Config") SDL_image=(-G "Ninja")'
    into a dictionary: {"SDL": ["-G", "Ninja Multi-Config"], "SDL_image": ["-G", "Ninja"]}
    """
    result = {}
    pattern = re.compile(r'(\w+)=\((.*?)\)')
    for match in pattern.finditer(lib_arg_str):
        lib_name = match.group(1)
        raw_args = match.group(2)
        try:
            args = shlex.split(raw_args)
            result[lib_name] = args
        except ValueError as e:
            log(f"Failed to parse arguments for {lib_name}: {e}", LogLevel.Warning)
    return result

# endregion === CMake argument parsing ===================================================
# ========================================================================================

def main():
    # region ===== Argument parsing ===========================
    parser = argparse.ArgumentParser(
        description="Build and install project dependencies using CMake with optional per-library arguments."
    )
    parser.add_argument(
        "--sources-dir", type=Path, default=Path("sources"),
        help="Path to directory containing library subfolders. Default: ./sources"
    )
    parser.add_argument(
        "--install-dir", type=Path, default=Path("bin") / platform.system(),
        help="Installation output directory. Default: ./bin/<System>"
    )
    parser.add_argument(
        "--cmake", type=str, default="cmake",
        help="Path to cmake executable file. Default: cmake"
    )
    parser.add_argument(
        "--cmake-args", type=str, default="",
        help="Global cmake arguments for all libraries. "
             "Example: --cmake-args '-G \"Ninja\" -DCMAKE_TOOLCHAIN_FILE=...'"
    )
    parser.add_argument(
        "--cmake-lib-args", type=str, default="",
        help=(
            "Library-specific cmake arguments. Use format: name=(args). Example: "
            "--cmake-lib-args 'zlib=(-DSKIP_EXAMPLES=ON) SDL=(-DOPTION=VALUE)'"
        )
    )

    args = parser.parse_args()
    # endregion == Argument parsing ===========================

    # region ===== Global variables setup =====================
    global SOURCES_ROOT, INSTALL_ROOT, CMAKE, CMAKE_GLOBAL_ARGS, CMAKE_PERSUBMODULE_ARGS

    root = Path(__file__).resolve().parent
    SOURCES_ROOT = root / args.sources_dir
    INSTALL_ROOT = root / args.install_dir

    CMAKE = args.cmake
    CMAKE_GLOBAL_ARGS = shlex.split(args.cmake_args)
    CMAKE_PERSUBMODULE_ARGS = parse_cmake_lib_args(args.cmake_lib_args)
    # endregion == Global variables setup =====================

    # region ===== Build and install libraries with CMake =====
    # Source folder | Install folder | cmake arguments
    libraries = [
        ("VulkanMemoryAllocator-Hpp/VulkanMemoryAllocator", "VulkanMemoryAllocator", [
            "-DVMA_BUILD_DOCUMENTATION=OFF", "-DVMA_BUILD_SAMPLES=OFF"
        ]),
        ("VulkanMemoryAllocator-Hpp", "VulkanMemoryAllocator-Hpp", [
            "-DVMA_HPP_ENABLE_INSTALL=ON", "-DVMA_BUILD_EXAMPLE=OFF"
        ]),
        ("SDL", "SDL3", ["-DSDL_TEST_LIBRARY=OFF"]),
        ("SDL_image", "SDL3_image", [
            "-DSDLIMAGE_AVIF=OFF", "-DSDLIMAGE_LBM=OFF",
            "-DSDLIMAGE_PCX=OFF", "-DSDLIMAGE_TIF=OFF",
            "-DSDLIMAGE_XCF=OFF", "-DSDLIMAGE_XPM=OFF",
            "-DSDLIMAGE_XV=OFF", "-DSDLIMAGE_WEBP=OFF"
        ]),
    ]

    build_and_install_all_libraries(libraries)
    # endregion == Build and install libraries with CMake =====

    # region ===== Install header-only libraries ==============
    # Source folder | Install subfolder | files or folders
    header_libraries = [
        ("tinyobjloader", "", ["tiny_obj_loader.h"]),
        ("simple_term_colors", "", ["include/*"]),
    ]

    install_all_header_only_libraries(header_libraries)
    # endregion === Install header-only libraries =============

    # region ===== Install manually specified libraries =======
    # Source folder | Install folder | Rules (pattern, destonation)
    manual_install_libraries = [
        (
            "SteamworksSDK",
            "SteamworksSDK",
            [
                ( "redistributable_bin/**/*.dll",   "bin" ),
                ( "public/steam/lib/**/*.dll",      "bin" ),
                ( "public/steam/*.h",               "include/steam" ),
                ( "redistributable_bin/**/*.lib",   "lib" ),
                ( "redistributable_bin/**/*.so",    "lib" ),
                ( "redistributable_bin/**/*.dylib", "lib" ),
                ( "public/steam/lib/**/*.lib",      "lib" ),
                ( "public/steam/lib/**/*.so",       "lib" ),
                ( "public/steam/lib/**/*.dylib",    "lib" ),
            ],
        )
    ]

    install_all_manual_install_libraries(manual_install_libraries)
    # endregion == Install manually specified libraries =======

    log("All libraries installed successfully", LogLevel.Success)

if __name__ == "__main__":
    main()
