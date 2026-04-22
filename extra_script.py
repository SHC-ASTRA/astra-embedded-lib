# Don't try to fix the Import statements. Don't try to add a shebang.
# This file won't really work with a linter... because PlatformIO.

import subprocess
import os
import struct
from pathlib import Path
from sys import stderr
from warnings import warn
from inspect import getframeinfo, stack

from typing import TYPE_CHECKING, Any

if TYPE_CHECKING:
    env: Any
    projenv: Any

# SCons Imports
Import("env")
Import("projenv")
global_env = DefaultEnvironment()

DEBUG = False


def main():
    debug_print("DEBUGGING ENABLED: This is how you will see debug messages.")
    setup_lib_versioning()  # Called as a part of *-embedded-Lib build process

    old_wd = os.getcwd()
    os.chdir(global_env.subst("$BUILD_DIR"))
    setup_project_versioning()

    os.chdir(old_wd)


def get_repo_name():
    repo_path = subprocess.run(
        ["git", "rev-parse", "--show-toplevel"], capture_output=True, text=True
    )
    if repo_path.returncode != 0:
        warn("Warning: Could not determine git repository path.")
        return None
    else:
        repo_path = repo_path.stdout.strip()
        repo_name = Path(repo_path).parts[-1]
        debug_print(f"Current Git repository: {repo_name}")
        return repo_name


def detect_versioning():
    # Get git release version
    git_release = subprocess.run(
        ["git", "describe", "--tags", "--abbrev=0"], capture_output=True, text=True
    )

    # Defaults/fallbacks
    is_main = 0
    is_dirty = 0
    commit_hash = 0
    version_major = 0
    version_minor = 0
    version_patch = 0
    hash_parts = (0, 0)

    # Set release major/minor/patch
    if git_release.returncode == 0:
        version_str = git_release.stdout.strip()
        version_parts = version_str.lstrip("v").split(".")
        version_parts += ["0"] * (3 - len(version_parts))
        version_major = int(version_parts[0])
        version_minor = int(version_parts[1])
        version_patch = int(version_parts[2])

    # Set ismain to true if on main branch
    git_branch = subprocess.run(
        ["git", "rev-parse", "--abbrev-ref", "HEAD"], capture_output=True, text=True
    )
    if git_branch.returncode == 0:
        is_main = int(git_branch.stdout.strip() == "main")

    # Set isdirty to 1 if there are uncommitted changes
    git_status = subprocess.run(
        ["git", "status", "--porcelain"], capture_output=True, text=True
    )
    if git_status.returncode == 0:
        is_dirty = int(git_status.stdout.strip() != "")

    # Get current short commit hash
    git_hash = subprocess.run(
        ["git", "rev-parse", "HEAD"], capture_output=True, text=True
    )
    if git_hash.returncode == 0:
        commit_hash = git_hash.stdout.strip()[:8]
        debug_print(f"Current Git commit hash: {commit_hash}")
        commit_hash = int(commit_hash, 16)  # convert hex string to integer
        hash_parts = struct.unpack("<hh", commit_hash.to_bytes(4, "big"))
    else:
        warn("Warning: Could not determine git commit hash.")

    return (
        version_major,
        version_minor,
        version_patch,
        is_main,
        is_dirty,
        commit_hash,
        hash_parts,
    )


def setup_lib_versioning():
    # First, check the repository name
    repo_name = get_repo_name()

    if repo_name and repo_name.lower().endswith("-embedded-lib"):
        (
            version_major,
            version_minor,
            version_patch,
            is_main,
            is_dirty,
            commit_hash,
            hash_parts,
        ) = detect_versioning()
    else:
        warn("Repository name mismatch. crying.")
        version_major = 0
        version_minor = 0
        version_patch = 0
        is_main = 0
        is_dirty = 0
        commit_hash = 0
        hash_parts = (0, 0)

    debug_print(
        f"Got version for astra-embedded-lib: {version_major}.{version_minor}.{version_patch}, is_main={is_main}, is_dirty={is_dirty}, hash={commit_hash}"
    )

    append_version_defines(
        [
            ("ASTRA_LIB_VERSION_MAJOR", version_major),
            ("ASTRA_LIB_VERSION_MINOR", version_minor),
            ("ASTRA_LIB_VERSION_PATCH", version_patch),
            ("ASTRA_LIB_VERSION_ISMAIN", is_main),
            ("ASTRA_LIB_VERSION_ISDIRTY", is_dirty),
            ("ASTRA_LIB_VERSION_COMMIT_HASH_LOWER", hash_parts[0]),
            ("ASTRA_LIB_VERSION_COMMIT_HASH_UPPER", hash_parts[1]),
        ]
    )


def setup_project_versioning():
    # First, check the repository name
    repo_name = get_repo_name()

    if repo_name and ("-embedded" in repo_name or repo_name.startswith("rover-")):
        (
            version_major,
            version_minor,
            version_patch,
            is_main,
            is_dirty,
            commit_hash,
            hash_parts,
        ) = detect_versioning()
    else:
        warn("Repository name mismatch. crying.")
        version_major = 0
        version_minor = 0
        version_patch = 0
        is_main = 0
        is_dirty = 0
        commit_hash = 0
        hash_parts = (0, 0)

    debug_print(
        f"Got version for project: {version_major}.{version_minor}.{version_patch}, is_main={is_main}, is_dirty={is_dirty}, hash={commit_hash}"
    )

    append_version_defines(
        [
            ("PROJECT_VERSION_MAJOR", version_major),
            ("PROJECT_VERSION_MINOR", version_minor),
            ("PROJECT_VERSION_PATCH", version_patch),
            ("PROJECT_VERSION_ISMAIN", is_main),
            ("PROJECT_VERSION_ISDIRTY", is_dirty),
            ("PROJECT_VERSION_COMMIT_HASH_LOWER", hash_parts[0]),
            ("PROJECT_VERSION_COMMIT_HASH_UPPER", hash_parts[1]),
        ]
    )


def debug_print(msg: str):
    caller = getframeinfo(stack()[1][0])  # https://stackoverflow.com/a/24439444
    if DEBUG:
        print(f"[extra_script.py:{caller.lineno}] DEBUG: {msg}", file=stderr)


def append_version_defines(defines: list[tuple[str, Any]]):
    env.Append(CPPDEFINES=defines)
    projenv.Append(CPPDEFINES=defines)


main()
