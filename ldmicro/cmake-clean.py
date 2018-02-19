#!/usr/bin/env python

"""
Clean cmake generated files.
"""

import argparse
import os
import shutil
import subprocess
import sys

# Do not cleanup anything in these subdirectories.
PRUNE_DIRS = [".svn", ".git", "CVS"]


def make_clean(directory):
    """Run 'make clean' in directory.

    Arguments:
    - `directory`: target directory
    """

    args = [
        "make",
        "--directory=%s" % directory,
        "--quiet",
        "clean"
    ]
    process = subprocess.Popen(args)
    return process.wait()


def clean(directory):
    """Clean cmake files.

    Arguments:
    - `directory`: target directory
    """

    # Toplevel files.
    for filename in [
        "CMakeCache.txt",
        "CPackConfig.cmake",
        "CPackSourceConfig.cmake",
        "install_manifest.txt"
    ]:
        pathname = os.path.join(directory, filename)
        if os.path.exists(pathname):
            os.remove(pathname)

    # Toplevel directories.
    for dirname in ["_CPack_Packages"]:
        pathname = os.path.join(directory, dirname)
        if os.path.exists(pathname):
            shutil.rmtree(pathname)

    # CMakeFiles, cmake_install.cmake.
    for dirpath, dirnames, filenames in os.walk(directory):
        # Prune subdirs.
        for dirname in dirnames:
            if dirname in PRUNE_DIRS:
                dirnames.remove(dirname)

        if "CMakeFiles" in dirnames:
            for filename in ["cmake_install.cmake"]:
                if filename in filenames:
                    pathname = os.path.join(dirpath, filename)
                    if os.path.exists(pathname):
                        os.remove(pathname)
            shutil.rmtree(os.path.join(dirpath, "CMakeFiles"))
            dirnames.remove("CMakeFiles")

    # Remove empty directories.  The "repeat" construct is needed
    # because the dirnames list for the parent is generated before the
    # parent is processed.  When a directory is removed, there is no
    # way to remove it from the parent's dirnames list.  Note that
    # setting topdown=False will not help here, and it complicates the
    # pruning logic.
    repeat = True
    while repeat:
        repeat = False
        for dirpath, dirnames, filenames in os.walk(directory):
            # We must check for emptiness before pruning.  Otherwise
            # we could try to remove a directory that contains only
            # prunable subdirs.
            if len(dirnames) == 0 and len(filenames) == 0:
                os.rmdir(dirpath)
                repeat = True

            # Prune subdirs.
            for dirname in dirnames:
                if dirname in PRUNE_DIRS:
                    dirnames.remove(dirname)


def main():
    """main"""
    arg_parser = argparse.ArgumentParser(
        description="Clean cmake generated files."
    )
    arg_parser.add_argument(
        "dirs",
        metavar="dir",
        nargs="*",
        help="cmake directory"
    )
    args = arg_parser.parse_args()
    if len(args.dirs) == 0:
        args.dirs.append(".")

    for arg in args.dirs:
        make_clean(arg)
        clean(arg)

    return 0


if __name__ == "__main__":
    sys.exit(main())
