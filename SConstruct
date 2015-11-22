"""
SConstruct

This file is used by SCons as a blueprint for how to build the
projects in this workspace.

Copyright (c) 2015 by Richard Walters
"""

import os
import sys

# This is the list of projects in the workspace, in order from least to most
# dependent.  Projects at the same level of dependency can be listed
# in any order.
components = [
    # Level 0: No dependencies
    "PiSlicer",

    # Level 1: Depends on:
    #     PiSlicer
    "HelloWorld",
]

# These are used to make completely separate builds of the same projects
# across all platforms.
variants = [
    "Debug",
    "Release",
]

# These are the different architectures or operating systems for which
# to build the projects.
#
# A dictionary is defined for each platform that provides values to set
# for new environment variables or to add to existing environment variables
# for all variants of all projects for that platform.
platforms = {
    "arm-linux": {
        "SOURCE_DIRECTORIES": [
            "linux",
        ],
        "CCFLAGS": [
            "-fPIC",
        ],
        "LINKFLAGS": [
        ],
        "RPATH": "\\$$$$ORIGIN",
    },
}

# These are additional values to set for new environment variables or to
# add to existing environment variables for different combinations of
# platform and variant.
#
# The outer dictionary entries are for different platforms, while the middle
# dictionary entries are for different variants of a platform, and the inner
# dictionary entries are for environment variables of a platform-variant
# combination.
configurations = {
    "arm-linux": {
        "Debug": {
            "CCFLAGS": [
                "-O0",
                "-g3",
            ],
            "CPPDEFINES": [
                "DEBUG",
            ],
        },
        "Release": {
            "CCFLAGS": [
                "-O3",
                "-fomit-frame-pointer",
            ],
            "CPPDEFINES": [
                "NDEBUG",
            ],
        },
    },
}

# This is the default build environment from which all build environments
# in this workspace are derived.
root = Environment(
    # Toolchain selection
    tools = ["as", "gcc", "g++", "gnulink", "ar"],

    # Toolchain defaults
    ASFLAGS = [
        "-x", "assembler-with-cpp",
        "-c",
    ],
    CFLAGS = [
        "-std=c1x",
    ],
    CCFLAGS = [
        "-Wall",
    ],
    CXXFLAGS = [
        "-std=c++0x",
        "-fno-exceptions",
    ],
    CPPDEFINES = [
        "__STDC_FORMAT_MACROS",
    ],
    CPPPATH = [
        "#",
        ".",
    ],
    LINKFLAGS = [
    ],
    LIBPATH = [
    ],
    LIBS = [
        "c",
        "stdc++",
    ],

    # Custom variable used to direct where build products go
    BUILD = "Build/${VARIANT}/${PLATFORM}/",
)

# Abbreviate command strings of tools if not told to be verbose.
if ARGUMENTS.get("VERBOSE") != "1":
    root["ARCOMSTR"] = "[AR] $TARGET"
    root["ASCOMSTR"] = "[AS($PLATFORM:$VARIANT)] $SOURCE"
    root["CCCOMSTR"] = "[CC($PLATFORM:$VARIANT)] $SOURCE"
    root["SHCCCOMSTR"] = "[CC($PLATFORM:$VARIANT)] $SOURCE"
    root["CXXCOMSTR"] = "[CXX($PLATFORM:$VARIANT)] $SOURCE"
    root["SHCXXCOMSTR"] = "[CXX($PLATFORM:$VARIANT)] $SOURCE"
    root["LINKCOMSTR"] = "[LINK] $TARGET"
    root["SHLINKCOMSTR"] = "[LINK] $TARGET"
    root["LDMODULECOMSTR"] = "[LD] $TARGET"
    root["RANLIBCOMSTR"] = "[RANLIB] $TARGET"
    Copy.strfunc = lambda destination, source: "[CP] %s -> %s" % (source, destination)

# This is a helper function which constructs a list of build nodes
# representing static-link objects compiled from a given set of sources.
def StaticObjects(env, source, *args, **kargs):
    return [
        env.Object(
            "#${BUILD}obj/${COMPONENT}/" + os.path.splitext(os.path.basename(str(node)))[0],
            node,
            *args,
            **kargs
        ) for node in Flatten(source)
    ]
root.AddMethod(StaticObjects, "StaticObjects")

# This is a helper function which constructs a list of build nodes
# representing shared objects compiled from a given set of sources.
def SharedObjects(env, source, *args, **kargs):
    return [
        env.SharedObject(
            "#${BUILD}obj/${COMPONENT}/" + os.path.splitext(os.path.basename(str(node)))[0],
            node,
            *args,
            **kargs
        ) for node in Flatten(source)
    ]
root.AddMethod(SharedObjects, "SharedObjects")

# This is a helper function which constructs a list of build nodes
# which will make copies of a given set of sources in a given
# destination folder.
def Deploy(env, destination, source):
    return [
        env.Command(
            os.path.join(
                destination,
                os.path.basename(str(node))
            ),
            node,
            Copy("${TARGET}", "${SOURCE}")
        ) for node in Flatten(source)
    ]
root.AddMethod(Deploy, "Deploy")

# This is a helper function which constructs a list of the base names
# (file name without adornments such as file extension or "lib" prefix)
# of a given set of sources.
def Names(env, source):
    return [os.path.basename(str(node)) for node in Flatten(source)]
root.AddMethod(Names, "Names")

# This is a helper function which constructs a list of the directories
# containing a given set of sources.
def Directories(env, source):
    return [os.path.dirname(str(node)) for node in Flatten(source)]
root.AddMethod(Directories, "Directories")

# This is a helper function which configures a build environment to build
# a project for a specific platform and returns a list of build nodes
# representing the source files to compile for the project.
def CommonProject(env, projectName, platformName, platform, **kargs):
    env["COMPONENT"] = projectName
    localLibraryDependencies = kargs.get("localLibraryDependencies", {})
    systemLibraryDependencies = kargs.get("systemLibraryDependencies", [])
    platformLocalLibraryDependencies = []
    for libraryType in localLibraryDependencies:
        for localLibraryDependency in localLibraryDependencies[libraryType]:
            platformLocalLibraryDependencies.append(localLibraryDependency[platformName][libraryType])
    platformLocalLibraryDependencies.extend(platform.get("localLibraryDependencies", []))
    platformSystemLibraryDependencies = systemLibraryDependencies + platform.get("systemLibraryDependencies", [])
    env.Append(LIBPATH = env.Directories(platformLocalLibraryDependencies))
    env.Prepend(LIBS = env.Names(platformSystemLibraryDependencies))
    env.Prepend(LIBS = env.Names(platformLocalLibraryDependencies))
    return [
        Glob("*.c"),
        Glob("*.cpp"),
        [Glob("%s%s*.c" % (platformName, os.path.sep)) for platformName in env.get("SOURCE_DIRECTORIES", [])],
        [Glob("%s%s*.cpp" % (platformName, os.path.sep)) for platformName in env.get("SOURCE_DIRECTORIES", [])],
    ]
root.AddMethod(CommonProject, "CommonProject")

# This is a helper function which configures a build environment to build
# a library project for a specific platform and returns a dictionary
# of build nodes representing what is built for the project.
def LibraryProject(env, projectName, platformName, platform, **kargs):
    inputs = env.CommonProject(projectName, platformName, platform, **kargs)
    platformStaticObjectDependencies = kargs.get("staticObjectDependencies", []) + platform.get("staticObjectDependencies", [])
    platformSharedObjectDependencies = kargs.get("sharedObjectDependencies", []) + platform.get("sharedObjectDependencies", [])
    staticLibrary = env.Library("#${BUILD}lib/${COMPONENT}", env.StaticObjects(inputs), platformStaticObjectDependencies)
    sharedLibrary = env.SharedLibrary("#${BUILD}bin/${COMPONENT}", env.SharedObjects(inputs), platformSharedObjectDependencies)
    return {
        "staticLibrary": staticLibrary,
        "sharedLibrary": sharedLibrary,
    }
root.AddMethod(LibraryProject, "LibraryProject")

# This is a helper function which configures a build environment to build
# a program project for a specific platform and returns a dictionary
# of build nodes representing what is built for the project.
def ProgramProject(env, projectName, platformName, platform, **kargs):
    inputs = env.CommonProject(projectName, platformName, platform, **kargs)
    platformStaticObjectDependencies = kargs.get("staticObjectDependencies", []) + platform.get("staticObjectDependencies", [])
    platformObjectDependencies = kargs.get("staticObjectDependencies", []) + platform.get("staticObjectDependencies", [])
    program = env.Program("#${BUILD}bin/${COMPONENT}", env.StaticObjects(inputs), platformStaticObjectDependencies)
    return {
        "program": program,
    }
root.AddMethod(ProgramProject, "ProgramProject")

# For every desired variant (specified as a comma-separated list of variant
# names in the VARIANTS environment variable, or by default all supported
# variants) set up a platform build environment and build all projects
# in the workspace.
for variant in ARGUMENTS.get("VARIANTS", ",".join(variants)).split(","):
    # Make sure the variant is specified and is supported.
    if variant == "":
        continue
    if variant not in variants:
        print >>sys.stderr, "error: VARIANT '%s' not supported" % (variant)
        Exit(2)

    # Construct a build environment for every desired platform (specified
    # as a comma-separated list of platform names in the PLATFORMS environment
    # variable, or by default all supported platforms).
    env = root.Clone(
        VARIANT = variant,
    )
    platformEnvironments = {}
    for platform in ARGUMENTS.get("PLATFORMS", ",".join(platforms.keys())).split(","):
        # Make sure the platform is specified and is supported.
        if platform == "":
            continue
        if platform not in platforms:
            print >>sys.stderr, "error: PLATFORM '%s' not supported" % (platform)
            Exit(2)

        # Construct variant-specific build environment.
        platformEnvironment = env.Clone()
        for key in platforms[platform]:
            value = platforms[platform][key]
            if isinstance(value, list):
                if key not in platformEnvironment:
                    platformEnvironment[key] = []
                platformEnvironment[key].extend(value)
            else:
                platformEnvironment[key] = value
        if platform in configurations:
            if variant in configurations[platform]:
                for key in configurations[platform][variant]:
                    value = configurations[platform][variant][key]
                    if isinstance(value, list):
                        if key not in platformEnvironment:
                            platformEnvironment[key] = []
                        platformEnvironment[key].extend(value)
                    else:
                        platformEnvironment[key] = value
        platformEnvironment["PLATFORM"] = platform
        platformEnvironments[platform] = platformEnvironment
    Export("platformEnvironments")

    # Build components in the order in which they are specified.
    # After building a component, export it to subsequent components
    # as they may depend on it.
    for component in components:
        products = SConscript("%s/SConscript" % (component))
        Export({component: products})
