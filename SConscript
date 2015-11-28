"""
SConscript

This file is used by SCons as a blueprint for how to build this
component as a linux kernel module.
"""

import os

# Import the default build environments for all supported build platforms.
Import("platformEnvironments")

# List all supported platforms here.
# For each one, list any platform-specific dependencies.
platforms = [
    "arm-linux",
]

# Delegate to GNU Make to build kernel module.
# Only build something for the Release variant.
products = {}
for platform in platforms:
    if platform not in platformEnvironments:
        continue
    env = platformEnvironments[platform].Clone()
    if env["VARIANT"] == "Release":
        kernelVersion = os.popen("uname -r").read().strip()
        path = Dir(".").srcnode().abspath
        name = os.path.basename(path)
        object = "%s.ko" % (name)
        target = "clean" if env.GetOption("clean") else "modules"
        env.Execute("make -C /lib/modules/%s/build M=%s %s" % (kernelVersion, path, target))
        deployedModule = env.Deploy("#${BUILD}bin", object)
        products[platform] = {
            "module": deployedModule
        }
Return("products")
