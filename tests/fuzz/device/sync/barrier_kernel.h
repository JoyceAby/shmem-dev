#ifndef BARRIER_KERNEL_H
#define BARRIER_KERNEL_H

void increase_do(void* stream, uint64_t config, uint8_t *addr, int rank_id, int rank_size);
void increase_vec_do(void* stream, uint64_t config, uint8_t *addr, int rank_id, int rank_size);
void increase_do_odd_team(void* stream, uint64_t config, uint8_t *addr, int rank_id, int rank_size, shmem_team_t team_id);
void increase_vec_do_odd_team(void* stream, uint64_t config, uint8_t *addr, int rank_id, int rank_size, shmem_team_t team_id);

#endif // BARRIER_KERNEL_H