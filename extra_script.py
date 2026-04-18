# Don't try to fix the Import statements. Don't try to add a shebang.
# This file won't really work with a linter... because PlatformIO.

Import("env")
Import("projenv")
global_env = DefaultEnvironment()

import subprocess
import os
from sys import stderr
from warnings import warn
from inspect import getframeinfo, stack

DEBUG = True


def debug_print(msg: str):
    caller = getframeinfo(stack()[1][0])  # https://stackoverflow.com/a/24439444
    if DEBUG:
        print(f"[extra_script.py:{caller.lineno}] DEBUG: {msg}", file=stderr)


def get_repo_name():
    repo_name = subprocess.run(
        ["git", "rev-parse", "--show-toplevel"], capture_output=True, text=True
    )
    if repo_name.returncode != 0:
        warn("Warning: Could not determine git repository path.")
        return None
    else:
        repo_path = repo_name.stdout.strip()
        repo_name_real = repo_path.split("/")[-1].split("\\")[-1]
        debug_print(f"Current Git repository: {repo_name_real}")
        return repo_name_real


def detect_versioning():
    # Get git release version
    git_release = subprocess.run(
        ["git", "describe", "--tags", "--abbrev=0"], capture_output=True, text=True
    )

    # If no release tag is found, default to 0.0.0
    if git_release.returncode != 0:
        version_major = 0
        version_minor = 0
        version_patch = 0
    else:
        version_str = git_release.stdout.strip()
        version_parts = version_str.lstrip("v").split(".")
        version_major = int(version_parts[0]) if len(version_parts) > 0 else 0
        version_minor = int(version_parts[1]) if len(version_parts) > 1 else 0
        version_patch = int(version_parts[2]) if len(version_parts) > 2 else 0

    # Set ismain to true if on main and not dirty
    git_branch = subprocess.run(
        ["git", "rev-parse", "--abbrev-ref", "HEAD"], capture_output=True, text=True
    )
    if git_branch.returncode == 0 and git_branch.stdout.strip() == "main":
        is_main = 1
    else:
        is_main = 0

    # Set isdirty to 1 if there are uncommitted changes
    git_status = subprocess.run(
        ["git", "status", "--porcelain"], capture_output=True, text=True
    )
    if git_status.returncode == 0 and git_status.stdout.strip() != "":
        is_dirty = 1
    else:
        is_dirty = 0

    # Get current short commit hash
    git_hash = subprocess.run(
        ["git", "rev-parse", "--short", "HEAD"], capture_output=True, text=True
    )
    if git_hash.returncode != 0:
        warn("Warning: Could not determine git commit hash.")
        commit_hash = 0
    else:
        commit_hash = git_hash.stdout.strip()
        debug_print(f"Current Git commit hash: {commit_hash}")
        commit_hash = int(commit_hash, 16)  # convert hex string to integer

    return (version_major, version_minor, version_patch, is_main, is_dirty, commit_hash)


def append_version_defines(defines):
    env.Append(CPPDEFINES=defines)
    projenv.Append(CPPDEFINES=defines)


def setup_lib_versioning(target=None, source=None, env=None):
    # First, check the repository name
    repo_name = get_repo_name()

    if repo_name and repo_name.lower().endswith("-embedded-lib"):
        version_major, version_minor, version_patch, is_main, is_dirty, commit_hash = (
            detect_versioning()
        )
    else:
        warn("Repository name mismatch. crying.")
        version_major = 0
        version_minor = 0
        version_patch = 0
        is_main = 0
        is_dirty = 0
        commit_hash = 0

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
            ("ASTRA_LIB_VERSION_COMMIT_HASH", commit_hash),
        ]
    )


def setup_project_versioning(target=None, source=None, env=None):
    # First, check the repository name
    repo_name = get_repo_name()

    if repo_name and "-embedded" in repo_name:
        version_major, version_minor, version_patch, is_main, is_dirty, commit_hash = (
            detect_versioning()
        )
    else:
        warn("Repository name mismatch. crying.")
        version_major = 0
        version_minor = 0
        version_patch = 0
        is_main = 0
        is_dirty = 0
        commit_hash = 0

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
            ("PROJECT_VERSION_COMMIT_HASH", commit_hash),
        ]
    )


def main():
    debug_print("DEBUGGING ENABLED: This is how you will see debug messages.")
    setup_lib_versioning()  # Called as a part of *-embedded-Lib build process
    old_wd = os.getcwd()
    os.chdir(global_env.subst("$BUILD_DIR"))
    setup_project_versioning()
    os.chdir(old_wd)


main()
