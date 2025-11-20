#ifndef UNITTEST_H
#define UNITTEST_H

extern int test_global_ranks;
extern int test_gnpu_num;
extern const char* test_global_ipport;
extern int test_first_rank;
extern int test_first_npu;

void test_init(int rank_id, int n_ranks, uint64_t local_mem_size, aclrtStream *st);
void test_finalize(aclrtStream stream, int device_id);
void test_mutil_task(std::function<void(int, int, uint64_t)> func, uint64_t local_mem_size, int process_count);

#endif // UNITTEST_H