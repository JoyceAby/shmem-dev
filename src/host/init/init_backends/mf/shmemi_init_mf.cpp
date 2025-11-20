/*
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This file is a part of the CANN Open Software.
 * Licensed under CANN Open Software License Agreement Version 1.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */
#include "shmemi_init_mf.h"

#ifdef BACKEND_MF

// smem api
#include <smem_bm_def.h>
#include <smem_bm.h>
#include <smem.h>
#include <smem_shm_def.h>
#include <smem_shm.h>
#include <smem_trans_def.h>
#include <smem_trans.h>

constexpr int DEFAULT_FLAG = 0;
constexpr int DEFAULT_ID = 0;
constexpr int DEFAULT_TIMEOUT = 120;
constexpr int DEFAULT_TEVENT = 0;
constexpr int DEFAULT_BLOCK_NUM = 1;

// smem need
static smem_shm_t g_smem_handle = nullptr;
static char *g_ipport = nullptr;

shmemi_init_mf::shmemi_init_mf(shmem_init_attr_t *attr, char *ipport)
{
    attributes = attr;
    g_ipport = ipport;

    aclrtGetDevice(&device_id);
    smem_set_conf_store_tls(false, nullptr, 0);

    int32_t status = smem_init(DEFAULT_FLAG);
    if (status != SHMEM_SUCCESS) {
        SHM_LOG_ERROR("smem_init Failed");
    }
}

shmemi_init_mf::~shmemi_init_mf()
{
    finalize_device_state();
    remove_heap();
    release_heap();
}

int shmemi_init_mf::init_device_state()
{
    int32_t status = SHMEM_SUCCESS;
    smem_shm_config_t config;
    status = smem_shm_config_init(&config);
    if (status != SHMEM_SUCCESS) {
        SHM_LOG_ERROR("smem_shm_config_init Failed");
        return SHMEM_SMEM_ERROR;
    }

    status = smem_shm_init(attributes->ip_port, attributes->n_ranks, attributes->my_rank, device_id, &config);
    if (status != SHMEM_SUCCESS) {
        SHM_LOG_ERROR("smem_shm_init Failed");
        return SHMEM_SMEM_ERROR;
    }

    config.shmInitTimeout = attributes->option_attr.shm_init_timeout;
    config.shmCreateTimeout = attributes->option_attr.shm_create_timeout;
    config.controlOperationTimeout = attributes->option_attr.control_operation_timeout;

    return SHMEM_SUCCESS;
}

int shmemi_init_mf::update_device_state(void* host_ptr, size_t size)
{
    if (g_smem_handle == nullptr) {
        SHM_LOG_ERROR("smem_shm_create Not Success, update_device_state Failed");
        return SHMEM_SMEM_ERROR;
    }
    return smem_shm_set_extra_context(g_smem_handle, host_ptr, size);
}

int shmemi_init_mf::finalize_device_state()
{
    // dummy function
    return SHMEM_SUCCESS;
}

int shmemi_init_mf::reserve_heap(shmemi_device_host_state_t &g_state)
{
    int32_t status = SHMEM_SUCCESS;
    void *gva = nullptr;
    g_smem_handle = smem_shm_create(DEFAULT_ID, attributes->n_ranks, attributes->my_rank, g_state.heap_size,
                            static_cast<smem_shm_data_op_type>(attributes->option_attr.data_op_engine_type),
                            DEFAULT_FLAG, &gva);

    if (g_smem_handle == nullptr || gva == nullptr) {
        SHM_LOG_ERROR("smem_shm_create Failed");
        return SHMEM_SMEM_ERROR;
    }
    g_state.heap_base = (void *)((uintptr_t)gva + g_state.heap_size * attributes->my_rank);
    uint32_t reach_info = 0;
    for (int32_t i = 0; i < g_state.npes; i++) {
        status = smem_shm_topology_can_reach(g_smem_handle, i, &reach_info);
        g_state.p2p_heap_base[i] = (void *)((uintptr_t)gva + g_state.heap_size * i);
        if (reach_info & SMEMS_DATA_OP_MTE) {
            g_state.topo_list[i] |= SHMEM_TRANSPORT_MTE;
        }
        if (reach_info & SMEMS_DATA_OP_SDMA) {
            g_state.sdma_heap_base[i] = (void *)((uintptr_t)gva + g_state.heap_size * i);
        } else {
            g_state.sdma_heap_base[i] = NULL;
        }
        if (reach_info & SMEMS_DATA_OP_RDMA) {
            g_state.topo_list[i] |= SHMEM_TRANSPORT_ROCE;
        }
    }
    if (g_ipport != nullptr) {
        delete[] g_ipport;
        g_ipport = nullptr;
        attributes->ip_port = nullptr;
    } else {
        SHM_LOG_WARN("my_rank:" << attributes->my_rank << " g_ipport is released in advance!");
        attributes->ip_port = nullptr;
    }
    g_state.is_shmem_created = true;
    return status;
}

int shmemi_init_mf::setup_heap(shmemi_device_host_state_t &g_state)
{
    int32_t status = SHMEM_SUCCESS;
    return status;
}

int shmemi_init_mf::remove_heap()
{
    int32_t status = SHMEM_SUCCESS;
    return status;
}

int shmemi_init_mf::release_heap()
{
    if (g_smem_handle != nullptr) {
        int32_t status = smem_shm_destroy(g_smem_handle, 0);
        if (status != SHMEM_SUCCESS) {
            SHM_LOG_ERROR("smem_shm_destroy Failed");
            return SHMEM_SMEM_ERROR;
        }
        g_smem_handle = nullptr;
    }
    smem_shm_uninit(0);
    smem_uninit();
    return SHMEM_SUCCESS;
}

int shmemi_init_mf::transport_init(shmemi_device_host_state_t &g_state)
{
    return SHMEM_SUCCESS;
}

int shmemi_init_mf::transport_finalize()
{
    return SHMEM_SUCCESS;
}

#endif