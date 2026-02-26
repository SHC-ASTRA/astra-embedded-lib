# Don't try to fix the Import statements. Don't try to add a shebang.
# This file won't really work with a linter... because PlatformIO.

Import("env")
Import("projenv")

import subprocess
from warnings import warn


def get_repo_name():
    warn("PLEASE IGNORE THESE WARNINGS! They are not actually warnings. This text just won't show up with print(). Ask David ._.")
    repo_name = subprocess.run(["git", "rev-parse", "--show-toplevel"], capture_output=True, text=True)
    if repo_name.returncode != 0:
        warn("Warning: Could not determine git repository path.")
        return None
    else:
        repo_path = repo_name.stdout.strip()
        repo_name_real = repo_path.split("/")[-1].split("\\")[-1]
        warn(f"Current Git repository: {repo_name_real}")
        return repo_name_real

def detect_versioning():
    # Get git release version
    git_release = subprocess.run(["git", "describe", "--tags", "--abbrev=0"], capture_output=True, text=True)

    # If no release tag is found, default to 0.0.0
    if git_release.returncode != 0:
        version_major = 0
        version_minor = 0
        version_patch = 0
    else:
        version_str = git_release.stdout.strip()
        version_parts = version_str.lstrip('v').split('.')
        version_major = int(version_parts[0]) if len(version_parts) > 0 else 0
        version_minor = int(version_parts[1]) if len(version_parts) > 1 else 0
        version_patch = int(version_parts[2]) if len(version_parts) > 2 else 0

    # Set ismain to true if on main and not dirty
    git_branch = subprocess.run(["git", "rev-parse", "--abbrev-ref", "HEAD"], capture_output=True, text=True)
    git_status = subprocess.run(["git", "status", "--porcelain"], capture_output=True, text=True)
    is_main = 0
    if git_branch.returncode == 0 and git_status.returncode == 0:
        branch_name = git_branch.stdout.strip()
        if branch_name == "main" and git_status.stdout.strip() == "":
            is_main = 1

    return (version_major, version_minor, version_patch, is_main)


def setup_lib_versioning(target=None, source=None, env=None):
    # First, check the repository name
    repo_name = get_repo_name()

    if repo_name and repo_name.endswith("-Embedded-Lib"):
        version_major, version_minor, version_patch, is_main = detect_versioning()
    else:
        warn("Repository name mismatch. crying.")
        version_major = 0
        version_minor = 0
        version_patch = 0
        is_main = 0

    warn(f"Got version for astra-embedded-lib: {version_major}.{version_minor}.{version_patch}, is_main={is_main}")
    projenv.Append(CPPDEFINES=[
        ("ASTRA_LIB_VERSION_MAJOR", version_major),
        ("ASTRA_LIB_VERSION_MINOR", version_minor),
        ("ASTRA_LIB_VERSION_PATCH", version_patch),
        ("ASTRA_LIB_VERSION_ISMAIN", is_main)
    ])

def setup_project_versioning(target=None, source=None, env=None):
    # First, check the repository name
    repo_name = get_repo_name()

    if repo_name and "-embedded" in repo_name:
        version_major, version_minor, version_patch, is_main = detect_versioning()
    else:
        warn("Repository name mismatch. crying.")
        version_major = 0
        version_minor = 0
        version_patch = 0
        is_main = 0

    warn(f"Got version for project: {version_major}.{version_minor}.{version_patch}, is_main={is_main}")
    projenv.Append(CPPDEFINES=[
        ("PROJECT_VERSION_MAJOR", version_major),
        ("PROJECT_VERSION_MINOR", version_minor),
        ("PROJECT_VERSION_PATCH", version_patch),
        ("PROJECT_VERSION_ISMAIN", is_main)
    ])


DefaultEnvironment().AddPreAction("$BUILD_DIR/src/main.cpp.o", setup_project_versioning)  # Called as a part of *-embedded project build process
setup_lib_versioning()  # Called as a part of *-embedded-Lib build process
