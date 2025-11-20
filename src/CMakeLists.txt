# Copyright (c) 2025 Huawei Technologies Co., Ltd.
# This file is a part of the CANN Open Software.
# Licensed under CANN Open Software License Agreement Version 1.0 (the "License").
# Please refer to the License for details. You may not use this file except in compliance with the License.
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
# INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
# See LICENSE in the root of the software repository for the full text of the License.

if (BUILD_PYTHON STREQUAL "ON")
    add_subdirectory(host/python_wrapper)
endif ()

set(SHMEM_MPI_SUPPORT OFF)

if(SHMEM_MPI_SUPPORT)
  find_package(MPI REQUIRED)
else()
  find_package(MPI)
  if(MPI_FOUND)
        set(SHMEM_MPI_SUPPORT ON)
  endif()
endif()

file(GLOB_RECURSE SHMEM_KERNEL_FILES ${CMAKE_CURRENT_SOURCE_DIR}/device/*.cpp)
add_library(shmem_device OBJECT ${SHMEM_KERNEL_FILES})
target_compile_options(shmem_device PRIVATE ${CMAKE_CCE_COMPILE_OPTIONS} --cce-aicore-arch=dav-c220)
target_include_directories(shmem_device
        PUBLIC
        ${PROJECT_SOURCE_DIR}/include/
)

file(GLOB_RECURSE SHMEM_HOST_FILES ${CMAKE_CURRENT_SOURCE_DIR}/host/*.cpp)
list(FILTER SHMEM_HOST_FILES EXCLUDE REGEX "python_wrapper")
list(FILTER SHMEM_HOST_FILES EXCLUDE REGEX "modules")

add_library(shmem_host OBJECT ${SHMEM_HOST_FILES})
target_compile_options(shmem_host PRIVATE ${CMAKE_CPP_COMPILE_OPTIONS})
target_include_directories(shmem_host
        PUBLIC
        ${PROJECT_SOURCE_DIR}/include/
        ${PROJECT_SOURCE_DIR}/src/host
        ${PROJECT_SOURCE_DIR}/src/device
)

add_library(shmem SHARED $<TARGET_OBJECTS:shmem_device> $<TARGET_OBJECTS:shmem_host>)
target_link_options(shmem PRIVATE --cce-fatobj-link)

set(SHMEM_MTE_SUPPORT ON)
if(SHMEM_MTE_SUPPORT)
        add_library(shmem_transport_mte SHARED)

        target_sources(shmem_transport_mte PRIVATE 
                modules/transport/shmemi_mte.cpp)
        
        target_include_directories(shmem_transport_mte PRIVATE
                ${PROJECT_SOURCE_DIR}/include
                ${PROJECT_SOURCE_DIR}/src/host)
        
        set_target_properties(shmem_transport_mte PROPERTIES PREFIX "")

        install(TARGETS shmem_transport_mte
                LIBRARY DESTINATION lib)
endif()

# MPI
if(SHMEM_MPI_SUPPORT)
    separate_arguments(SHMEM_CXX_LINK_FLAGS NATIVE_COMMAND "${MPI_CXX_LINK_FLAGS}")
    target_link_options(shmem INTERFACE ${SHMEM_CXX_LINK_FLAGS})
    target_compile_definitions(shmem INTERFACE ${MPI_CXX_COMPILE_DEFINITIONS})
    target_compile_options(shmem INTERFACE ${MPI_CXX_COMPILE_OPTIONS})
    
    add_library(
    shmem_bootstrap_mpi SHARED
    )
    target_sources(shmem_bootstrap_mpi PRIVATE modules/bootstrap/shmemi_bootstrap_mpi.cpp)
    target_link_libraries(shmem_bootstrap_mpi PRIVATE MPI::MPI_CXX)
    target_include_directories(shmem_bootstrap_mpi
                                PRIVATE
                                ${PROJECT_SOURCE_DIR}/include
                                ${PROJECT_SOURCE_DIR}/src/host
    )
    set_target_properties(shmem_bootstrap_mpi PROPERTIES PREFIX "")
    install(TARGETS shmem_bootstrap_mpi
        LIBRARY DESTINATION lib
    )

endif()

set(SHMEM_RDMA_SUPPORT ON)
if(SHMEM_RDMA_SUPPORT)
    add_library(
        shmem_transport_rdma SHARED
    )
    target_sources(shmem_transport_rdma PRIVATE 
        modules/transport/shmemi_rdma.cpp
        modules/transport/rdma/device_qp_manager.cpp
        modules/transport/rdma/dl_hccp_api.cpp
    )
    target_link_libraries(shmem_transport_rdma PRIVATE MPI::MPI_CXX)
    target_include_directories(shmem_transport_rdma
                                PRIVATE
                                ${PROJECT_SOURCE_DIR}/include
                                ${PROJECT_SOURCE_DIR}/src/host
                                ${PROJECT_SOURCE_DIR}/src/modules
    )
    set_target_properties(shmem_transport_rdma PROPERTIES PREFIX "")
    install(TARGETS shmem_transport_rdma
        LIBRARY DESTINATION lib
    )
endif()

# 安装配置
install(TARGETS shmem
        LIBRARY DESTINATION lib
        PUBLIC_HEADER DESTINATION include
)

install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/
        DESTINATION include
        FILES_MATCHING PATTERN "*.h"
)
