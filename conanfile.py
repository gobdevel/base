##
# Copyright (c) 2025 Gobind Prasad <gobdeveloper@gmail.com>
#
# This source code is licensed under the MIT license found in the
# LICENSE file in the root directory of this source tree.
#
# SPDX-License-Identifier: MIT
##

import os
from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout, CMakeDeps


class cruxRecipe(ConanFile):
    name = "crux"
    version = "1.0"
    package_type = "library"

    # Optional metadata
    license = "MIT"
    author = "Gobind Prasad gobdeveloper@gmail.com"
    url = "<Package recipe repository url here, for issues about the package>"
    description = "A C++ library for daemonizing microservices with a focus on infrastructure management."
    topics = ("c++", "infra", "daemon", "daemonize", "microservice")

    # Binary configuration
    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False], "fPIC": [True, False]}
    default_options = {"shared": False, "fPIC": True}

    # Sources are located in the same place as this recipe, copy them to the recipe
    exports_sources = "CMakeLists.txt", "src/*", "include/*", "tests/*"

    def config_options(self):
        if self.settings.os == "Windows":
            self.options.rm_safe("fPIC")

    def configure(self):
        if self.options.shared:
            self.options.rm_safe("fPIC")

    def requirements(self):
        self.requires("spdlog/1.15.3", transitive_headers=True)
        self.test_requires("gtest/1.16.0")

    def layout(self):
        cmake_layout(self)

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()
        tc = CMakeToolchain(self)
        tc.variables["CMAKE_EXPORT_COMPILE_COMMANDS"] = True
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

        if not self.conf.get("tools.build:skip_test", default=False):
            test_folder = os.path.join("tests")
            self.run(os.path.join(test_folder, "test_crux"))

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.libs = ["crux"]
