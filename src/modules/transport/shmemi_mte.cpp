/*
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This file is a part of the CANN Open Software.
 * Licensed under CANN Open Software License Agreement Version 1.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include <stdint.h>
#include <stdlib.h>
#include <iostream>
#include <acl/acl.h>

#include "shmemi_host_common.h"
#include "internal/host_device/shmemi_types.h"
#include "transport/shmemi_transport.h"

#ifdef __cplusplus
extern "C" {
#endif

int shmemi_mte_can_access_peer(int *access, shmemi_transport_pe_info_t *peer_info, shmemi_transport *t, shmemi_device_host_state_t *g_state) {
    // host_id same return 1, otherwise 0
    if (g_state->host_hash == peer_info->host_hash) {
        *access = 1;
    } else {
        *access = 0;
    }
    return 0;
}

int shmemi_mte_connect_peers(shmemi_transport *t, int *selected_dev_ids, int num_selected_devs, shmemi_device_host_state_t *g_state) {
    // EnablePeerAccess
    for (int i = 0; i < num_selected_devs; i++) {
        SHMEM_CHECK_RET(aclrtDeviceEnablePeerAccess(selected_dev_ids[i], 0));
    }
    return 0;
}

int shmemi_mte_finalize(shmemi_transport *t, shmemi_device_host_state_t *g_state) {
    return 0;
}

// control plane
int shmemi_mte_init(shmemi_transport_t *t, shmemi_device_host_state_t *g_state) {
    t->can_access_peer = shmemi_mte_can_access_peer;
    t->connect_peers = shmemi_mte_connect_peers;
    t->finalize = shmemi_mte_finalize;

    return 0;
}

#ifdef __cplusplus
}
#endif