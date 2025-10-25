# Author: Grinlex

# Example of usage:
# python install_dependencies.py \
#   --cache-dir caches \
#   --cmake-lib VulkanMemoryAllocator-Hpp/Vulkan-Headers VulkanHeaders build "-DVULKAN_HEADERS_ENABLE_TESTS=OFF -DVULKAN-HEADERS_ENABLE_MODULE=ON" \
#   --cmake-lib VulkanMemoryAllocator-Hpp/VulkanMemoryAllocator VulkanMemoryAllocator build "-DVMA_BUILD_DOCUMENTATION=OFF -DVMA_BUILD_SAMPLES=OFF -DVMA_ENABLE_INSTALL=ON" \
#   --cmake-lib VulkanMemoryAllocator-Hpp VulkanMemoryAllocator-Hpp build "-DVMA_HPP_ENABLE_INSTALL=ON" \
#   --cmake-lib SDL SDL3 build "-DSDL_TEST_LIBRARY=OFF" \
#   --cmake-lib SDL_image SDL3_image build "-DSDLIMAGE_AVIF=OFF -DSDLIMAGE_LBM=OFF -DSDLIMAGE_PCX=OFF -DSDLIMAGE_TIF=OFF -DSDLIMAGE_XCF=OFF -DSDLIMAGE_XPM=OFF -DSDLIMAGE_XV=OFF -DSDLIMAGE_WEBP=OFF" \
#   --header-lib tinyobjloader "" tiny_obj_loader.h \
#   --header-lib simple_term_colors "" include/stc.hpp \
#   --manual-lib SteamworksSDK SteamworksSDK \
#       "redistributable_bin/**/*.dll" bin \
#       "public/steam/lib/**/*.dll" bin \
#       "public/steam/*.h" include/steam \
#       "redistributable_bin/**/*.lib" lib \
#       "redistributable_bin/**/*.so" lib \
#       "redistributable_bin/**/*.dylib" lib \
#       "public/steam/lib/**/*.lib" lib \
#       "public/steam/lib/**/*.so" lib \
#       "public/steam/lib/**/*.dylib" lib


import argparse
import os
import platform
import shlex
import shutil
import subprocess
import sys
from enum import IntEnum
from glob import glob
from pathlib import Path
from typing import TypeVar

SOURCES_ROOT: Path
INSTALL_ROOT: Path
CACHE_ROOT: Path
HEADER_SUBDIR: Path
CMAKE: str
CMAKE_GLOBAL_ARGS: list[str]

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


# ? maybe I should add compilation argument hash or file hash?
def get_git_hash(source_dir: Path) -> str:
    result = subprocess.run(
        ["git", "-C", str(source_dir), "rev-parse", "HEAD"],
        capture_output=True, text=True, check=True
    )
    return result.stdout.strip()

def check_git_hash_match(source_dir: Path, hash_file: Path) -> bool:
    try:
        current_hash: str = get_git_hash(source_dir)
        if hash_file.exists():
            with open(hash_file, "r", encoding="utf-8") as f:
                existing_hash: str = f.read().strip()
            return current_hash == existing_hash
    except subprocess.CalledProcessError as e:
        log(f"Failed to get git hash for {source_dir}: {e}", LogLevel.Error)
    return False


class InstallingLibrary(object):
    lib_name: str
    source_dir: Path
    install_dir: Path
    source_dir_base: Path
    install_dir_base: Path

    def __init__(self, source_dir_base: Path, install_dir_base: Path) -> None:
        self.lib_name = source_dir_base.name
        self.source_dir = SOURCES_ROOT / source_dir_base
        self.install_dir = INSTALL_ROOT / install_dir_base
        self.source_dir_base = source_dir_base
        self.install_dir_base = install_dir_base

    def BuildAndInstall(self) -> None:
        raise NotImplementedError

    def InstallLibrary(self) -> None:
        global SOURCES_ROOT, INSTALL_ROOT, CACHE_ROOT

        hash_file: Path = (CACHE_ROOT / self.install_dir_base if CACHE_ROOT else self.install_dir) / f"git_hash_{self.lib_name}.txt"

        if hash_file.exists() and check_git_hash_match(self.source_dir, hash_file):
            log(f"[{self.lib_name}] is up to date.")
            return

        if not self.install_dir.exists():
            self.install_dir.mkdir(parents=True)

        log(f"Installing [{self.lib_name}]...")

        self.BuildAndInstall()

        current_hash: str = get_git_hash(self.source_dir)
        hash_file.parent.mkdir(parents=True, exist_ok=True)
        with open(hash_file, "w", encoding="utf-8") as f:
            f.write(current_hash)

        log(f"[{self.lib_name}] installed.", LogLevel.Success)


# For parallel work of this script we need to lock the build dir
def acquire_lock(lock_file: Path):
    lock_file.parent.mkdir(parents=True, exist_ok=True)
    f = open(lock_file, "w")

    try:
        if os.name == "nt":
            import msvcrt
            msvcrt.locking(f.fileno(), msvcrt.LK_NBLCK, 1)
        else:
            import fcntl
            fcntl.flock(f, fcntl.LOCK_EX | fcntl.LOCK_NB)
        return f
    except (OSError, BlockingIOError):
        f.close()
        return None

class CMakeLibrary(InstallingLibrary):
    build_folder: Path
    extra_cmake_flags: list[str]

    def __init__(self, source_dir_base: Path, install_dir_base: Path, build_folder: Path | None = None, extra_cmake_flags: list[str] | None = None) -> None:
        super().__init__(source_dir_base, install_dir_base)
        self.extra_cmake_flags = extra_cmake_flags or []
        self.build_folder = build_folder or Path("build")

    def BuildAndInstall(self) -> None:
        log(f"Compiling [{self.lib_name}]...")

        # Prepare build dir to allow multiple instance of this script at one time
        build_dir: Path = self.source_dir / self.build_folder
        n = 0
        lock = None

        while True:
            lock_file = build_dir / ".lock"
            lock = acquire_lock(lock_file)
            if lock is not None:
                # Delete all files and folders except .lock file
                for item in build_dir.iterdir():
                    if item.name == lock_file.name:
                        continue
                    if item.is_dir():
                        shutil.rmtree(item)
                    else:
                        item.unlink()
                break
            else:
                # Use build-{n} folder instead
                n += 1
                build_dir = self.source_dir / f"{self.build_folder}-{n}"

        cmake_cmd = [
            CMAKE,
            "-DCMAKE_BUILD_TYPE=Release", # ? what if user uses multi-config generator?
            f"-DCMAKE_INSTALL_PREFIX={self.install_dir}",
            f"-DCMAKE_PREFIX_PATH={INSTALL_ROOT}",
            ".."
        ] + self.extra_cmake_flags + CMAKE_GLOBAL_ARGS

        subprocess.run(cmake_cmd, cwd=build_dir, check=True)

        build_cmd = [CMAKE, "--build", ".", "--config", "Release", "--parallel"] # todo: job count argument
        subprocess.run(build_cmd, cwd=build_dir, check=True)

        lock.close() # Unlock the build folder

        log(f"[{self.lib_name}] successfully built.", LogLevel.Success)

        if self.install_dir.exists():
            shutil.rmtree(self.install_dir)
        self.install_dir.mkdir(parents=True)

        install_cmd = [CMAKE, "--install", ".", "--config", "Release"]
        subprocess.run(install_cmd, cwd=build_dir, check=True)

        shutil.rmtree(build_dir)


def split_pattern(pattern: str) -> tuple[Path, str]:
    """
    Splits a path pattern into a fixed prefix and a wildcard sub-pattern.
    `fixed_prefix` is the path up to (but not including) the first part containing a wildcard (*, ?, [).
    `sub_pattern` is the remaining part of the path starting from the first wildcard.

    Example:
    `"redistributable_bin/**/*.dll"` ->
    `(Path("redistributable_bin"), "**/*.dll")`
    """
    parts = Path(pattern).parts

    for i, part in enumerate(parts):
        if any(ch in part for ch in '*?['):
            fixed = Path(*parts[:i])
            sub = "/".join(parts[i:])
            return fixed, sub
    return Path(*parts), ""

class ManualLibrary(InstallingLibrary):
    rules: list[tuple[str, str]]

    def __init__(self, source_dir_base: Path, install_dir_base: Path, rules: list[tuple[str, str]] | None = None) -> None:
        super().__init__(source_dir_base, install_dir_base)
        self.rules = rules or []

    def BuildAndInstall(self) -> None:
        for pattern, dst_subdir in self.rules:
            fixed_prefix, sub_pattern = split_pattern(pattern)
            glob_root = self.source_dir / fixed_prefix

            if not glob_root.exists():
                log(f"Pattern base path not found: {glob_root}", LogLevel.Warning)
                continue

            if glob_root.is_file():
                target = self.install_dir / dst_subdir / glob_root.name
                target.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(glob_root, target)
                continue

            search_pattern = str(glob_root / sub_pattern)
            matches = glob(search_pattern, recursive=True)

            for full_path in matches:
                full_path = Path(full_path)

                try:
                    rel_path = full_path.relative_to(glob_root)
                except ValueError:
                    log(f"Failed to compute relative path for {full_path}", LogLevel.Warning)
                    continue

                target = self.install_dir / dst_subdir / rel_path

                if full_path.is_dir():
                    shutil.copytree(full_path, target, dirs_exist_ok=True)
                else:
                    target.parent.mkdir(parents=True, exist_ok=True)
                    shutil.copy2(full_path, target)


class HeaderLibrary(ManualLibrary):
    def __init__(self, source_dir_base: Path, install_dir_base: Path | None = None, paths: list[str] | None = None) -> None:
        global HEADER_SUBDIR
        super().__init__(
            source_dir_base,
            HEADER_SUBDIR,
            [(p, str(install_dir_base)) for p in (paths or [])]
        )


def skip_if_missing(lib_folder: Path) -> bool:
    if not lib_folder.exists():
        log(f"Source folder not found: {lib_folder}", LogLevel.Warning)
        return True
    return False

T = TypeVar("T", bound=InstallingLibrary)
def install_libraries(libraries: list[T]) -> None:
    for library in libraries:
        if skip_if_missing(library.source_dir):
            continue

        try:
            library.InstallLibrary()
        except subprocess.CalledProcessError:
            log(f"Failed to build or install {library.lib_name}", LogLevel.Error)
            sys.exit(1)


def parse_cmake_libs(args_list: list[list[str]]) -> list[CMakeLibrary]:
    libs: list[CMakeLibrary] = []

    for group in args_list:
        if len(group) != 4:
            log(f"Invalid --cmake-lib syntax: {group}", LogLevel.Error)
            sys.exit(1)

        source_dir_base = group[0]
        install_dir_base = group[1]
        build_dir = group[2]
        try:
            extra_cmake_args = shlex.split(group[3])
        except ValueError as e:
            log(f"Failed to parse cmake args for {source_dir_base}: {e}", LogLevel.Error)
            sys.exit(1)

        libs.append(CMakeLibrary(Path(source_dir_base), Path(install_dir_base), Path(build_dir), extra_cmake_args))

    return libs

def parse_header_libs(args_list: list[list[str]]) -> list[HeaderLibrary]:
    libs = []

    for group in args_list:
        if len(group) < 3:
            log(f"Invalid --header-lib syntax: {group}", LogLevel.Error)
            sys.exit(1)

        source_subdir = group[0]
        install_subdir = group[1]
        paths = group[2:]

        libs.append(HeaderLibrary(Path(source_subdir), Path(install_subdir), paths))

    return libs

def parse_manual_install_libs(args_list: list[list[str]]) -> list[ManualLibrary]:
    libs = []

    for group in args_list:
        if len(group) < 4 or len(group[2:]) % 2 != 0:
            log(f"Invalid --manual-lib syntax: {group}", LogLevel.Error)
            sys.exit(1)

        source_subdir = group[0]
        install_subdir = group[1]
        pairs = [(group[i], group[i + 1]) for i in range(2, len(group), 2)]

        libs.append(ManualLibrary(Path(source_subdir), Path(install_subdir), pairs))

    return libs


# todo: add --parallel argument for parallel build
def main():
    parser = argparse.ArgumentParser(
        description=(
            "Universal dependency builder and installer.\n"
            "Builds and installs third-party libraries from source into a local output tree."
        ),
        formatter_class=type('CustomFormatter', (argparse.ArgumentDefaultsHelpFormatter, argparse.RawTextHelpFormatter), {})
    )

    parser.add_argument(
        "--sources-dir", type=Path, default=Path("src"),
        help="Path that contains library source directories."
    )
    parser.add_argument(
        "--install-dir", type=Path, default=Path("bin") / platform.system(),
        help="Installation output directory. Default is 'bin/<Platform>'."
    )
    parser.add_argument(
        "--cache-dir", type=str, default="",
        help="Directory used for caching (e.g. git hash cache)."
    )
    parser.add_argument(
        "--cmake", type=str, default="cmake",
        help="Path to the CMake executable to use for CMake-based libraries."
    )
    parser.add_argument(
        "--cmake-args", type=str, default="",
        help=(
            "Global CMake arguments applied to all libraries. "
            "Provide as a single quoted string, e.g. '-G \"Ninja\" -DCMAKE_TOOLCHAIN_FILE=...'."
        )
    )
    parser.add_argument(
        "--cmake-lib", nargs=4, action="append",
        metavar=("SOURCE_SUBDIR", "INSTALL_SUBDIR", "BUILD_SUBDIR", "CMAKE_CONFIGURE_ARGS"),
        help=(
            "Add a CMake-based library to build and install. This option may be repeated.\n\n"
            "Arguments (in order):\n"
            "  SOURCE_SUBDIR         - subfolder inside <SOURCES_DIR> that contains the library source.\n"
            "  INSTALL_SUBDIR        - subfolder inside <INSTALL_DIR> where installed files will be placed.\n"
            "  BUILD_SUBDIR          - subfolder inside SOURCE_SUBDIR where CMake build files are generated.\n"
            "  CMAKE_CONFIGURE_ARGS  - quoted CMake configure options passed as a single argument."
        )
    )
    parser.add_argument(
        "--header-subdir", type=Path, default=Path("header-only"),
        help="Subdirectory under <INSTALL_DIR> where header-only libraries will be installed."
    )
    parser.add_argument(
        "--header-lib", nargs="+", action="append", metavar="ARGS",
        help=(
            "Install a header-only library. This option may be repeated.\n\n"
            "Format: SOURCE_SUBDIR INSTALL_SUBDIR <HEADER_GLOB> [<HEADER_GLOB> ...]\n"
            "  SOURCE_SUBDIR  - subfolder under <SOURCES_DIR> containing the headers.\n"
            "  INSTALL_SUBDIR - destination subfolder under <HEADER_SUBDIR>.\n"
            "  HEADER_GLOB    - one or more glob patterns selecting header files to install."
        )
    )
    parser.add_argument(
        "--manual-lib", nargs="+", action="append", metavar="ARGS",
        help=(
            "Define manual copy/install rules. This option may be repeated.\n\n"
            "Format: SOURCE_SUBDIR INSTALL_SUBDIR <pattern1> <dst1> [<pattern2> <dst2> ...]\n"
            "  SOURCE_SUBDIR  - subfolder under <SOURCES_DIR> containing files to copy.\n"
            "  INSTALL_SUBDIR - destination subfolder under <INSTALL_DIR>.\n"
            "  patternN dstN  - file glob pattern (relative to SOURCE_SUBDIR) and the destination subfolder\n"
            "                   (relative to INSTALL_SUBDIR) where matches will be copied.\n\n"
            "Wildcards are supported. When copying directories that include wildcards, a constant prefix is ignored.\n"
            "For example, the rule 'redistributable_bin/**/*.dll' 'bin' will copy 'steam_api64.dll' located at\n"
            "<SOURCE_SUBDIR>/redistributable_bin/win64 into <INSTALL_SUBDIR>/win64/bin."
        )
    )

    args = parser.parse_args()

    global SOURCES_ROOT, INSTALL_ROOT, CACHE_ROOT, HEADER_SUBDIR, CMAKE, CMAKE_GLOBAL_ARGS

    SOURCES_ROOT = args.sources_dir
    INSTALL_ROOT = args.install_dir
    CACHE_ROOT = args.cache_dir

    HEADER_SUBDIR = args.header_subdir

    CMAKE = args.cmake
    CMAKE_GLOBAL_ARGS = shlex.split(args.cmake_args)

    cmake_libraries: list[CMakeLibrary] = parse_cmake_libs(args.cmake_lib or [])
    install_libraries(cmake_libraries)

    header_libraries: list[HeaderLibrary] = parse_header_libs(args.header_lib or [])
    install_libraries(header_libraries)

    manual_install_libraries: list[ManualLibrary] = parse_manual_install_libs(args.manual_lib or [])
    install_libraries(manual_install_libraries)

    if not(cmake_libraries) and not(header_libraries) and not(manual_install_libraries):
        log("Nothing to do.")
    else:
        log("All libraries installed successfully", LogLevel.Success)

if __name__ == "__main__":
    main()
