/*
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This file is a part of the CANN Open Software.
 * Licensed under CANN Open Software License Agreement Version 1.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */
#include "shmemi_init_default.h"
#include "common/shmemi_logger.h"

shmemi_init_default::shmemi_init_default(shmem_init_attr_t *attr)
{
    mype = attr->my_rank;
    npes = attr->n_ranks;
    auto status = aclrtGetDevice(&device_id);
    if (status != 0) {
        SHM_LOG_ERROR("Get Device_id error");
    }
}

shmemi_init_default::~shmemi_init_default()
{
    finalize_device_state();
    remove_heap();
    release_heap();
    transport_finalize();
}

int shmemi_init_default::init_device_state()
{
    global_state_d = new global_state_reigister(device_id);
    if (global_state_d->get_init_status() != 0) {
        SHM_LOG_ERROR("global_state reigister error");
    }
    return SHMEM_SUCCESS;
}

int shmemi_init_default::finalize_device_state()
{
    delete global_state_d;
    return SHMEM_SUCCESS;
}

int shmemi_init_default::update_device_state(void* host_ptr, size_t size)
{
    SHMEM_CHECK_RET(aclrtMemcpy(global_state_d->get_ptr(), size, host_ptr, size, ACL_MEMCPY_HOST_TO_DEVICE));
    return SHMEM_SUCCESS;
}

int shmemi_init_default::reserve_heap(shmemi_device_host_state_t &g_state)
{
    heap_obj = new shmem_symmetric_heap(mype, npes, device_id);

    SHMEM_CHECK_RET(heap_obj->reserve_heap(g_state.heap_size));

    g_state.heap_base = heap_obj->get_heap_base();

    return SHMEM_SUCCESS;
}

int shmemi_init_default::setup_heap(shmemi_device_host_state_t &g_state)
{
    SHMEM_CHECK_RET(heap_obj->setup_heap());

    for (int32_t i = 0; i < g_state.npes; i++) {
        g_state.p2p_heap_base[i] = heap_obj->get_peer_heap_base_p2p(i);
    }
    g_state.is_shmem_created = true;

    return SHMEM_SUCCESS;
}

int shmemi_init_default::remove_heap()
{
    SHMEM_CHECK_RET(heap_obj->remove_heap());
    return SHMEM_SUCCESS;
}

int shmemi_init_default::release_heap()
{
    SHMEM_CHECK_RET(heap_obj->unreserve_heap());
    return SHMEM_SUCCESS;
}

int shmemi_init_default::transport_init(shmemi_device_host_state_t &g_state)
{
    SHMEM_CHECK_RET(shmemi_transport_init(g_state));                    // mte init && rdma init
    SHMEM_CHECK_RET(shmemi_build_transport_map(g_state));               // build transport_map
    SHMEM_CHECK_RET(shmemi_transport_setup_connections(g_state));       // connect_endpoints by transpost_map
    return SHMEM_SUCCESS;
}

int shmemi_init_default::transport_finalize()
{
    SHMEM_CHECK_RET(shmemi_transport_finalize());
    return SHMEM_SUCCESS;
}