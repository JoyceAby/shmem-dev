#ifndef TEAM_KERNEL_H
#define TEAM_KERNEL_H

void get_device_state(uint32_t block_dim, void* stream, uint8_t* gva, shmem_team_t team_id);

#endif