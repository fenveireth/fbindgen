// Must translate size_t as usize, not completely resolve to u64

#include <sys/types.h>

void* mmap(void* addr, size_t length, int prot, int flags, int fd, off_t offset);