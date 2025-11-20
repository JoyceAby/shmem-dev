
/*
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This file is a part of the CANN Open Software.
 * Licensed under CANN Open Software License Agreement Version 1.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */
#ifndef SHMEMI_INIT_MF_H
#define SHMEMI_INIT_MF_H

#include <iostream>

#include "init/init_backends/shmemi_init_base.h"
#include "shmemi_host_common.h"
#include "internal/host_device/shmemi_types.h"

class shmemi_init_mf: public shmemi_init_base {
public:
    shmemi_init_mf(shmem_init_attr_t *attr, char *ipport);
    ~shmemi_init_mf();

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
    int32_t device_id;

    shmem_init_attr_t *attributes;
    char *g_ipport = nullptr;
};

#endif // SHMEMI_INIT_MF_H