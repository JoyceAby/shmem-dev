/*
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This file is a part of the CANN Open Software.
 * Licensed under CANN Open Software License Agreement Version 1.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */
#include "shmemi_host_common.h"
#include "dlfcn.h"

#include "transport/shmemi_transport.h"

static void *transport_mte_lib = NULL;
static void *transport_rdma_lib = NULL;

uint64_t *host_hash_list;

shmemi_host_state_t g_host_state;

int32_t shmemi_transport_init(shmemi_device_host_state_t &g_state) {
    g_host_state.num_choosen_transport = 2;
    g_host_state.transport_map = (int *)calloc(g_state.npes * g_state.npes, sizeof(int));
    g_host_state.pe_info = (shmemi_transport_pe_info *)calloc(g_state.npes, sizeof(shmemi_transport_pe_info));

    transport_mte_lib = dlopen("shmem_transport_mte.so", RTLD_NOW);
    if (!transport_mte_lib) {
        SHM_LOG_ERROR("Transport unable to load " << "shmem_transport_mte.so" << ", err is: " << stderr);
        return SHMEM_INVALID_VALUE;
    }
    transport_rdma_lib = dlopen("shmem_transport_rdma.so", RTLD_NOW);
    if (!transport_rdma_lib) {
        SHM_LOG_ERROR("Transport unable to load " << "shmem_transport_rdma.so" << ", err is: " << stderr);
        return SHMEM_INVALID_VALUE;
    }

    transport_init_func init_mte_fn;
    init_mte_fn = (transport_init_func)dlsym(transport_mte_lib, "shmemi_mte_init");
    if (!init_mte_fn) {
        dlclose(transport_mte_lib);
        transport_mte_lib = NULL;
        SHM_LOG_ERROR("Unable to get info from " << "shmem_transport_mte.so" << ".");
        return SHMEM_INVALID_VALUE;
    }

    // Package my_info
    shmemi_transport_pe_info_t my_info;
    my_info.pe = g_state.mype;
    my_info.host_hash = g_state.host_hash;

    int32_t device_id;
    SHMEM_CHECK_RET(aclrtGetDevice(&device_id));
    my_info.dev_id = device_id;
    int32_t logicDeviceId = -1;
    rtLibLoader& loader = rtLibLoader::getInstance();
    if (loader.isLoaded()) {
        loader.getLogicDevId(device_id, &logicDeviceId);
    }
    g_host_state.choosen_transports[1].logical_dev_id = logicDeviceId;
    g_host_state.choosen_transports[1].dev_id = device_id;

    // AllGather All pe's host info
    g_boot_handle.allgather((void *)&my_info, g_host_state.pe_info, sizeof(shmemi_transport_pe_info_t), &g_boot_handle);

    SHMEM_CHECK_RET(init_mte_fn(&g_host_state.choosen_transports[0], &g_state));

    transport_init_func init_rdma_fn;
    init_rdma_fn = (transport_init_func)dlsym(transport_rdma_lib, "shmemi_rdma_init");
    if (!init_rdma_fn) {
        dlclose(transport_rdma_lib);
        transport_rdma_lib = NULL;
        SHM_LOG_ERROR("Unable to get info from " << "shmem_transport_rdma.so" << ".");
        return SHMEM_INVALID_VALUE;
    }
    SHMEM_CHECK_RET(init_rdma_fn(&g_host_state.choosen_transports[1], &g_state));

    return SHMEM_SUCCESS;
}

int32_t shmemi_build_transport_map(shmemi_device_host_state_t &g_state) {
    int *local_map = NULL;
    local_map = (int *)calloc(g_state.npes, sizeof(int));

    shmemi_transport_t t;

    // Loop can_access_peer, j = 0 means MTE, j = 1 means RDMA ...
    for (int j = 0; j < g_host_state.num_choosen_transport; j++) {
        t = g_host_state.choosen_transports[j];

        for (int i = 0; i < g_state.npes; i++) {
            int reach = 0;

            SHMEM_CHECK_RET(t.can_access_peer(&reach, g_host_state.pe_info + i, &t, &g_state));

            if (reach) {
                int m = 1 << j;
                local_map[i] |= m;
            }
        }
    }

    for (int i = 0; i < g_state.npes; i++) {
        g_state.topo_list[i] = static_cast<uint8_t>(local_map[i]);
    }

    g_boot_handle.allgather(local_map, g_host_state.transport_map, g_state.npes * sizeof(int), &g_boot_handle);

    if (local_map) free(local_map);
    return SHMEM_SUCCESS;
}

int32_t shmemi_transport_setup_connections(shmemi_device_host_state_t &g_state) {
    shmemi_transport_t t;
    // MTE 
    t = g_host_state.choosen_transports[0];

    int *mte_peer_list;
    int mte_peer_num = 0;
    mte_peer_list = (int *)calloc(g_state.npes, sizeof(int));

    int local_offset = g_state.mype * g_state.npes;
    for (int i = 0; i < g_state.npes; i++) {
        if (i == g_state.mype)
            continue;
        if (g_host_state.transport_map[local_offset + i] & 1) {
            shmemi_transport_pe_info_t *peer_info = (g_host_state.pe_info + i);
            mte_peer_list[mte_peer_num] = peer_info->dev_id;
            ++mte_peer_num;
        }
    }

    t.connect_peers(&t, mte_peer_list, mte_peer_num, &g_state);
    t = g_host_state.choosen_transports[1];
    t.connect_peers(&t, mte_peer_list, mte_peer_num, &g_state);

    return 0;
}

int32_t shmemi_transport_finalize() {
    shmemi_transport_t t;
    // MTE 
    t = g_host_state.choosen_transports[0];
    t.finalize(&t, &g_state);

    if (transport_mte_lib != NULL) {
        dlclose(transport_mte_lib);
        transport_mte_lib = NULL;
    }
    
    t = g_host_state.choosen_transports[1];
    t.finalize(&t, &g_state);

    if (transport_rdma_lib != NULL) {
        dlclose(transport_rdma_lib);
        transport_rdma_lib = NULL;
    }
    return 0;
}
