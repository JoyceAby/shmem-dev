#undef L2_CACHE_HINT
#include "kernel_operator.h"

#include "shmem_api.h"

const int length = 16;
const int ub_size = 256;

class kernel_shmemx_mte_put_num {
public:
    __aicore__ inline kernel_shmemx_mte_put_num() {}
    __aicore__ inline void Init(GM_ADDR gva, GM_ADDR dev)
    {
        gva_gm = (__gm__ int8_t *)gva;
        dev_gm = (__gm__ int8_t *)dev;

        /* set GM Buffer */
        src_gm.SetGlobalBuffer(dev_gm);
        dst_gm.SetGlobalBuffer(gva_gm);

        rank = shmem_my_pe();
        rank_size = shmem_n_pes();

        /* 1x4096 Bytes Buffer */
        pipe.InitBuffer(buf_queue, 1, 4096);
    }
    __aicore__ inline void Process(uint64_t config)
    {
        shmemx_set_ffts_config(config);
        AscendC::LocalTensor<int8_t> buf_tensor = buf_queue.AllocTensor<int8_t>();
        uintptr_t addr = static_cast<uintptr_t>(buf_tensor.address_.bufferAddr);
        __ubuf__ int8_t *buf = (__ubuf__ int8_t *)addr;
        shmem_mte_put_mem_nbi(gva_gm, dev_gm, buf, (uint32_t)ub_size, rank_size * length / 4, rank, EVENT_ID0);
        AscendC::SetFlag<AscendC::HardEvent::MTE3_MTE2>(EVENT_ID0);
        AscendC::WaitFlag<AscendC::HardEvent::MTE3_MTE2>(EVENT_ID0);
        shmem_mte_put_mem_nbi(dst_gm[rank_size * length / 4], src_gm[rank_size * length / 4], buf_tensor, rank_size * length / 4, rank, EVENT_ID0);
        AscendC::SetFlag<AscendC::HardEvent::MTE3_MTE2>(EVENT_ID0);
        AscendC::WaitFlag<AscendC::HardEvent::MTE3_MTE2>(EVENT_ID0);
        shmemx_mte_put_mem_nbi(gva_gm + rank_size * length / 2, dev_gm + rank_size * length / 2, rank_size * length / 4, rank, false);
        AscendC::SetFlag<AscendC::HardEvent::MTE3_MTE2>(EVENT_ID0);
        AscendC::WaitFlag<AscendC::HardEvent::MTE3_MTE2>(EVENT_ID0);
        shmemx_mte_put_mem_nbi(gva_gm + rank_size * length * 3 / 4, dev_gm + rank_size * length * 3 / 4, rank_size * length / 4, rank, false);
        shmemx_barrier_all_vec();
        buf_queue.FreeTensor(buf_tensor);
    }
private:
    AscendC::TPipe pipe;
    AscendC::TQue<AscendC::TPosition::VECIN, 2> buf_queue;

    __gm__ int8_t *gva_gm;
    __gm__ int8_t *dev_gm;
    AscendC::GlobalTensor<int8_t> src_gm, dst_gm;

    int64_t rank;
    int64_t rank_size;
};


extern "C" __global__ __aicore__ void put_shmemx_mte_num_test(GM_ADDR gva, GM_ADDR dev, uint64_t config)
{
    kernel_shmemx_mte_put_num op;
    op.Init(gva, dev);
    op.Process(config);
}

void test_shmemx_mte_put(uint32_t block_dim, void* stream, uint64_t config, uint8_t* gva, uint8_t* dev)
{
    put_shmemx_mte_num_test<<<block_dim, nullptr, stream>>>(gva, dev, config);
}


class kernel_shmemx_mte_get_num {
public:
    __aicore__ inline kernel_shmemx_mte_get_num() {}
    __aicore__ inline void Init(GM_ADDR gva, GM_ADDR dev)
    {
        gva_gm = (__gm__ int8_t *)gva;
        dev_gm = (__gm__ int8_t *)dev;

        /* set GM Buffer */                                                                                                             \
        src_gm.SetGlobalBuffer(gva_gm);                                                                                                 \
        dst_gm.SetGlobalBuffer(dev_gm);

        rank = shmem_my_pe();
        rank_size = shmem_n_pes();

        /* 1x4096 Bytes Buffer */
        pipe.InitBuffer(buf_queue, 1, 4096);
    }
    __aicore__ inline void Process(uint64_t config)
    {
        shmemx_set_ffts_config(config);
        AscendC::LocalTensor<int8_t> buf_tensor = buf_queue.AllocTensor<int8_t>();
        uintptr_t addr = static_cast<uintptr_t>(buf_tensor.address_.bufferAddr);
        __ubuf__ int8_t *buf = (__ubuf__ int8_t *)addr;

        for (int i = 0; i < rank_size / 2; i++) {
            shmem_mte_get_mem_nbi(dev_gm + length * i, gva_gm, buf, (uint32_t)ub_size, length / 2, i % rank_size, EVENT_ID0);
            AscendC::SetFlag<AscendC::HardEvent::MTE3_MTE2>(EVENT_ID0);
            AscendC::WaitFlag<AscendC::HardEvent::MTE3_MTE2>(EVENT_ID0);
            shmem_mte_get_mem_nbi(dst_gm[length * i + length / 2], src_gm, buf_tensor, length / 2, i % rank_size, EVENT_ID0);
            AscendC::SetFlag<AscendC::HardEvent::MTE3_MTE2>(EVENT_ID0);
            AscendC::WaitFlag<AscendC::HardEvent::MTE3_MTE2>(EVENT_ID0);
        }

        for (int i = rank_size / 2; i < rank_size; i++) {
            shmemx_mte_get_mem_nbi(dev_gm + length * i, gva_gm, length, i % rank_size, true);
            AscendC::SetFlag<AscendC::HardEvent::MTE3_MTE2>(EVENT_ID0);
            AscendC::WaitFlag<AscendC::HardEvent::MTE3_MTE2>(EVENT_ID0);
            shmemx_mte_get_mem_nbi(dev_gm + length * i + length / 2, gva_gm, length / 2, i % rank_size, true);
            AscendC::SetFlag<AscendC::HardEvent::MTE3_MTE2>(EVENT_ID0);
            AscendC::WaitFlag<AscendC::HardEvent::MTE3_MTE2>(EVENT_ID0);
        }

        shmemx_barrier_all_vec();
        buf_queue.FreeTensor(buf_tensor);
    }
private:
    AscendC::TPipe pipe;
    AscendC::TQue<AscendC::TPosition::VECIN, 2> buf_queue;
    __gm__ int8_t *gva_gm;
    __gm__ int8_t *dev_gm;
    AscendC::GlobalTensor<int8_t> src_gm, dst_gm;

    int64_t rank;
    int64_t rank_size;
};

extern "C" __global__ __aicore__ void get_shmemx_mte_num_test(GM_ADDR gva, GM_ADDR dev, uint64_t config)
{
    kernel_shmemx_mte_get_num op;
    op.Init(gva, dev);
    op.Process(config);
}

void test_shmemx_mte_get(uint32_t block_dim, void* stream, uint64_t config, uint8_t* gva, uint8_t* dev)
{
    get_shmemx_mte_num_test<<<block_dim, nullptr, stream>>>(gva, dev, config);
}
