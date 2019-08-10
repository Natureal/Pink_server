#ifndef PINK_MEM_POOL_H
#define PINK_MEM_POOL_H

#include <cstddef>

class pink_mem_pool{
private:
	union obj{
		union obj *free_list_link;
		char data[1];
	}
	enum {__ALIGN = 8};
	enum {__MAX_BYTES = 128};
	enum {__NFREELISTS = __MAX_BYTES / __ALIGN};
private:
	static size_t ROUND_UP(size_t bytes);
	static size_t FREE_INDEX(size_t bytes);
	static char* chunk_alloc(size_t size, int &nobjs);
public:
	static void* allocate(size_t n);
	static void* deallocate(void *p, size_t n);
private:
	// 16 free lists
	static obj* volatile free_list[__NFREELISTS];
	static char *start_free;
	static char *end_free;
}

char* pink_mem_pool::start_free = 0;
char* pink_mem_pool::end_free = 0;

pink_mem_pool::obj* volatile pink_mem_pool::free_list[__NFREELISTS] = {0};

size_t pink_mem_pool::ROUND_UP(size_t bytes){
	return ((bytes) + __ALIGN - 1) & ~(__ALIGN - 1);
}

size_t pink_mem_pool::FREE_INDEX(size_t bytes){
	return (((bytes) + __ALIGN - 1) / __ALIGN - 1);
}

void* pink_mem_pool::allocate(size_t n){
	
}

void* pink_mem_pool::deallocate(void *p, size_t n){

}

char* pink_mem_pool::chunk_alloc(size_t size, int &nobjs){
	
}



#endif