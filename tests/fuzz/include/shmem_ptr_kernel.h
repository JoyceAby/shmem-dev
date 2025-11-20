#ifndef SHMEM_PTR_KERNEL_H
#define SHMEM_PTR_KERNEL_H

void get_device_ptr(uint32_t block_dim, void* stream, uint8_t* gva);

#endif // SHMEM_PTR_KERNEL_H