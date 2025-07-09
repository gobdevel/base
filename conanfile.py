##
# Copyright (c) 2025 Gobind Prasad <gobdeveloper@gmail.com>
#
# This source code is licensed under the MIT license found in the
# LICENSE file in the root directory of this source tree.
#
# SPDX-License-Identifier: MIT
##

from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout, CMakeDeps


class baseRecipe(ConanFile):
    name = "base"
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
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
        "with_tests": [True, False],
        "with_benchmarks": [True, False],
        "with_examples": [True, False],
        "with_docs": [True, False],
    }
    default_options = {
        "shared": False,
        "fPIC": True,
        "with_tests": False,
        "with_benchmarks": False,
        "with_examples": False,
        "with_docs": False,
    }

    # Sources are located in the same place as this recipe, copy them to the recipe
    exports_sources = (
        "CMakeLists.txt",
        "src/*",
        "include/*",
        "tests/*",
        "benchmarks/*",
        "examples/*",
        "docs/*",
    )

    def config_options(self):
        if self.settings.os == "Windows":
            self.options.rm_safe("fPIC")

    def configure(self):
        if self.options.shared:
            self.options.rm_safe("fPIC")

    def requirements(self):
        self.requires("spdlog/1.15.3", transitive_headers=True)
        self.requires("tomlplusplus/3.4.0", transitive_headers=True)
        self.requires(
            "asio/1.34.2", transitive_headers=True
        )  # Standalone ASIO (header-only)
        self.requires("nlohmann_json/3.11.3", transitive_headers=True)

        # Add test and benchmark dependencies when needed
        if self.options.with_tests:
            self.test_requires("gtest/1.16.0")
        if self.options.with_benchmarks:
            self.test_requires("benchmark/1.9.1")

    def layout(self):
        cmake_layout(self)

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()
        tc = CMakeToolchain(self)
        tc.variables["CMAKE_EXPORT_COMPILE_COMMANDS"] = True
        tc.variables["BUILD_TESTING"] = bool(self.options.with_tests)
        tc.variables["BUILD_BENCHMARK"] = bool(self.options.with_benchmarks)
        tc.variables["BUILD_EXAMPLE"] = bool(self.options.with_examples)
        tc.variables["BUILD_DOCS"] = bool(self.options.with_docs)
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.libs = ["base"]
