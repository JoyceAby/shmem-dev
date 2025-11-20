#ifndef TEAM_ALLGATHER_KERNEL_H
#define TEAM_ALLGATHER_KERNEL_H

void team_allgather(uint32_t block_dim, void* stream, uint64_t config, uint8_t* gva, shmem_team_t team_id);

#endif // TEAM_ALLGATHER_KERNEL_H