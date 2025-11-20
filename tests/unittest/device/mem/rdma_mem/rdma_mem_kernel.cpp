#include "kernel_operator.h"

#include "shmem_api.h"
constexpr uint64_t MESSAGE_SIZE = 64;

extern "C" __global__ __aicore__ void RDMAGetTestLowLevel(GM_ADDR gva, uint64_t config)
{
    shmemx_set_ffts_config(config);
    AscendC::TPipe pipe;
    AscendC::TBuf<AscendC::TPosition::VECOUT> buf;
    pipe.InitBuffer(buf, UB_ALIGN_SIZE * 2);
    AscendC::LocalTensor<uint8_t> ubLocal = buf.GetWithOffset<uint8_t>(UB_ALIGN_SIZE * 2, 0);

    int64_t rank = smem_shm_get_global_rank();
    int64_t rank_size = smem_shm_get_global_rank_size();
    GM_ADDR dest_addr;

    for (int64_t peer = 0; peer < rank_size; peer++) {
        if (peer == rank) {
            continue;
        }
        dest_addr = gva + peer * MESSAGE_SIZE;
        shmem_roce_get_mem_nbi(dest_addr, dest_addr, (__ubuf__ uint8_t*)ubLocal.GetPhyAddr(), MESSAGE_SIZE, peer);
    }
}

void test_rdma_get_low_level(uint32_t block_dim, void* stream, uint8_t* gva, uint64_t config)
{
    RDMAGetTestLowLevel<<<block_dim, nullptr, stream>>>(gva, config);
}

extern "C" __global__ __aicore__ void RDMAPutTestLowLevel(GM_ADDR gva, uint64_t config)
{
    shmemx_set_ffts_config(config);
    AscendC::TPipe pipe;
    AscendC::TBuf<AscendC::TPosition::VECOUT> buf;
    pipe.InitBuffer(buf, UB_ALIGN_SIZE * 2);
    AscendC::LocalTensor<uint8_t> ubLocal = buf.GetWithOffset<uint8_t>(UB_ALIGN_SIZE * 2, 0);

    int64_t rank = smem_shm_get_global_rank();
    int64_t rank_size = smem_shm_get_global_rank_size();
    GM_ADDR src_addr;

    for (int64_t peer = 0; peer < rank_size; peer++) {
        if (peer == rank) {
            continue;
        }
        src_addr = gva + rank * MESSAGE_SIZE;
        shmem_roce_put_mem_nbi(src_addr, src_addr, (__ubuf__ uint8_t*)ubLocal.GetPhyAddr(), MESSAGE_SIZE, peer);
    }
}

void test_rdma_put_low_level(uint32_t block_dim, void* stream, uint8_t* gva, uint64_t config)
{
    RDMAPutTestLowLevel<<<block_dim, nullptr, stream>>>(gva, config);
}

extern "C" __global__ __aicore__ void RDMAGetTestHighLevel(GM_ADDR gva, uint64_t config)
{
    shmemx_set_ffts_config(config);
    int64_t rank = smem_shm_get_global_rank();
    int64_t rank_size = smem_shm_get_global_rank_size();
    GM_ADDR dest_addr;

    for (int64_t peer = 0; peer < rank_size; peer++) {
        if (peer == rank) {
            continue;
        }
        dest_addr = gva + peer * MESSAGE_SIZE;
        shmem_get_uint8_mem_nbi(dest_addr, dest_addr, MESSAGE_SIZE, peer);
    }
}

void test_rdma_get_high_level(uint32_t block_dim, void* stream, uint8_t* gva, uint64_t config)
{
    RDMAGetTestHighLevel<<<block_dim, nullptr, stream>>>(gva, config);
}

extern "C" __global__ __aicore__ void RDMAPutTestHighLevel(GM_ADDR gva, uint64_t config)
{
    shmemx_set_ffts_config(config);
    int64_t rank = smem_shm_get_global_rank();
    int64_t rank_size = smem_shm_get_global_rank_size();
    GM_ADDR src_addr;

    for (int64_t peer = 0; peer < rank_size; peer++) {
        if (peer == rank) {
            continue;
        }
        src_addr = gva + rank * MESSAGE_SIZE;
        shmem_put_uint8_mem_nbi(src_addr, src_addr, MESSAGE_SIZE, peer);
    }
}

void test_rdma_put_high_level(uint32_t block_dim, void* stream, uint8_t* gva, uint64_t config)
{
    RDMAPutTestHighLevel<<<block_dim, nullptr, stream>>>(gva, config);
}