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
#include <cstring>
#include <vector>
#include <iostream>
#include <sstream>
#include <netdb.h>
#include <arpa/inet.h>
#include <functional>

#include "acl/acl.h"
#include "shmemi_host_common.h"

using namespace std;

#define DEFAULT_MY_PE (-1)
#define DEFAULT_N_PES (-1)

constexpr int DEFAULT_FLAG = 0;
constexpr int DEFAULT_ID = 0;
constexpr int DEFAULT_TIMEOUT = 120;
constexpr int DEFAULT_TEVENT = 0;
constexpr int DEFAULT_BLOCK_NUM = 1;

// initializer
#define SHMEM_DEVICE_HOST_STATE_INITIALIZER                                              \
    {                                                                                    \
        (1 << 16) + sizeof(shmemi_device_host_state_t), /* version */                    \
            (DEFAULT_MY_PE),                            /* mype */                       \
            (DEFAULT_N_PES),                            /* npes */                       \
            NULL,                                       /* heap_base */                  \
            {NULL},                                     /* p2p_heap_base */              \
            {NULL},                                     /* rdma_heap_base */             \
            {NULL},                                     /* sdma_heap_base */             \
            {},                                         /* topo_list */                  \
            SIZE_MAX,                                   /* heap_size */                  \
            {NULL},                                     /* team_pools */                 \
            0,                                          /* sync_pool */                  \
            0,                                          /* sync_counter */               \
            0,                                          /* core_sync_pool */             \
            0,                                          /* core_sync_counter */          \
            0,                                          /* host_hash */                  \
            false,                                      /* shmem_is_shmem_initialized */ \
            false,                                      /* shmem_is_shmem_created */     \
            {0, 16 * 1024, 0},                          /* shmem_mte_config */           \
            0,                                          /* qp_info */                    \
    }

shmemi_device_host_state_t g_state = SHMEM_DEVICE_HOST_STATE_INITIALIZER;
shmemi_host_state_t g_state_host = {nullptr, DEFAULT_TEVENT, DEFAULT_BLOCK_NUM};
shmem_init_attr_t g_attr;

static char *g_ipport = nullptr;

int32_t version_compatible()
{
    int32_t status = SHMEM_SUCCESS;
    return status;
}

int32_t shmemi_options_init()
{
    int32_t status = SHMEM_SUCCESS;
    return status;
}

uint64_t shmemi_get_host_hash()
{
    char hostname[128];
    struct hostent *he;

    if (gethostname(hostname, sizeof(hostname)) != 0) {
        perror("gethostname");
        return 0;
    }

    if ((he = gethostbyname(hostname)) == NULL) {
        perror("gethostbyname");
        return 0;
    }

    // Host IP Address
    for (int i = 0; he->h_addr_list[i] != NULL; i++) {
        char *ip = inet_ntoa(*(struct in_addr*)he->h_addr_list[i]);
    }

    std::size_t host_hash = std::hash<std::string>{}(hostname);
    return host_hash;
}

int32_t shmemi_state_init_attr(shmem_init_attr_t *attributes)
{
    int32_t status = SHMEM_SUCCESS;
    g_state.mype = attributes->my_rank;
    g_state.npes = attributes->n_ranks;
    g_state.heap_size = attributes->local_mem_size + SHMEM_EXTRA_SIZE;
    g_state.host_hash = shmemi_get_host_hash();

    aclrtStream stream = nullptr;
    SHMEM_CHECK_RET(aclrtCreateStream(&stream));
    g_state_host.default_stream = stream;
    g_state_host.default_event_id = DEFAULT_TEVENT;
    g_state_host.default_block_num = DEFAULT_BLOCK_NUM;
    return status;
}

int32_t check_attr(shmem_init_attr_t *attributes)
{
    if ((attributes->my_rank < 0) || (attributes->n_ranks <= 0)) {
        SHM_LOG_ERROR("my_rank:" << attributes->my_rank << " and n_ranks: " << attributes->n_ranks
                                 << " cannot be less 0 , n_ranks still cannot be equal 0");
        return SHMEM_INVALID_VALUE;
    } else if (attributes->n_ranks > SHMEM_MAX_RANKS) {
        SHM_LOG_ERROR("n_ranks: " << attributes->n_ranks << " cannot be more than " << SHMEM_MAX_RANKS);
        return SHMEM_INVALID_VALUE;
    } else if (attributes->my_rank >= attributes->n_ranks) {
        SHM_LOG_ERROR("n_ranks:" << attributes->n_ranks << " cannot be less than my_rank:" << attributes->my_rank);
        return SHMEM_INVALID_PARAM;
    } else if (attributes->local_mem_size <= 0) {
        SHM_LOG_ERROR("local_mem_size:" << attributes->local_mem_size << " cannot be less or equal 0");
        return SHMEM_INVALID_VALUE;
    }
    return SHMEM_SUCCESS;
}

shmemi_init_base* init_manager;

int32_t shmemi_control_barrier_all()
{
    return g_boot_handle.barrier(&g_boot_handle);
}

int32_t update_device_state()
{
    return init_manager->update_device_state((void *)&g_state, sizeof(shmemi_device_host_state_t));
}

int32_t shmem_set_data_op_engine_type(shmem_init_attr_t *attributes, data_op_engine_type_t value)
{
    attributes->option_attr.data_op_engine_type = value;
    return SHMEM_SUCCESS;
}

int32_t shmem_set_timeout(shmem_init_attr_t *attributes, uint32_t value)
{
    attributes->option_attr.shm_init_timeout = value;
    attributes->option_attr.shm_create_timeout = value;
    attributes->option_attr.control_operation_timeout = value;
    return SHMEM_SUCCESS;
}

int32_t shmem_set_attr(int32_t my_rank, int32_t n_ranks, uint64_t local_mem_size, const char *ip_port,
                       shmem_init_attr_t **attributes)
{
    SHM_ASSERT_RETURN(local_mem_size <= SHMEM_MAX_LOCAL_SIZE, SHMEM_INVALID_VALUE);
    SHM_ASSERT_RETURN(n_ranks <= SHMEM_MAX_RANKS, SHMEM_INVALID_VALUE);
    SHM_ASSERT_RETURN(my_rank <= SHMEM_MAX_RANKS, SHMEM_INVALID_VALUE);
    *attributes = &g_attr;
    if (ip_port == nullptr) {
        SHM_LOG_ERROR("my_rank:" << my_rank << " ip_port is NULL!");
        return SHMEM_INVALID_PARAM;
    }
    // 安全警告：此处strlen依赖ip_port以null结尾，如果ip_port不是合法的C字符串，将导致越界读取
    size_t ip_len = strlen(ip_port);
    g_ipport = new (std::nothrow) char[ip_len + 1];
    if (g_ipport == nullptr) {
        SHM_LOG_ERROR("my_rank:" << my_rank << " failed to allocate IP port string!");
        return SHMEM_INNER_ERROR;
    }
    std::copy(ip_port, ip_port + ip_len + 1, g_ipport);
    if (g_ipport == nullptr) {
        SHM_LOG_ERROR("my_rank:" << my_rank << " g_ipport is nullptr!");
        return SHMEM_INVALID_VALUE;
    }
    int attr_version = (1 << 16) + sizeof(shmem_init_attr_t);
    g_attr.my_rank = my_rank;
    g_attr.n_ranks = n_ranks;
    g_attr.ip_port = g_ipport;
    g_attr.local_mem_size = local_mem_size;
    g_attr.option_attr = {attr_version, SHMEM_DATA_OP_MTE, DEFAULT_TIMEOUT, 
                               DEFAULT_TIMEOUT, DEFAULT_TIMEOUT};
    // g_attr_init = true;
    return SHMEM_SUCCESS;
}

int32_t shmem_init_status()
{
    if (!g_state.is_shmem_created)
        return SHMEM_STATUS_NOT_INITIALIZED;
    else if (!g_state.is_shmem_initialized)
        return SHMEM_STATUS_SHM_CREATED;
    else if (g_state.is_shmem_initialized)
        return SHMEM_STATUS_IS_INITIALIZED;
    else
        return SHMEM_STATUS_INVALID;
}

int32_t shmem_init_attr(shmemx_bootstrap_t bootstrap_flags, shmem_init_attr_t *attributes)
{
    int32_t ret;

    // config init
    SHM_ASSERT_RETURN(attributes != nullptr, SHMEM_INVALID_PARAM);
    SHMEM_CHECK_RET(check_attr(attributes));
    SHMEM_CHECK_RET(version_compatible());
    SHMEM_CHECK_RET(shmemi_options_init());

    // bootstrap init
    shmemi_bootstrap_attr_t attr = {};
    SHMEM_CHECK_RET(shmemi_bootstrap_init(bootstrap_flags, &attr));

    // shmem basic init
#ifdef BACKEND_MF
    init_manager = new shmemi_init_mf(attributes, g_ipport);
#else
    init_manager = new shmemi_init_default(attributes);
#endif
    SHMEM_CHECK_RET(shmemi_state_init_attr(attributes));
    SHMEM_CHECK_RET(init_manager->init_device_state());
    SHMEM_CHECK_RET(init_manager->reserve_heap(g_state));
    SHMEM_CHECK_RET(init_manager->transport_init(g_state));
    SHMEM_CHECK_RET(init_manager->setup_heap(g_state));
    
    // shmem submodules init
    SHMEM_CHECK_RET(memory_manager_initialize(g_state.heap_base, g_state.heap_size));
    SHMEM_CHECK_RET(shmemi_team_init(g_state.mype, g_state.npes));
    SHMEM_CHECK_RET(shmemi_sync_init());
    g_state.is_shmem_initialized = true;
    SHMEM_CHECK_RET(update_device_state());
    SHMEM_CHECK_RET(shmemi_control_barrier_all());
    return SHMEM_SUCCESS;
}

int32_t shmem_finalize()
{
    SHMEM_CHECK_RET(shmemi_team_finalize());
    delete init_manager;

    shmemi_bootstrap_finalize();
    return SHMEM_SUCCESS;
}

void shmem_info_get_version(int *major, int *minor)
{
    SHM_ASSERT_RET_VOID(major != nullptr && minor != nullptr);
    *major = SHMEM_MAJOR_VERSION;
    *minor = SHMEM_MINOR_VERSION;
}

void shmem_info_get_name(char *name)
{
    SHM_ASSERT_RET_VOID(name != nullptr);
    std::ostringstream oss;
    oss << "SHMEM v" << SHMEM_VENDOR_MAJOR_VER << "." << SHMEM_VENDOR_MINOR_VER << "." << SHMEM_VENDOR_PATCH_VER;
    auto version_str = oss.str();
    size_t i;
    for (i = 0; i < SHMEM_MAX_NAME_LEN - 1 && version_str[i] != '\0'; i++) {
        name[i] = version_str[i];
    }
    name[i] = '\0';
}