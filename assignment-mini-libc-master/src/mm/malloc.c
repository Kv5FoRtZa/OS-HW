// SPDX-License-Identifier: BSD-3-Clause

#include <internal/mm/mem_list.h>
#include <internal/types.h>
#include <internal/essentials.h>
#include <sys/mman.h>
#include <string.h>
#include <stdlib.h>

void *malloc(size_t size)
{
	if(size < 1)
	{
		return NULL;
	}
	void *memorie =  mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if(memorie == MAP_FAILED)
	{
		return NULL;
	} else {
		mem_list_add(memorie, size);
		return memorie;
	}
}

void *calloc(size_t nmemb, size_t size)
{
	int size_real = size * nmemb;
	void *memorie = malloc(size_real);
	//trebuie alocat cu 0?
	for(int i = 0; i < size_real; i ++)
	{
		//memorie[i] = 0;
	}
	return memorie;
}

void free(void *ptr)
{
	struct mem_list *memorie_de_eliberat = mem_list_find(ptr);
	munmap(ptr, memorie_de_eliberat->len);
	mem_list_del(ptr);
}

void *realloc(void *ptr, size_t size)
{
	if(ptr == NULL)
	{
		return NULL;
	}
	long *memorie_veche = ptr;
	long lugime = sizeof(memorie_veche);
	void *memorie = mremap(ptr, lugime, size, MREMAP_MAYMOVE);
	if(memorie == MAP_FAILED)
	{
		return NULL;
	}
	return memorie;
}

void *reallocarray(void *ptr, size_t nmemb, size_t size)
{
	int lungime = size * nmemb;
	return realloc(ptr, lungime);;
}
