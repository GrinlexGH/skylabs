# Author: Grinlex

import argparse
import platform
import re
import shlex
import shutil
import subprocess
import sys
import urllib.request
import zipfile
from enum import IntEnum
from pathlib import Path

SOURCES_ROOT = Path()
INSTALL_ROOT = Path()
CMAKE = "cmake"
CMAKE_GLOBAL_ARGS = list[str]()
CMAKE_PERSUBMODULE_ARGS = dict[str, list[str]]()

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

def build_library(source_dir_base: Path, install_dir_base: Path, extra_cmake_flags: list[str] = []) -> None:
    global SOURCES_ROOT, INSTALL_ROOT, CMAKE, CMAKE_GLOBAL_ARGS, CMAKE_PERSUBMODULE_ARGS

    lib_name = source_dir_base.name
    source_dir = SOURCES_ROOT / source_dir_base
    install_dir = INSTALL_ROOT / install_dir_base
    hash_file = install_dir / "git_hash.txt"

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

    if install_dir.exists():
        shutil.rmtree(install_dir)
    install_dir.mkdir(parents=True)

    install_cmd = [CMAKE, "--install", ".", "--config", "Release"]
    subprocess.run(install_cmd, cwd=build_dir, check=True)

    shutil.rmtree(build_dir)

    current_hash = get_git_hash(source_dir)
    with open(hash_file, "w", encoding="utf-8") as f:
        f.write(current_hash)

    log(f"[{lib_name}] build complete.", LogLevel.Success)

def install_header_only_library(source_dir_base: Path, install_dir_base: Path, header_paths: list[str]):
    global SOURCES_ROOT, INSTALL_ROOT

    lib_name = source_dir_base.name
    source_dir = SOURCES_ROOT / source_dir_base
    install_dir = INSTALL_ROOT / "header-only" / install_dir_base
    hash_file = install_dir / f"git_hash_{lib_name}.txt"

    if hash_file.exists() and check_git_hash_match(source_dir, hash_file):
        log(f"[{lib_name}] is up to date.")
        return

    if not install_dir.exists():
        install_dir.mkdir(parents=True)

    for header in header_paths:
        src = source_dir / header
        dst = install_dir

        if src.is_file():
            dst.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(src, dst)
        elif src.is_dir():
            shutil.copytree(src, dst, dirs_exist_ok=True)
        else:
            log(f"[{lib_name}] Cannot find path: {src}", LogLevel.Warning)

    current_hash = get_git_hash(source_dir)
    with open(hash_file, "w", encoding="utf-8") as f:
        f.write(current_hash)

    log(f"[{lib_name}] installed.", LogLevel.Success)

def download_and_extract_zip(url, zip_path, extract_to):
    try:
        log(f"Downloading from {url} to {zip_path}", LogLevel.Info)
        urllib.request.urlretrieve(url, zip_path)
    except Exception as e:
        log(f"Failed to download: {e}", LogLevel.Error)
        return False

    log(f"Unzipping archive to {extract_to}", LogLevel.Info)
    try:
        with zipfile.ZipFile(zip_path, 'r') as zip_ref:
            for member in zip_ref.infolist():
                fixed_path = extract_to / member.filename.replace('\\', '/')
                if member.is_dir():
                    fixed_path.mkdir(parents=True, exist_ok=True)
                else:
                    fixed_path.parent.mkdir(parents=True, exist_ok=True)
                    with zip_ref.open(member) as source, open(fixed_path, 'wb') as target:
                        shutil.copyfileobj(source, target)
    except Exception as e:
        log(f"Failed to unzip archive: {e}", LogLevel.Error)
        return False

    return True

def install_steamworks_sdk(sizeof_void_p=8):
    global INSTALL_ROOT
    version = "1.62"
    arch = "x64" if sizeof_void_p == 8 else "x86"

    zip_name = f"steamworks_sdk_{arch}.zip"
    zip_path = INSTALL_ROOT / "downloads" / zip_name
    sdk_url = (
        f"https://github.com/julianxhokaxhiu/SteamworksSDKCI/releases/download/"
        f"{version}/SteamworksSDK-v{version}.0_{arch}.zip"
    )
    sdk_extract_dir = INSTALL_ROOT / f"steamworks_sdk_{arch}"

    if sdk_extract_dir.exists():
        log("[Steamworks SDK] is up to date.", LogLevel.Info)
        return

    zip_path.parent.mkdir(parents=True, exist_ok=True)

    success = download_and_extract_zip(sdk_url, zip_path, sdk_extract_dir)
    if not success:
        log("[Steamworks SDK] installation failed.", LogLevel.Error)

    log("[Steamworks SDK] installed.", LogLevel.Success)

def parse_cmake_lib_args(lib_arg_str: str) -> dict[str, list[str]]:
    """
    Parse library-specific cmake arguments passed as a string like:
    'SDL=(-G "Ninlogja Multi-Config") SDL_image=(-G "Ninja")'
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

def main():
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
    parser.add_argument("--cmake-sizeof-void-p", type=int, default=8,
        help=(
            "Size of void pointer from cmake for downloading pre-compiled libraries. "
            "Usually ${CMAKE_SIZE_OF_VOID_P} Default: 8"
        )
    )

    args = parser.parse_args()

    global SOURCES_ROOT, INSTALL_ROOT, CMAKE, CMAKE_GLOBAL_ARGS, CMAKE_PERSUBMODULE_ARGS

    root = Path(__file__).resolve().parent
    SOURCES_ROOT = root / args.sources_dir
    INSTALL_ROOT = root / args.install_dir

    CMAKE = args.cmake
    CMAKE_GLOBAL_ARGS = shlex.split(args.cmake_args)
    CMAKE_PERSUBMODULE_ARGS = parse_cmake_lib_args(args.cmake_lib_args)

    install_steamworks_sdk(args.cmake_sizeof_void_p)

    libraries = [
        ("SDL", "SDL3", ["-DSDL_TEST_LIBRARY=OFF"]),
        ("SDL_image", "SDL3_image", [
            "-DSDLIMAGE_AVIF=OFF", "-DSDLIMAGE_LBM=OFF",
            "-DSDLIMAGE_PCX=OFF", "-DSDLIMAGE_TIF=OFF",
            "-DSDLIMAGE_XCF=OFF", "-DSDLIMAGE_XPM=OFF",
            "-DSDLIMAGE_XV=OFF", "-DSDLIMAGE_WEBP=OFF"
        ]),
        ("VulkanMemoryAllocator-Hpp/VulkanMemoryAllocator", "VulkanMemoryAllocator", [
            "-DVMA_BUILD_DOCUMENTATION=OFF", "-DVMA_BUILD_SAMPLES=OFF"
        ]),
        ("VulkanMemoryAllocator-Hpp", "VulkanMemoryAllocator-Hpp", [
            "-DVMA_HPP_ENABLE_INSTALL=ON", "-DVMA_BUILD_EXAMPLE=OFF"
        ]),
    ]

    for lib_folder, install_subdir, flags in libraries:
        src_path = SOURCES_ROOT / lib_folder
        if not src_path.exists():
            log(f"Source folder not found: {src_path}", LogLevel.Warning)
            continue
        try:
            build_library(Path(lib_folder), Path(install_subdir), flags)
        except subprocess.CalledProcessError:
            log(f"Failed to compile {lib_folder}", LogLevel.Error)
            sys.exit(1)

    header_libraries = [
        ("tinyobjloader", "", ["tiny_obj_loader.h"]),
        ("simple_term_colors", "", ["include"]),
    ]

    for lib_folder, install_subdir, files in header_libraries:
        src_path = SOURCES_ROOT / lib_folder
        if not src_path.exists():
            log(f"Source folder not found: {src_path}", LogLevel.Warning)
            continue
        try:
            install_header_only_library(Path(lib_folder), Path(install_subdir), files)
        except subprocess.CalledProcessError:
            log(f"Failed to compile {lib_folder}", LogLevel.Error)
            sys.exit(1)

    log("All libraries installed successfully", LogLevel.Success)

if __name__ == "__main__":
    main()
