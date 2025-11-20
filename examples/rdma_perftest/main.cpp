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
#include <cstdlib>
#include <string>
#include <vector>
#include <sys/file.h>
#include <stdio.h>
#include <string.h>
#include <mpi.h>

#include "acl/acl.h"
#include "shmem_api.h"
#include "shmemi_host_common.h"

int g_npus = 8;
const char *ipport;
int f_rank = 0;
int f_npu = 0;
const char *test_type;

extern void rdma_highlevel_put_pingpong_latency_do(uint32_t block_dim, void* stream, uint64_t fftsConfig, uint8_t* gva, int message_length);
extern void rdma_postsend_cost_do(uint32_t block_dim, void* stream, uint64_t fftsConfig, uint8_t* gva, int message_length);
extern void rdma_highlevel_put_bw_do(uint32_t block_dim, void* stream, uint64_t fftsConfig, uint8_t* gva, int message_length);
extern void rdma_mte_put_bw_do(uint32_t block_dim, void* stream, uint64_t fftsConfig, uint8_t* gva, int message_length, int64_t iter);

int test_shmem_rdma_highlevel_put_pingpong_latency(int rank_id, int n_ranks, uint64_t local_mem_size, int message_length)
{
    uint32_t iteration = 1;
    int32_t device_id = rank_id % g_npus + f_npu;
    int status = 0;
    aclrtStream stream = nullptr;

    status = aclInit(nullptr);
    status = aclrtSetDevice(device_id);
    status = aclrtCreateStream(&stream);

    shmem_init_attr_t *attributes;
    status = shmem_set_attr(rank_id, n_ranks, local_mem_size, ipport, &attributes);
    status = shmem_init_attr(SHMEMX_INIT_WITH_MPI, attributes);

    uint64_t fftsConfig = shmemx_get_ffts_config();
    uint8_t* gva = (uint8_t*)shmem_malloc(1024 * 1024 * 6);

    int64_t *xHost;
    size_t totalSize = message_length * n_ranks;

    aclrtMallocHost((void **)(&xHost), totalSize);
    for (uint32_t i = 0; i < message_length / sizeof(int64_t); i++) {
        xHost[i] = rank_id + 10;
    }
    aclrtMemcpy(gva + rank_id * message_length, message_length, xHost, message_length, ACL_MEMCPY_HOST_TO_DEVICE);
    aclrtMemcpy(gva + n_ranks * message_length + 32 * (rank_id + 1), 32, xHost, 32, ACL_MEMCPY_HOST_TO_DEVICE);

    for (uint32_t i = 0; i < iteration; i++) {
        rdma_highlevel_put_pingpong_latency_do(1, stream, fftsConfig, gva, message_length);
    }
    aclrtSynchronizeStream(stream);
    if (rank_id == 0) {
        aclrtMemcpy(xHost, sizeof(int64_t), gva + message_length * n_ranks, sizeof(int64_t), ACL_MEMCPY_DEVICE_TO_HOST);
        std::cout << "RDMA highlevel put pingpong latency test. Message length = " << message_length << " Byte; latency = " << xHost[0] / 50.0 << " us." << std::endl;
    }

    aclrtFreeHost(xHost);
    shmem_finalize();
    aclrtDestroyStream(stream);
    aclrtResetDevice(device_id);
    aclFinalize();
    return 0;
}

int test_shmem_rdma_postsend_cost(int rank_id, int n_ranks, uint64_t local_mem_size, int message_length)
{
    uint32_t iteration = 1;
    int32_t device_id = rank_id % g_npus + f_npu;
    int status = 0;
    aclrtStream stream = nullptr;

    status = aclInit(nullptr);
    status = aclrtSetDevice(device_id);
    status = aclrtCreateStream(&stream);

    shmem_init_attr_t *attributes;
    status = shmem_set_attr(rank_id, n_ranks, local_mem_size, ipport, &attributes);
    status = shmem_init_attr(SHMEMX_INIT_WITH_MPI, attributes);

    uint64_t fftsConfig = shmemx_get_ffts_config();
    uint8_t* gva = (uint8_t*)shmem_malloc(1024 * 1024 * 6);

    int64_t *xHost;
    size_t totalSize = message_length * n_ranks;

    aclrtMallocHost((void **)(&xHost), totalSize);
    for (uint32_t i = 0; i < message_length / sizeof(int64_t); i++) {
        xHost[i] = rank_id + 10;
    }
    aclrtMemcpy(gva + rank_id * message_length, message_length, xHost, message_length, ACL_MEMCPY_HOST_TO_DEVICE);

    for (uint32_t i = 0; i < iteration; i++) {
        rdma_postsend_cost_do(1, stream, fftsConfig, gva, message_length);
    }
    aclrtSynchronizeStream(stream);
    if (rank_id == 0) {
        aclrtMemcpy(xHost, sizeof(int64_t), gva + message_length * n_ranks, sizeof(int64_t), ACL_MEMCPY_DEVICE_TO_HOST);
        std::cout << "RDMA postsend cost test. Message length = " << message_length << " Byte; postsend cost = " << xHost[0] / (50.0 * 500) << " us." << std::endl;
    }

    aclrtFreeHost(xHost);
    shmem_finalize();
    aclrtDestroyStream(stream);
    aclrtResetDevice(device_id);
    aclFinalize();
    return 0;
}

int test_shmem_rdma_highlevel_put_bw(int rank_id, int n_ranks, uint64_t local_mem_size, int message_length)
{
    int32_t device_id = rank_id % g_npus + f_npu;
    int status = 0;
    aclrtStream stream = nullptr;

    status = aclInit(nullptr);
    status = aclrtSetDevice(device_id);
    status = aclrtCreateStream(&stream);

    shmem_init_attr_t *attributes;
    status = shmem_set_attr(rank_id, n_ranks, local_mem_size, ipport, &attributes);
    status = shmem_init_attr(SHMEMX_INIT_WITH_MPI, attributes);

    uint64_t fftsConfig = shmemx_get_ffts_config();
    uint8_t* gva = (uint8_t*)shmem_malloc(1024 * 1024 * 6);

    int64_t *xHost;
    size_t totalSize = message_length * n_ranks;

    aclrtMallocHost((void **)(&xHost), totalSize);
    for (uint32_t i = 0; i < message_length / sizeof(int64_t); i++) {
        xHost[i] = rank_id + 10;
    }
    aclrtMemcpy(gva + rank_id * message_length, message_length, xHost, message_length, ACL_MEMCPY_HOST_TO_DEVICE);

    rdma_highlevel_put_bw_do(1, stream, fftsConfig, gva, message_length);
    aclrtSynchronizeStream(stream);
    if (rank_id == 0) {
        aclrtMemcpy(xHost, sizeof(int64_t), gva + message_length * n_ranks, sizeof(int64_t), ACL_MEMCPY_DEVICE_TO_HOST);
        std::cout << "RDMA high level put bandwidth test. Message length = " << message_length << " Byte; time = " << xHost[0] / (50.0) << " us." << std::endl;
    }

    aclrtFreeHost(xHost);
    shmem_finalize();
    aclrtDestroyStream(stream);
    aclrtResetDevice(device_id);
    aclFinalize();
    return 0;
}

int test_shmem_rdma_mte_put_bw(int rank_id, int n_ranks, uint64_t local_mem_size, int message_length)
{
    int32_t device_id = rank_id % g_npus + f_npu;
    int status = 0;
    aclrtStream stream = nullptr;

    status = aclInit(nullptr);
    status = aclrtSetDevice(device_id);
    status = aclrtCreateStream(&stream);

    shmem_init_attr_t *attributes;
    status = shmem_set_attr(rank_id, n_ranks, local_mem_size, ipport, &attributes);
    status = shmem_init_attr(SHMEMX_INIT_WITH_MPI, attributes);
    shmem_mte_set_ub_params(0, 128 * 1024, 0);

    uint64_t fftsConfig = shmemx_get_ffts_config();
    uint8_t* gva = (uint8_t*)shmem_malloc(1024 * 1024 * 32);
    int64_t *inHost;
    int64_t *outHost;
    size_t totalSize = message_length * n_ranks * 3;
    aclrtMallocHost((void **)(&inHost), totalSize);
    aclrtMallocHost((void **)(&outHost), totalSize);
    memset(inHost, 0, totalSize);
    double rdmaTotalTime = 0.0;
    double mteTotalTime = 0.0;

    for (int iter = 0; iter < 20; iter++) {
        for (uint32_t i = 0; i < message_length / sizeof(int64_t); i++) {
            inHost[i + rank_id * message_length / sizeof(int64_t)] = rank_id + 10 + iter;
        }
        for (uint32_t i = 0; i < message_length / sizeof(int64_t); i++) {
            inHost[i + (rank_id + n_ranks) * message_length / sizeof(int64_t)] = rank_id + 10 + iter;
        }
        aclrtMemcpy(gva, totalSize, inHost, totalSize, ACL_MEMCPY_HOST_TO_DEVICE);
        shmemi_control_barrier_all();
        rdma_mte_put_bw_do(1, stream, fftsConfig, gva, message_length, iter);
        aclrtSynchronizeStream(stream);
        if (rank_id == 0 && iter >= 10) {
            aclrtMemcpy(outHost, 64, gva + message_length * n_ranks * 2, 64, ACL_MEMCPY_DEVICE_TO_HOST);
            rdmaTotalTime += outHost[0] / 50.0;
            mteTotalTime += outHost[6] / 50.0;
        }
    }
    if (rank_id == 0) {
        std::cout << "RDMA rdma mte test. Message length = " << message_length << " Byte; average RDMA time = " << rdmaTotalTime / 10.0 << " us." << std::endl;
        std::cout << "RDMA rdma mte test. Message length = " << message_length << " Byte; average MTE time = " << mteTotalTime / 10.0 << " us." << std::endl;
    }

    aclrtFreeHost(inHost);
    aclrtFreeHost(outHost);
    shmem_finalize();
    aclrtDestroyStream(stream);
    aclrtResetDevice(device_id);
    aclFinalize();
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc != 7) {
        std::cout << "[ERROR] Paramater number mismatch." << std::endl;
        std::cout << "[USAGE] ./rdma_perftest <n_ranks> <rank_id> <ipport> <g_npus> <f_rank> <f_npu> <test_type> <msg_len>. See README for more details." << std::endl;
    }
    int status = 0;
    MPI_Init(&argc, &argv);

    // 获取当前进程的编号（rank）
    int n_ranks;
    MPI_Comm_size(MPI_COMM_WORLD, &n_ranks);
    int rank_id;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank_id);
    if (n_ranks != 2) {
        std::cout << "[ERROR] Error number of ranks! Only support 2 ranks!" << std::endl;
        return -1;
    }
    if (rank_id >= 2) {
        std::cout << "[ERROR] Error rank ID! Only support 2 ranks!" << std::endl;
        return -1;
    }
    ipport = argv[1];
    g_npus = atoi(argv[2]);
    f_rank = atoi(argv[3]);
    f_npu = atoi(argv[4]);
    test_type = argv[5];
    int msg_len = atoi(argv[6]);
    uint64_t local_mem_size = 1024UL * 1024UL * 64;
    if (std::string(test_type) == "highlevel_put_pingpong_latency") {
        test_shmem_rdma_highlevel_put_pingpong_latency(rank_id, n_ranks, local_mem_size, msg_len);
    } else if (std::string(test_type) == "postsend_cost") {
        test_shmem_rdma_postsend_cost(rank_id, n_ranks, local_mem_size, msg_len);
    } else if (std::string(test_type) == "highlevel_put_bw") {
        test_shmem_rdma_highlevel_put_bw(rank_id, n_ranks, local_mem_size, msg_len);
    } else if (std::string(test_type) == "rdma_mte_bw") {
        test_shmem_rdma_mte_put_bw(rank_id, n_ranks, local_mem_size, msg_len);
    }

    std::cout << "[SUCCESS] demo run success in rank " << rank_id << std::endl;
    MPI_Finalize();

    return 0;
}