/*
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This file is a part of the CANN Open Software.
 * Licensed under CANN Open Software License Agreement Version 1.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */
#ifndef SHMEMI_INIT_H
#define SHMEMI_INIT_H

#include "stdint.h"
#include "internal/host_device/shmemi_types.h"

#ifdef BACKEND_MF
#include "init/init_backends/mf/shmemi_init_mf.h"
#else
#include "init/init_backends/default/shmemi_init_default.h"
#endif

extern shmemi_device_host_state_t g_state;
extern shmemi_host_state_t g_state_host;

int32_t shmemi_control_barrier_all();

int32_t update_device_state(void);

#endif  // SHMEMI_INIT_H
