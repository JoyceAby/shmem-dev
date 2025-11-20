# Copyright (c) 2025 Huawei Technologies Co., Ltd.
# This file is a part of the CANN Open Software.
# Licensed under CANN Open Software License Agreement Version 1.0 (the "License").
# Please refer to the License for details. You may not use this file except in compliance with the License.
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
# INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
# See LICENSE in the root of the software repository for the full text of the License.

set(CMAKE_C_COMPILER gcc)
set(CMAKE_CXX_COMPILER g++)

add_subdirectory(host)
add_subdirectory(device)

add_executable(shmem_fuzztest $<TARGET_OBJECTS:shmem_fuzztest_host>)
target_link_directories(shmem_fuzztest PRIVATE
    ${PROJECT_SOURCE_DIR}/install/memfabric_hybrid/lib
    ${PROJECT_SOURCE_DIR}/3rdparty/googletest/lib
    ${PROJECT_SOURCE_DIR}/3rdparty/secodefuzz/lib
)
target_link_libraries(shmem_fuzztest PRIVATE shmem_fuzztest_device mf_smem gtest gcov Secodefuzz)
