/*
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This file is a part of the CANN Open Software.
 * Licensed under CANN Open Software License Agreement Version 1.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */
#include "shmemi_heap.h"
#include "host/shmem_host_def.h"
#include "common/shmemi_host_types.h"
#include "common/shmemi_logger.h"

shmem_symmetric_heap::shmem_symmetric_heap(int pe_id, int pe_size, int dev_id): mype(pe_id), npes(pe_size), device_id(dev_id)
{
    physical_handle_list.resize(pe_size);
    share_handle_list.resize(pe_size);
    pid_list.resize(pe_size);

    memprop.handleType = ACL_MEM_HANDLE_TYPE_NONE;
    memprop.allocationType = ACL_MEM_ALLOCATION_TYPE_PINNED;
    memprop.memAttr = ACL_HBM_MEM_HUGE;
    memprop.location.type = ACL_MEM_LOCATION_TYPE_DEVICE;
    memprop.location.id = dev_id;
    memprop.reserve = 0;
}

int shmem_symmetric_heap::reserve_heap(size_t size)
{
    peer_heap_base_p2p_ = (void **)std::calloc(npes, sizeof(void *));

    // reserve virtual ptrs
    for (int i = 0; i < npes; i++) {
        peer_heap_base_p2p_[i] = NULL;
    }

    // reserve local heap_base_
    SHMEM_CHECK_RET(aclrtReserveMemAddress(&(peer_heap_base_p2p_[mype]), size, 0, nullptr, 1));
    heap_base_ = peer_heap_base_p2p_[mype];

    // alloc local physical memory
    SHMEM_CHECK_RET(aclrtMallocPhysical(&local_handle, size, &memprop, 0));

    alloc_size = size;
    SHMEM_CHECK_RET(aclrtMapMem(peer_heap_base_p2p_[mype], alloc_size, 0, local_handle, 0));

    return SHMEM_SUCCESS;
}

int shmem_symmetric_heap::export_memory()
{
    // Get share_handle
    SHMEM_CHECK_RET(aclrtMemExportToShareableHandle(local_handle, memprop.handleType, 0, &share_handle));
    return SHMEM_SUCCESS;
}

int shmem_symmetric_heap::export_pid()
{
    // Get local pid
    SHMEM_CHECK_RET(aclrtDeviceGetBareTgid(&my_pid));
    return SHMEM_SUCCESS;
}

int shmem_symmetric_heap::import_pid()
{
    // Get all pids
    g_boot_handle.allgather(&my_pid, pid_list.data(), 1 * sizeof(int), &g_boot_handle);

    // Add Pid into white list
    std::vector<int32_t> share_pid = {};
    for (int i = 0; i < npes; i++) {
        if (i == mype) {
            continue;
        }
        // Check if p2p connected
        if (peer_heap_base_p2p_[i] == NULL) {
            continue;
        }
        share_pid.push_back(pid_list[i]);
    }

    SHMEM_CHECK_RET(aclrtMemSetPidToShareableHandle(share_handle, share_pid.data(), npes - 1));
    return SHMEM_SUCCESS;
}

int shmem_symmetric_heap::import_memory()
{
    g_boot_handle.allgather(&share_handle, share_handle_list.data(), 1 * sizeof(uint64_t), &g_boot_handle);
    for (int i = 0; i < npes; i++) {
        if (i == mype) {
            physical_handle_list[i] = local_handle;
            continue;
        }
        // Check if p2p connected
        if (peer_heap_base_p2p_[i] == NULL) {
            continue;
        }
        SHMEM_CHECK_RET(aclrtMemImportFromShareableHandle(share_handle_list[i], device_id, &(physical_handle_list[i])));
    }

    return SHMEM_SUCCESS;
}

int shmem_symmetric_heap::setup_heap()
{
    // MTE p2p_heap_base_ reserve
    int local_offset = mype * npes;
    for (int i = 0; i < npes; i++) {
        if (i == mype)
            continue;
        
        if (g_host_state.transport_map[local_offset + i] & 1) {
            SHMEM_CHECK_RET(aclrtReserveMemAddress(&(peer_heap_base_p2p_[i]), alloc_size, 0, nullptr, 1));
        }
    }

    SHMEM_CHECK_RET(export_memory());
    SHMEM_CHECK_RET(export_pid());
    SHMEM_CHECK_RET(import_pid());
    SHMEM_CHECK_RET(import_memory());

    // Shareable Handle Map
    for (int i = 0; i < npes; i++) {
        // Check if p2p connected
        if (i != mype && peer_heap_base_p2p_[i] != NULL) {
            SHMEM_CHECK_RET(aclrtMapMem(peer_heap_base_p2p_[i], alloc_size, 0, physical_handle_list[i], 0));
        }
    }
    return SHMEM_SUCCESS;
}

int shmem_symmetric_heap::remove_heap()
{
    for (int i = 0; i < npes; i++) {
        if (peer_heap_base_p2p_[i] != NULL) {
            SHMEM_CHECK_RET(aclrtUnmapMem(peer_heap_base_p2p_[i]));
        }
    }
    return SHMEM_SUCCESS;
}

int shmem_symmetric_heap::unreserve_heap()
{
    for (int i = 0; i < npes; i++) {
        if (peer_heap_base_p2p_[i] != NULL) {
            SHMEM_CHECK_RET(aclrtReleaseMemAddress(peer_heap_base_p2p_[i]));
        }
    }
    SHMEM_CHECK_RET(aclrtFreePhysical(local_handle));
    return SHMEM_SUCCESS;
}

void *shmem_symmetric_heap::get_heap_base()
{
    return heap_base_;
}

void *shmem_symmetric_heap::get_peer_heap_base_p2p(int pe_id)
{
    return peer_heap_base_p2p_[pe_id];
}