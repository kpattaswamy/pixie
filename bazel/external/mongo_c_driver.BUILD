# Copyright 2018- The Pixie Authors.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# SPDX-License-Identifier: Apache-2.0

load("@rules_foreign_cc//foreign_cc:defs.bzl", "cmake")

licenses(["notice"])

exports_files(["LICENSE"])
filegroup(
    name = "all",
    srcs = glob(["**"]),
)

cmake(
    name = "mongo_c_driver",
    cache_entries = {
        "BUILD_VERSION": "1.24.0",
    },
    includes = [
        "src/libbson/src/bson",
    ],
    #out_include_dir = "src/libbson/src",
    out_static_libs = [
        "libbson-static-1.0.a"
    ],
    build_args = [
        "--",  # <- Pass remaining options to the native tool.
        "-j`nproc`",
        "-l`nproc`",
    ],
    lib_source = ":all",
    visibility = ["//visibility:public"],
    working_directory = "",
)
