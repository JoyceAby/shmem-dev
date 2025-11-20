/*
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This file is a part of the CANN Open Software.
 * Licensed under CANN Open Software License Agreement Version 1.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */
#ifdef SHMEM_BOOTSTRAP_UID
#include "shmemi_bootstrap.h"

typedef struct {
    char ifname[64];
    int af;
    int32_t ip, port;
} shmemi_bootstrap_uid_state_t;

static shmemi_bootstrap_uid_state_t shmemi_bootstrap_uid_state;

int shmemi_bootstrap_plugin_init(void *args, shmemi_bootstrap_handle_t *boot_handle) {
    // INIT

    handle->allgather = shmemi_bootstrap_uid_allgather;
    handle->barrier = shmemi_bootstrap_uid_allgather;
    handle->finalize = shmemi_bootstrap_uid_finalize;

}

int shmemi_bootstrap_uid_finalize(shmemi_bootstrap_handle_t *boot_handle) {

}

int shmemi_bootstrap_uid_allgather(void *dst, void *src, size_t size, shmemi_bootstrap_handle_t *boot_handle) {

}

int shmemi_bootstrap_uid_barrier(shmemi_bootstrap_handle_t *boot_handle) {

}

#endif // SHMEM_BOOTSTRAP_UID