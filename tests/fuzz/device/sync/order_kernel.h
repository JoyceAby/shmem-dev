#ifndef ORDER_KERNEL_H
#define ORDER_KERNEL_H

void quiet_order_do(void* stream, uint64_t config, uint8_t *addr, int32_t rank_id, int32_t n_ranks);
void fence_order_do(void* stream, uint64_t config, uint8_t *addr, int32_t rank_id, int32_t n_ranks);

#endif // ORDER_KERNEL_H