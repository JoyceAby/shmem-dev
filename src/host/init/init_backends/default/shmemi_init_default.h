
/*
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This file is a part of the CANN Open Software.
 * Licensed under CANN Open Software License Agreement Version 1.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */
#ifndef SHMEMI_INIT_NORMAL_H
#define SHMEMI_INIT_NORMAL_H

#include <iostream>

#include "init/init_backends/shmemi_init_base.h"

#include "host/shmem_host_def.h"
#include "internal/host_device/shmemi_types.h"

#include "mem/shmemi_global_state.h"
#include "mem/shmemi_heap.h"

#include "bootstrap/shmemi_bootstrap.h"

#include "transport/shmemi_transport.h"

class shmemi_init_default: public shmemi_init_base {
public:
    shmemi_init_default(shmem_init_attr_t *attr);
    ~shmemi_init_default();

    int init_device_state() override;
    int finalize_device_state() override;
    int update_device_state(void* host_ptr, size_t size) override;

    int reserve_heap(shmemi_device_host_state_t &g_state) override;
    int setup_heap(shmemi_device_host_state_t &g_state) override;
    int remove_heap() override;
    int release_heap() override;

    int transport_init(shmemi_device_host_state_t &g_state) override;
    int transport_finalize() override;
private:
    int mype;
    int npes;
    int device_id;

    // global_state
    global_state_reigister *global_state_d = nullptr;

    // heap_obj
    shmem_symmetric_heap *heap_obj = nullptr;
};

#endif // SHMEMI_INIT_NORMAL_H