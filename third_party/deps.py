# Author: Grinlex

# Example of usage:
# python deps.py \
#   --cache-dir=caches \
#   --sources-dir=third_party/src \
#   --install-dir=third_party/bin \
#   --cmake-args='-G "Ninja Multi-Config" -DCMAKE_POLICY_DEFAULT_CMP0091=NEW' \
#   add-header-lib \
#     --src=tinyobjloader \
#     --glob=tiny_obj_loader.h \
#   add-cmake-lib \
#     --src=SDL \
#     --install=SDL3 \
#     --build-dir=build_cmake \
#     --args=-DSDL_TEST_LIBRARY=OFF \
#   add-cmake-lib \
#     --src=SDL_image \
#     --install=SDL3_image \
#     --args='-DSDLIMAGE_AVIF=OFF -DSDLIMAGE_WEBP=OFF' \
#   add-header-lib \
#     --src=simple_term_colors \
#     --glob=include/stc.hpp \
#   add-manual-lib \
#     --src=SteamworksSDK \
#     --install=SteamworksSDK \
#       rule --src='redistributable_bin/**/*.dll'    --dst=bin \
#       rule --src='public/steam/lib/**/*.dll'       --dst=bin \
#       rule --src='public/steam/*.h'                --dst=include/steam --ex='*.h' \
#       rule --src='redistributable_bin/**/*.lib'    --dst=lib \
#       rule --src='redistributable_bin/**/*.so'     --dst=lib \
#       rule --src='redistributable_bin/**/*.dylib'  --dst=lib \
#       rule --src='public/steam/lib/**/*.lib'       --dst=lib \
#       rule --src='public/steam/lib/**/*.so'        --dst=lib \
#       rule --src='public/steam/lib/**/*.dylib'     --dst=lib


import argparse
import itertools
import os
import platform
import shlex
import shutil
import subprocess
import sys
from enum import IntEnum
from glob import glob
from pathlib import Path
from typing import Generic, Optional, TypeVar

SOURCES_ROOT: Path
INSTALL_ROOT: Path
CACHE_ROOT: Optional[Path]
HEADER_SUBDIR: Path
CMAKE: str
CMAKE_GLOBAL_ARGS: list[str]


class LogType(IntEnum):
    Info = 0
    Success = 1
    Warning = 2
    Error = 3


class LogLevel(IntEnum):
    Normal = 0
    V1 = 1
    V2 = 2
    V3 = 3


class TerminalColors:
    OKBLUE = "\033[94m"
    OKGREEN = "\033[92m"
    WARNING = "\033[93m"
    FAIL = "\033[91m"
    ENDC = "\033[0m"

# TODO: more logging
CURRENT_LOG_LEVEL: LogLevel = LogLevel.Normal


def log(message, log_type: LogType = LogType.Info, log_level: LogLevel = LogLevel.Normal):
    if log_level > CURRENT_LOG_LEVEL:
        return

    if log_type == LogType.Error:
        print(f"{TerminalColors.FAIL}{message}{TerminalColors.ENDC}", flush=True)
    elif log_type == LogType.Warning:
        print(f"{TerminalColors.WARNING}{message}{TerminalColors.ENDC}", flush=True)
    elif log_type == LogType.Success:
        print(f"{TerminalColors.OKGREEN}{message}{TerminalColors.ENDC}", flush=True)
    else:
        print(f"{TerminalColors.OKBLUE}{message}{TerminalColors.ENDC}", flush=True)


class InstallingLibrary(object):
    lib_name: str
    source_dir: Path
    install_dir: Path
    source_dir_base: Path
    install_dir_base: Path
    git_hash: Optional[str]

    def __init__(self, source_dir_base: Path, install_dir_base: Path) -> None:
        self.lib_name = source_dir_base.name
        self.source_dir = SOURCES_ROOT / source_dir_base
        self.install_dir = INSTALL_ROOT / install_dir_base
        self.source_dir_base = source_dir_base
        self.install_dir_base = install_dir_base
        self.git_hash = None

    @staticmethod
    def WriteLineAt(path: Path, n: int, text: str) -> None:
        path.parent.mkdir(parents=True, exist_ok=True)

        if path.exists():
            with path.open("r", encoding="utf-8") as f:
                lines = f.readlines()
        else:
            lines = []

        while len(lines) < n:
            lines.append("\n")

        lines[n - 1] = text.rstrip("\n") + "\n"
        with path.open("w", encoding="utf-8") as f:
            f.writelines(lines)

    @staticmethod
    def ReadLineAt(path: Path, n: int) -> Optional[str]:
        if not path.exists():
            return None

        with path.open("r", encoding="utf-8") as f:
            for i, line in enumerate(f, start=1):
                if i == n:
                    return line.rstrip("\n")

        return None

    def BuildAndInstall(self) -> None:
        raise NotImplementedError

    def GetGitHash(self) -> str:
        if self.git_hash is None:
            self.git_hash = subprocess.run(
                ["git", "-C", str(self.source_dir), "rev-parse", "HEAD"],
                capture_output=True, text=True, check=True
            ).stdout.strip()
        return self.git_hash

    def CheckGitHash(self, hash_file: Path) -> bool:
        try:
            return self.GetGitHash() == self.ReadLineAt(hash_file, 1)
        except subprocess.CalledProcessError as e:
            log(f"Failed to get git hash for {self.source_dir}!\nError: {e}", LogType.Error)
        return False

    def IsHashRelevant(self, hash_file: Path) -> bool:
        if hash_file.exists() and self.CheckGitHash(hash_file):
            return True
        return False

    def WriteHash(self, hash_file) -> None:
        self.WriteLineAt(hash_file, 1, self.GetGitHash())

    def InstallLibrary(self) -> None:
        global SOURCES_ROOT, INSTALL_ROOT, CACHE_ROOT

        hash_file: Path = (
            CACHE_ROOT / self.install_dir_base if CACHE_ROOT else self.install_dir
        ) / f"hash_{self.lib_name}.txt"

        if self.IsHashRelevant(hash_file):
            log(f"[{self.lib_name}] is up to date.")
            return

        if not self.install_dir.exists():
            self.install_dir.mkdir(parents=True)

        log(f"Installing [{self.lib_name}]...")

        self.BuildAndInstall()

        self.WriteHash(hash_file)

        log(f"[{self.lib_name}] installed.", LogType.Success)


class CMakeLibrary(InstallingLibrary):
    build_dir: Path
    extra_args: list[str]
    build_hash: Optional[str]
    build_debug: bool

    def __init__(
        self,
        source_dir_base: Path,
        install_dir_base: Path,
        build_dir: Path | None = None,
        extra_args: list[str] | None = None,
        build_debug: bool = False
    ) -> None:
        super().__init__(source_dir_base, install_dir_base)
        self.extra_args = extra_args or []
        self.build_dir = build_dir or Path("build")
        self.build_hash = None
        self.build_debug = build_debug

    # For parallel work of this script we need to lock the build dir
    @staticmethod
    def _AcquireLock(lock_file: Path):
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

    @staticmethod
    def IsGeneratorMultiConfig(build_dir: Path) -> bool:
        try:
            cache = build_dir / "CMakeCache.txt"
            generator: str = ""

            with cache.open() as f:
                for line in f:
                    if line.startswith("CMAKE_GENERATOR:"):
                        generator = line.split("=")[1].strip()

            if (generator.startswith("Visual Studio") or
                generator in ("Ninja Multi-Config", "FASTBuild", "Xcode")
            ):
                return True
        except OSError:
            pass
        return False

    def GetBuildHash(self):
        if self.build_hash is None:
            import hashlib

            global CMAKE_GLOBAL_ARGS
            data_str = "|".join(self.extra_args + CMAKE_GLOBAL_ARGS)
            self.build_hash = hashlib.md5(data_str.encode()).hexdigest()
        return self.build_hash

    def CheckBuildHash(self, hash_file: Path):
        try:
            return self.GetBuildHash() == self.ReadLineAt(hash_file, 2)
        except subprocess.CalledProcessError as e:
            log(f"Failed to get git hash for {self.source_dir}!\nError: {e}", LogType.Error)
        return False

    def IsHashRelevant(self, hash_file) -> bool:
        return super().IsHashRelevant(hash_file) and self.CheckBuildHash(hash_file)

    def WriteHash(self, hash_file) -> None:
        super().WriteHash(hash_file)
        self.WriteLineAt(hash_file, 2, self.GetBuildHash())

    def BuildAndInstall(self) -> None:
        log(f"Compiling [{self.lib_name}]...")

        # Prepare build dir to allow multiple instance of this script at one time
        build_dir: Path = self.source_dir / self.build_dir
        n = 0
        lock = None

        stage = "acquire lock file"
        try:
            # Lock directory
            while True:
                lock_file = build_dir / ".lock"
                lock = CMakeLibrary._AcquireLock(lock_file)
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
                    build_dir = self.source_dir / f"{self.build_dir}-{n}"

            # Configure
            stage = "configure"
            cmake_cmd = [
                CMAKE,
                "..",
                f"-DCMAKE_INSTALL_PREFIX={self.install_dir}",
                f"-DCMAKE_PREFIX_PATH={INSTALL_ROOT}",
            ] + self.extra_args + CMAKE_GLOBAL_ARGS

            configs = ["Release"]
            if self.build_debug:
                configs.append("Debug")
                cmake_cmd.append("-DCMAKE_DEBUG_POSTFIX=d")

            subprocess.run(cmake_cmd, cwd=build_dir, check=True)

            isMulti = self.IsGeneratorMultiConfig(build_dir)
            needToCleanupInstall = True

            for config in configs:
                if isMulti:
                    build_cmd = [ CMAKE, "--build", ".", "--config", config, "--parallel" ] # todo: job count argument
                else:
                    # Reconfigure
                    cmake_cmd = [ CMAKE, f"-DCMAKE_BUILD_TYPE={config}", ".." ]
                    stage = "reconfigure"
                    subprocess.run(cmake_cmd, cwd=build_dir, check=True)
                    build_cmd = [ CMAKE, "--build", ".", "--parallel" ] # todo: job count argument

                # Build
                stage = "build"
                subprocess.run(build_cmd, cwd=build_dir, check=True)

                log(f"[{self.lib_name}] successfully built" + (f" in {config} configuration." if len(configs) > 1 else "."),
                    LogType.Success
                )

                stage = "cleanup install folder"
                if needToCleanupInstall:
                    if self.install_dir.exists():
                        shutil.rmtree(self.install_dir)
                    self.install_dir.mkdir(parents=True)
                    needToCleanupInstall = False

                # Install
                stage = "install"
                if isMulti:
                    install_cmd = [ CMAKE, "--install", ".", "--config", config ]
                    subprocess.run(install_cmd, cwd=build_dir, check=True)
                else:
                    install_cmd = [ CMAKE, "--install", "." ]
                    subprocess.run(install_cmd, cwd=build_dir, check=True)
        except Exception:
            log(f"Failed to {stage}!", LogType.Error)
            raise
        finally:
            if lock is not None:
                lock.close() # Unlock the build folder
            shutil.rmtree(build_dir)


class ManualLibrary(InstallingLibrary):
    rules: list[tuple[str, str, str]]

    def __init__(self, source_dir_base: Path, install_dir_base: Path, rules: list[tuple[str, str, str]] | None = None) -> None:
        super().__init__(source_dir_base, install_dir_base)
        self.rules = rules or []

    @staticmethod
    def _SplitPattern(pattern: str) -> tuple[Path, str]:
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
            if any(ch in part for ch in "*?["):
                fixed = Path(*parts[:i])
                sub = "/".join(parts[i:])
                return fixed, sub
        return Path(*parts), ""

    def BuildAndInstall(self) -> None:
        for pattern, dst_subdir, exclude_pattern in self.rules:
            fixed_prefix, sub_pattern = ManualLibrary._SplitPattern(pattern)
            glob_root = self.source_dir / fixed_prefix

            if not glob_root.exists():
                log(f"Pattern base path not found: {glob_root}", LogType.Warning)
                continue

            if glob_root.is_file():
                if exclude_pattern != "":
                    log(f"There is no need in exclude glob '{exclude_pattern}' if you copy path.\n"
                        "Exclude glob excludes files only from glob.", LogType.Warning
                    )
                target = self.install_dir / dst_subdir / glob_root.name
                target.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(glob_root, target)
                continue

            matches = set(Path(p) for p in glob(str(glob_root / sub_pattern), recursive=True))

            exclude_matches = set()
            if exclude_pattern:
                ex_root = self.source_dir / fixed_prefix / exclude_pattern

                if ex_root.exists():
                    exclude_matches = set(Path(p) for p in glob(str(ex_root), recursive=True))

            to_copy = matches - exclude_matches

            for full_path in to_copy:
                try:
                    rel_path = full_path.relative_to(glob_root)
                except ValueError:
                    log(f"Failed to compute relative path for {full_path}", LogType.Warning)
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
            [(p, str(install_dir_base), "") for p in (paths or [])]
        )


T = TypeVar("T", bound=InstallingLibrary)
class LibraryCommand(Generic[T]):
    parser: argparse.ArgumentParser

    def __init__(self, parser: argparse.ArgumentParser):
        self.parser = parser

    @staticmethod
    def GetName() -> str:
        raise NotImplementedError

    @staticmethod
    def GetLibType() -> type[T]:
        raise NotImplementedError

    def Parse(self, args: list[str]) -> argparse.Namespace:
        return self.parser.parse_args(args)

    def _CreateLibrary(self, namespace: argparse.Namespace) -> T:
        raise NotImplementedError

    T = TypeVar("T", bound=InstallingLibrary)
    def CreateLibrary(self, args: list[str]) -> T:
        return self._CreateLibrary(self.Parse(args))


class CMakeCommand(LibraryCommand[CMakeLibrary]):
    def __init__(self):
        super().__init__(self._CreateParser())

    @staticmethod
    def GetName() -> str:
        return "add-cmake-lib"

    @staticmethod
    def GetLibType() -> type[CMakeLibrary]:
        return CMakeLibrary

    def _CreateParser(self) -> argparse.ArgumentParser:
        parser = argparse.ArgumentParser(
            prog='add-cmake-lib',
            description=(
                "Add a CMake-based library to build and install.\n"
                "This command defines a single library that will be configured, built,\n"
                "and installed using CMake."
            ),
            formatter_class=type('CustomFormatter', (argparse.ArgumentDefaultsHelpFormatter, argparse.RawTextHelpFormatter), {})
        )
        parser.add_argument(
            '--src', type=str, required=True,
            help="Source subfolder (relative to --sources-dir)."
        )
        parser.add_argument(
            '--install', type=str, default="",
            help="Install subfolder (relative to --install-dir). (Default: same as --src)"
        )
        parser.add_argument(
            '--build-dir', type=str, default="build",
            help="CMake build subfolder (relative to --src). (Default: 'build')"
        )
        parser.add_argument(
            '--args', type=str, default="",
            help="Extra CMake configure arguments (as a single quoted string)."
        )
        parser.add_argument(
            '--build-debug', action="store_true",
            help="Also build Debug configuration after building Release. Useful for installing static libraries."
        )
        return parser

    def _CreateLibrary(self, namespace: argparse.Namespace) -> CMakeLibrary:
        try:
            extra_cmake_args = shlex.split(namespace.args)
        except ValueError as e:
            raise ValueError(f"Failed to parse cmake args for {namespace.src}!\nError: {e}")

        return CMakeLibrary(
            source_dir_base=Path(namespace.src),
            install_dir_base=Path(namespace.install),
            build_dir=Path(namespace.build_dir),
            extra_args=extra_cmake_args,
            build_debug=namespace.build_debug
        )


class HeaderCommand(LibraryCommand[HeaderLibrary]):
    def __init__(self):
        super().__init__(self._CreateParser())

    @staticmethod
    def GetName() -> str:
        return "add-header-lib"

    @staticmethod
    def GetLibType() -> type[HeaderLibrary]:
        return HeaderLibrary

    def _CreateParser(self) -> argparse.ArgumentParser:
        parser = argparse.ArgumentParser(
            prog='add-header-lib',
            description=(
                "Install a header-only library.\n"
                "This command copies header files from a source directory to the \n"
                "installation tree using glob patterns."
            ),
            formatter_class=type('CustomFormatter', (argparse.ArgumentDefaultsHelpFormatter, argparse.RawTextHelpFormatter), {})
        )
        parser.add_argument(
            '--src', type=str, required=True,
            help="Source subfolder (relative to --sources-dir)."
        )
        parser.add_argument(
            '--install-subdir', type=str, default="",
            help="Install subfolder (relative to --header-subdir). (Default: root of --header-subdir)"
        )
        parser.add_argument(
            '--glob', type=str, required=True, action='append',
            help="Glob pattern for headers. Can be used multiple times."
        )
        return parser

    def _CreateLibrary(self, namespace: argparse.Namespace) -> HeaderLibrary:
        return HeaderLibrary(
            source_dir_base=Path(namespace.src),
            install_dir_base=Path(namespace.install_subdir),
            paths=namespace.glob
        )


class ManualCommand(LibraryCommand[ManualLibrary]):
    rule_parser: argparse.ArgumentParser

    def __init__(self):
        self.rule_parser = self._CreateRulesParser()
        super().__init__(self._CreateParser())

    @staticmethod
    def GetName() -> str:
        return "add-manual-lib"

    @staticmethod
    def GetLibType() -> type[ManualLibrary]:
        return ManualLibrary

    def _CreateParser(self) -> argparse.ArgumentParser:
        parser = argparse.ArgumentParser(
            prog='add-manual-lib',
            description=(
                "Define manual copy/install rules for a library.\n"
                "This command uses one or more 'rule' sub-commands to copy files\n"
                "and preserve directory structures.\n\n"

                "When copying directories that include wildcards, a constant prefix is ignored.\n"
                "For example, the rule --src 'redistributable_bin/**/*.dll' --install 'bin' will copy 'steam_api64.dll' located at\n"
                "<SOURCE_SUBDIR>/redistributable_bin/win64 into <INSTALL_SUBDIR>/win64/bin."
            ),
            formatter_class=argparse.RawTextHelpFormatter
        )
        parser.add_argument(
            '--src', type=str, required=True,
            help="Source subfolder (relative to --sources-dir)."
        )
        parser.add_argument(
            '--install', type=str, required=True,
            help="Install subfolder (relative to --install-dir)."
        )
        parser.add_argument(
            'rules', nargs=argparse.REMAINDER,
            help="A list of 'rule' sub-commands. (See '... rule --help' for details)"
        )
        return parser

    def _CreateRulesParser(self) -> argparse.ArgumentParser:
        parser = argparse.ArgumentParser(
            prog='rule',
            description="Defines a single file copy rule for 'add-manual-lib'."
        )
        parser.add_argument(
            '--src', type=str, required=True,
            help="Source glob pattern (relative to parent's --src)."
        )
        parser.add_argument(
            '--dst', type=str, required=True,
            help="Destination subfolder (relative to parent's --install)."
        )
        parser.add_argument(
            '--ex', type=str, default=None,
            help="Glob pattern for files/dirs to exclude from this rule (relative to --src constant prefix)."
        )
        return parser

    def _CreateLibrary(self, namespace: argparse.Namespace) -> ManualLibrary:
        rules = []
        rule_args_list = namespace.rules

        rule_indices = [i for i, x in enumerate(rule_args_list) if x == 'rule']

        if not rule_indices or rule_args_list[0] != 'rule':
            raise ValueError("Invalid --manual-lib syntax for {namespace.src}. Expected 'rule' sub-commands.")

        for i in range(len(rule_indices)):
            # Get one rule
            start = rule_indices[i] + 1
            end = rule_indices[i+1] if (i + 1) < len(rule_indices) else len(rule_args_list)
            rule_args = rule_args_list[start:end]

            try:
                rule_namespace = self.rule_parser.parse_args(rule_args)
                rules.append((rule_namespace.src, rule_namespace.dst, rule_namespace.ex or ""))
            except Exception as e:
                raise ValueError(f"Failed to parse 'rule' args: {rule_args}. Error: {e}")

        return ManualLibrary(
            source_dir_base=Path(namespace.src),
            install_dir_base=Path(namespace.install),
            rules=rules
        )


def register_commands() -> list[type[LibraryCommand]]:
    return [
        CMakeCommand,
        HeaderCommand,
        ManualCommand
    ]


def create_main_parser():
    main_parser = argparse.ArgumentParser(
        description=(
            "Universal dependency builder and installer.\n"
            "Builds and installs third-party libraries from source into a local output tree."
        ),
        formatter_class=type('CustomFormatter', (argparse.ArgumentDefaultsHelpFormatter, argparse.RawTextHelpFormatter), {})
    )
    main_parser.add_argument(
        "--sources-dir", type=Path, default=Path("src"),
        help="Root directory containing library source code. (Default: 'src')"
    )
    main_parser.add_argument(
        "--install-dir", type=Path, default=Path("bin") / platform.system(),
        help="Root directory for built library installations. (Default: 'bin/<platform>')"
    )
    main_parser.add_argument(
        "--cache-dir", type=str, default=None,
        help="Directory for hash files. (Default: '<INSTALL_DIR>/<library_installation_folder>')"
    )
    main_parser.add_argument(
        "--cmake", type=str, default="cmake",
        help="Path to the CMake executable. (Default: 'cmake')"
    )
    main_parser.add_argument(
        "--cmake-args", type=str, default="",
        help="Global CMake arguments (as a single quoted string) applied to all libraries."
    )
    main_parser.add_argument(
        "--header-subdir", type=Path, default=Path("header-only"),
        help="Subdirectory under <INSTALL_DIR> for header-only libraries. (Default: 'header-only')"
    )

    return main_parser


# For every argument from sys.argv, this function returns a grouping key (count, cmd).
# The key stays the same for all consecutive arguments belonging to the same command.
# When a new command token is encountered, 'count' is incremented (indicating that
# this is other group) and 'cmd' changes. itertools.groupby() will then group
# arguments until the key changes, effectively collecting each command with its
# parameters into a separate list.
#
# inspired by https://stackoverflow.com/a/10449310/16793487
def groupargs(arg, state={'cmd': None, 'count': 0}, known_commands: list[str] | None = None):
    if arg in known_commands:
        state['cmd'] = arg
        state['count'] += 1
    elif state['cmd'] is None:
        state['cmd'] = 'global'
        state['count'] = 0

    # (0, 'global'), (1, 'add-cmake-lib'), (2, 'add-cmake-lib'), ...
    return (state['count'], state['cmd'])


# todo: add --parallel argument for parallel build
def main():
    global \
        SOURCES_ROOT, \
        INSTALL_ROOT, \
        CACHE_ROOT, \
        HEADER_SUBDIR, \
        CMAKE, \
        CMAKE_GLOBAL_ARGS

    main_parser = create_main_parser()
    main_namespace = argparse.Namespace()
    libraries: list[InstallingLibrary] = []

    COMMANDS = register_commands()
    COMMAND_NAMES = [ command.GetName() for command in COMMANDS ]
    COMMAND_MAP = { command.GetName(): command for command in COMMANDS }

    # Group commands
    command_groups = [ (key, list(args))
                       for key, args in itertools.groupby(sys.argv[1:], lambda arg: groupargs(arg, known_commands=COMMAND_NAMES)) ]

    # Acquire global args
    global_group_args = []
    if command_groups and command_groups[0][0][1] == 'global':
        _, args_list = command_groups.pop(0)
        global_group_args = args_list

    # Global variables initialization
    main_parser.parse_args(global_group_args or [], namespace=main_namespace)

    SOURCES_ROOT = main_namespace.sources_dir
    INSTALL_ROOT = main_namespace.install_dir
    CACHE_ROOT = Path(main_namespace.cache_dir) if main_namespace.cache_dir else None
    HEADER_SUBDIR = main_namespace.header_subdir
    CMAKE = main_namespace.cmake

    try:
        CMAKE_GLOBAL_ARGS = shlex.split(main_namespace.cmake_args)
    except ValueError as e:
        log(f"Failed to parse global --cmake-args!\nError: {e}", LogType.Error)
        sys.exit(1)

    for key, cmdline in command_groups:
        cmd_name = key[1]

        if not cmdline:
            log(f"No arguments passed to {cmd_name}", LogType.Warning)
            continue

        try:
            command_handler = COMMAND_MAP[cmd_name]()
            libraries.append(command_handler.CreateLibrary(cmdline[1:]))
        except Exception as e:
            log(f"Failed to process command: {' '.join(cmdline)}!\nError: {e}", LogType.Error)
            sys.exit(1)

    for library in libraries:
        if not library.source_dir.exists():
            log(f"Source folder not found: {library.source_dir}", LogType.Warning)
            continue

        try:
            library.InstallLibrary()
        except Exception as e:
            log(f"Failed to process {library.lib_name}!\nError: {e}", LogType.Error)
            sys.exit(1)

    if not(libraries):
        log("Nothing to do.")
    else:
        log("All libraries installed successfully", LogType.Success)


if __name__ == "__main__":
    main()
