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

#define BOOTSTRAP_MODULE_MPI "shmem_bootstrap_mpi.so"
#define BOOTSTRAP_MODULE_UID "shmem_bootstrap_uid.so"

#define BOOTSTRAP_PLUGIN_INIT_FUNC "shmemi_bootstrap_plugin_init"

shmemi_bootstrap_handle_t g_boot_handle;

static void *plugin_hdl = nullptr;
static char *plugin_name = nullptr;

int bootstrap_loader_finalize(shmemi_bootstrap_handle_t *handle)
{
    int status = handle->finalize(handle);

    if (status != 0)
        SHM_LOG_ERROR("Bootstrap plugin finalize failed for " << plugin_name);

    dlclose(plugin_hdl);
    plugin_hdl = nullptr;
    free(plugin_name);
    plugin_name = nullptr;

    return 0;
}

// for UID
int32_t shmemi_bootstrap_pre_init() {

}

void shmemi_bootstrap_loader()
{
    dlerror();
    if (plugin_hdl == nullptr) {

        plugin_hdl = dlopen(plugin_name, RTLD_NOW);
    }
    dlerror();
}

void shmemi_bootstrap_free()
{
    if (plugin_hdl != nullptr) {
        dlclose(plugin_hdl);
        plugin_hdl = nullptr;
    }

    if (plugin_name != nullptr) {
        free(plugin_name);
        plugin_name = nullptr;
    }
}

int32_t shmemi_bootstrap_init(int flags, shmemi_bootstrap_attr_t *attr) {
    int32_t status = SHMEM_SUCCESS;
    void *arg;
    if (flags & SHMEMX_INIT_WITH_MPI) {
        plugin_name = BOOTSTRAP_MODULE_MPI;
        arg = (attr != NULL) ? attr->mpi_comm : NULL;
    } else if (flags & SHMEMX_INIT_WITH_UNIQUEID) {
        plugin_name = BOOTSTRAP_MODULE_UID;
        status = shmemi_bootstrap_pre_init();
    } else {
        SHM_LOG_ERROR("Unknown Type for bootstrap");
        status = SHMEM_INVALID_PARAM;
    }
    shmemi_bootstrap_loader();
   
    if (!plugin_hdl) {
        SHM_LOG_ERROR("Bootstrap unable to load " << plugin_name << ", err is: " << stderr);
        shmemi_bootstrap_free();
        return SHMEM_INVALID_VALUE;
    }

    int (*plugin_init)(void *, shmemi_bootstrap_handle_t *);
    *((void **)&plugin_init) = dlsym(plugin_hdl, BOOTSTRAP_PLUGIN_INIT_FUNC);
    if (!plugin_init) {
        SHM_LOG_ERROR("Bootstrap plugin init func dlsym failed");
        shmemi_bootstrap_free();
        return SHMEM_INNER_ERROR;
    }
    status = plugin_init(arg, &g_boot_handle);
    if (status != 0) {
        SHM_LOG_ERROR("Bootstrap plugin init failed for " << plugin_name);
        shmemi_bootstrap_free();
        return SHMEM_INNER_ERROR;
    }
    return status;
}

void shmemi_bootstrap_finalize() {
    g_boot_handle.finalize(&g_boot_handle);

    dlclose(plugin_hdl);
}
