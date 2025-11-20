/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 */
#include <iostream>
#include <string>
#include <vector>
#include <gtest/gtest.h>

#include "acl/acl.h"
#include "shmem_api.h"
#include "shmemi_host_common.h"

using namespace std;

extern int test_gnpu_num;
extern int test_first_npu;
extern void test_mutil_task(std::function<void(int, int, uint64_t)> func, uint64_t local_mem_size, int process_count);
extern void test_init(int rank_id, int n_ranks, uint64_t local_mem_size, aclrtStream *st);
extern void test_finalize(aclrtStream stream, int device_id);

extern void shmem_barrier_all();

class HostPutMemTest {
public:
    inline HostPutMemTest() {}
    inline void Init(void* gva, void* dev, int64_t rank_, size_t element_size_)
    {
        gva_gm = gva;
        dev_gm = dev;

        rank = rank_;
        element_size = element_size_;
    }
    inline void Process()
    {
        shmem_putmem(gva_gm, dev_gm, element_size, rank);
    }
private:
    void *gva_gm;
    void *dev_gm;

    int64_t rank;
    size_t element_size;
};

template<typename T>
class HostGetMemTest {
public:
    inline HostGetMemTest() {}
    inline void Init(T* gva, T* dev, int64_t rank_, size_t element_each_rank_, int64_t rank_size_)
    {
        gva_gm = gva;
        dev_gm = dev;

        rank = rank_;
        element_each_rank = element_each_rank_;
        rank_size = rank_size_;
    }
    inline void Process(bool is_nbi = false)
    {
        if (is_nbi) {
            for (int i = 0; i < rank_size; i++) {
                shmem_get_float_mem_nbi(dev_gm + 16 * i, gva_gm, 16, i % rank_size);
                // how to sync in host process instead of barrier_all
                // AscendC::SetFlag<AscendC::HardEvent::MTE3_MTE2>(EVENT_ID0);
                // AscendC::WaitFlag<AscendC::HardEvent::MTE3_MTE2>(EVENT_ID0);
            }
        } else {
            for (int i = 0; i < rank_size; i++) {
                shmem_get_float_mem(dev_gm + 16 * i, gva_gm, 16, i % rank_size);
            }
        }
    }
private:
    T *gva_gm;
    T *dev_gm;

    int64_t rank;
    size_t element_each_rank;
    int64_t rank_size;
};


void host_test_putmem(void* gva, void* dev, int64_t rank_, size_t element_size_)
{
    HostPutMemTest op;
    op.Init(gva, dev, rank_, element_size_);
    op.Process();
}


template<typename T>
void host_test_getmem(T* gva, T* dev, int64_t rank_, size_t element_each_rank_, int64_t rank_size_)
{
    HostGetMemTest<T> op;
    op.Init(gva, dev, rank_, element_each_rank_, rank_size_);
    op.Process();
}


static void host_test_put_get_mem(int rank_id, int rank_size, uint64_t local_mem_size)
{
    int total_size = 16 * static_cast<int>(rank_size);
    size_t input_size = total_size * sizeof(float);

    std::vector<float> input(total_size, 0);
    for (int i = 0; i < 16; i++) {
        input[i] = (rank_id + 10);
    }

    void *dev_ptr;
    ASSERT_EQ(aclrtMalloc(&dev_ptr, input_size, ACL_MEM_MALLOC_NORMAL_ONLY), 0);

    ASSERT_EQ(aclrtMemcpy(dev_ptr, input_size, input.data(), input_size, ACL_MEMCPY_HOST_TO_DEVICE), 0);

    void *ptr = shmem_malloc(1024);
    host_test_putmem(ptr, dev_ptr, rank_id, input_size);
    ASSERT_EQ(aclrtSynchronizeStream(shm::g_state_host.default_stream), 0);
    sleep(2);

    ASSERT_EQ(aclrtMemcpy(input.data(), input_size, ptr, input_size, ACL_MEMCPY_DEVICE_TO_HOST), 0);

    string p_name = "[Process " + to_string(rank_id) + "] ";
    std::cout << "After putmem:" << p_name;
    for (int i = 0; i < total_size; i++) {
        std::cout << input[i] << " ";
    }
    std::cout << std::endl;
    host_test_getmem<float>((float*)ptr, (float*)dev_ptr, rank_id, 16, rank_size);
    ASSERT_EQ(aclrtSynchronizeStream(shm::g_state_host.default_stream), 0);
    sleep(2);

    ASSERT_EQ(aclrtMemcpy(input.data(), input_size, dev_ptr, input_size, ACL_MEMCPY_DEVICE_TO_HOST), 0);

    std::cout << "After getmem:" << p_name;
    for (int i = 0; i < total_size; i++) {
        std::cout << input[i] << " ";
    }
    std::cout << std::endl;
    // for gtest
    int32_t flag = 0;
    for (int i = 0; i < total_size; i++) {
        int stage = i / 16;
        if (input[i] != (stage + 10)) {
            flag = 1;
        }
    }
    ASSERT_EQ(flag, 0);
}


void test_host_shmem_putmem_and_getmem(int rank_id, int n_ranks, uint64_t local_mem_size)
{
    int32_t device_id = rank_id % test_gnpu_num + test_first_npu;
    aclrtStream stream;
    test_init(rank_id, n_ranks, local_mem_size, &stream);
    ASSERT_NE(stream, nullptr);

    host_test_put_get_mem(rank_id, n_ranks, local_mem_size);
    std::cout << "[TEST] begin to exit...... rank_id: " << rank_id << std::endl;
    test_finalize(stream, device_id);
    if (::testing::Test::HasFailure()) {
        exit(1);
    }
}

void host_test_int32_g_and_p(int rank_id, int n_ranks, uint64_t local_mem_size)
{
    int total_size = static_cast<int>(n_ranks);
    size_t input_size = total_size * sizeof(int);

    int *ptr = static_cast<int*>(shmem_malloc(1024));
    shmem_int32_p(ptr, rank_id + 10, rank_id);
    ASSERT_EQ(aclrtSynchronizeStream(shm::g_state_host.default_stream), 0);
    sleep(2);

    int msg;
    ASSERT_EQ(aclrtMemcpy(&msg, input_size, ptr, input_size, ACL_MEMCPY_DEVICE_TO_HOST), 0);

    string p_name = "[Process " + to_string(rank_id) + "] ";
    std::cout << p_name << ", putValue is " << msg << std::endl;
    ASSERT_EQ(msg, rank_id + 10);

    for (int i = 0; i < n_ranks; ++i) {
        int getValue = shmem_int32_g(ptr, i);
        std::cout << p_name << ", getValue is " << getValue << std::endl;
        ASSERT_EQ(getValue, i + 10);
    }
}

void test_host_shmem_int32_p_and_g(int rank_id, int n_ranks, uint64_t local_mem_size)
{
    int32_t device_id = rank_id % test_gnpu_num + test_first_npu;
    aclrtStream stream;
    test_init(rank_id, n_ranks, local_mem_size, &stream);
    ASSERT_NE(stream, nullptr);

    host_test_int32_g_and_p(rank_id, n_ranks, local_mem_size);
    std::cout << "[TEST] begin to exit...... rank_id: " << rank_id << std::endl;
    test_finalize(stream, device_id);
    if (::testing::Test::HasFailure()) {
        exit(1);
    }
}

TEST(TestMemHostApi, TestShmemMemGetAndPutMem)
{
    const int process_count = test_gnpu_num;
    uint64_t local_mem_size = 1024UL * 1024UL * 1024;
    test_mutil_task(
            [this](int rank_id, int n_ranks, uint64_t local_mem_size) {
                test_host_shmem_putmem_and_getmem(rank_id, n_ranks, local_mem_size);
            }, local_mem_size, process_count);
}

TEST(TestMemHostApi, TestShmemMemGetAndPutInt)
{
    const int process_count = test_gnpu_num;
    uint64_t local_mem_size = 1024UL * 1024UL * 1024;
    test_mutil_task(
            [this](int rank_id, int n_ranks, uint64_t local_mem_size) {
                test_host_shmem_int32_p_and_g(rank_id, n_ranks, local_mem_size);
            }, local_mem_size, process_count);
}