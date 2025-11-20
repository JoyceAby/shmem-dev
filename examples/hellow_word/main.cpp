/*
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This file is a part of the CANN Open Software.
 * Licensed under CANN Open Software License Agreement Version 1.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */
#include <iostream>
#include <mpi.h>
#include <unistd.h>
#include <acl/acl.h>
#include "shmem_api.h"

const char *ipport = "tcp://127.0.0.1:8998";
int f_rank = 0;
int f_npu = 0;

int main(int argc, char* argv[]) 
{
    // MPI Init
    MPI_Init(&argc, &argv);
    int rank_id, n_ranks;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank_id);
    MPI_Comm_size(MPI_COMM_WORLD, &n_ranks);

    int status = SHMEM_SUCCESS;
    aclInit(nullptr);
    aclrtSetDevice(rank_id);

    uint64_t local_mem_size = 1024UL * 1024UL * 1024;

    shmem_init_attr_t *attributes;
    status = shmem_set_attr(rank_id, n_ranks, local_mem_size, ipport, &attributes);
    status = shmem_init_attr(SHMEMX_INIT_WITH_MPI, attributes);

    status = shmem_finalize();
    if ( status != SHMEM_SUCCESS) {
        std::cout << "[ERROR] demo run failed!" << std::endl;
        std::exit(status);
    }
    aclrtResetDevice(rank_id);
    aclFinalize();
    MPI_Finalize();
    std::cout << "[SUCCESS] demo run success!" << std::endl;
}