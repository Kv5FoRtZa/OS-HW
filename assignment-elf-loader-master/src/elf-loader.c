// SPDX-License-Identifier: BSD-3-Clause

#define _GNU_SOURCE
#include <elf.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>
void *map_elf(const char *filename)
{
	// This part helps you store the content of the ELF file inside the buffer.
	struct stat st;
	void *file;
	int fd;

	fd = open(filename, O_RDONLY);
	if (fd < 0) {
		perror("open");
		exit(1);
	}

	fstat(fd, &st);

	file = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (file == MAP_FAILED) {
		perror("mmap");
		close(fd);
		exit(1);
	}

	return file;
}

void load_and_run(const char *filename, int argc, char **argv, char **envp)
{
	void *sp = NULL;

	sp = mmap((void *)0x700000000000, 1024 * 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, -1, 0);
	char *stiva = (char *)sp + 1024 * 4096;
	// stiva e pusa la o adresa mare pentru a nu suprascrie alte chestii utile(nu mai da punctajul la primele daca nu)
	void *elf_contents = map_elf(filename);
	char *vrf = (char *)elf_contents;
	int elf_magic_vf = 0;

	if (vrf[0] == 0x7f && vrf[1] == 0x45 && vrf[2] == 0x4c && vrf[3] == 0x46)
		elf_magic_vf = 1;
	if (!elf_magic_vf) {
		fprintf(stderr, "Not a valid ELF file");
		exit(3);
	}
	if (vrf[4] != 0x02) {
		fprintf(stderr, "Not a 64-bit ELF");
		exit(4);
	}
	int nr_of_headers, size_of_header;
	//aici imi declar pointerul ca ELF64, dupa ce am verificat ca e un elf 64
	Elf64_Phdr  *file = (Elf64_Phdr  *)elf_contents;
	Elf64_Ehdr  *E_file = (Elf64_Ehdr  *)elf_contents;
	unsigned long long load_base = 0;

	if (E_file->e_type == ET_DYN)
		load_base = 0x555555554000;
	//load_base o alta adresa mare pt punctul 5
	nr_of_headers = E_file->e_phnum;
	//nr de headere printre care se afla PT_LOAD
	Elf64_Phdr  *start = (Elf64_Phdr  *)((char *)elf_contents + E_file->e_phoff);

	size_of_header = E_file->e_ehsize;
	for (int i = 0; i < nr_of_headers; i++) {
		if (start->p_type == 1) {
			// 1 e ptload
			//lungime mare e pt bss
			//bss nu intra in filesz pt ca dc ai scrie niste 0 in plus
			size_t  lungime = start->p_filesz;
			size_t  lungime_mare = start->p_memsz;
			size_t  offset = start->p_offset;
			uintptr_t  map_start = (start->p_vaddr + load_base) & ~(uintptr_t)(4096 - 1);
			// se mapeaza in functie de marimea unei pagini, nu mai da stiva daca am acelasi lucru intre pagini
			void *adresa_destinatie = (void *)map_start;
			size_t actually_lungime_page_stuff = lungime_mare + ((start->p_vaddr + load_base) - map_start);
			size_t  permisiuni = start->p_flags;
			void *memorie_mapata;

			memorie_mapata = mmap(adresa_destinatie, actually_lungime_page_stuff, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
			memcpy((char *)memorie_mapata + ((start->p_vaddr + load_base) - map_start), (char *)elf_contents + offset, lungime);
			if (lungime_mare > lungime) {
				memset((char *) (start->p_vaddr + load_base) + lungime, 0, lungime_mare - lungime);
				//bag 0 de la lungime la lungime mare
				//pt ca daca exista bss, atunci e nevoie de niste 0 in plus
			}
			if (permisiuni == 1)
				mprotect(memorie_mapata, lungime_mare, PROT_EXEC);
			else if (permisiuni == 2)
				mprotect(memorie_mapata, lungime_mare, PROT_WRITE);
			else if (permisiuni == 3)
				mprotect(memorie_mapata, lungime_mare, PROT_WRITE | PROT_EXEC);
			else if (permisiuni == 4)
				mprotect(memorie_mapata, lungime_mare, PROT_READ);
			else if (permisiuni == 5)
				mprotect(memorie_mapata, lungime_mare, PROT_READ | PROT_EXEC);
			else if (permisiuni == 6)
				mprotect(memorie_mapata, lungime_mare, PROT_WRITE | PROT_READ);
			else if (permisiuni == 7)
				mprotect(memorie_mapata, lungime_mare, PROT_READ | PROT_WRITE | PROT_EXEC);
		}
		start++;
	}
	unsigned long long nr, i = 0;

	//if (E_file->e_type == ET_EXEC) {
	//pun tot pe stiva stringurile envp si argv
	// le salvez adresele si apoi pun adresele pe stiva in ordine
	//same pt random si executabil
		//aici intra envp
		while (envp[i] != NULL)
			i++;
		uintptr_t *adrese_envp = malloc(i * sizeof(uintptr_t));
		uintptr_t *adrese_argv = malloc(argc * sizeof(uintptr_t));

		i--;
		for (int j = i; j >= 0; j--) {
			int lung = strlen(envp[j]) + 1;

			stiva -= lung;
			adrese_envp[j] = (uintptr_t)stiva;
			memcpy(stiva, envp[j], lung);
		}
		// argv
		for (int j = argc - 1; j >= 0; j--) {
			int lung = strlen(argv[j]) + 1;

			stiva -= lung;
			adrese_argv[j] = (uintptr_t)stiva;
			memcpy(stiva, argv[j], lung);
		}
		//exec_nume
		int lungime_filename = strlen(filename) + 1;

		stiva -= lungime_filename;
		uintptr_t adresa_exec = (uintptr_t)stiva;

		memcpy(stiva, filename, lungime_filename);
		//random
		stiva -= 16;
		unsigned char random[16];

		for (int i = 0; i < 16; i++)
			random[i] = i + 34;
		memcpy(stiva, random, 16);
		uintptr_t aliniere = (uintptr_t)stiva;

		aliniere &= ~(0xF);
		stiva = (char *)aliniere;
		uintptr_t adresa_random = (uintptr_t)stiva;
		//null after auxv
		stiva -= sizeof(unsigned long long);
		nr = 0;
		memcpy(stiva, &nr, sizeof(unsigned long long));
		stiva -= sizeof(unsigned long long);
		nr = AT_NULL;
		memcpy(stiva, &nr, sizeof(unsigned long long));
		//pagesize
		nr = 4096;
		stiva -= sizeof(unsigned long long);
		memcpy(stiva, &nr, sizeof(unsigned long long));
		stiva -= sizeof(unsigned long long);
		nr = AT_PAGESZ;
		memcpy(stiva, &nr, sizeof(unsigned long long));
		//ecex
		stiva -= sizeof(unsigned long long);
		nr = adresa_exec;
		memcpy(stiva, &nr, sizeof(unsigned long long));
		stiva -= sizeof(unsigned long long);
		nr = AT_EXECFN;
		memcpy(stiva, &nr, sizeof(unsigned long long));
		//random
		stiva -= sizeof(unsigned long long);
		nr = adresa_random;
		memcpy(stiva, &nr, sizeof(unsigned long long));
		stiva -= sizeof(unsigned long long);
		nr = AT_RANDOM;
		memcpy(stiva, &nr, sizeof(unsigned long long));
		//entry
		nr = E_file->e_entry + load_base;
		stiva -= sizeof(unsigned long long);
		memcpy(stiva, &nr, sizeof(unsigned long long));
		stiva -= sizeof(unsigned long long);
		nr = AT_ENTRY;
		memcpy(stiva, &nr, sizeof(unsigned long long));
		//phnum
		stiva -= sizeof(unsigned long long);
		nr = E_file->e_phnum;
		memcpy(stiva, &nr, sizeof(unsigned long long));
		stiva -= sizeof(unsigned long long);
		nr = AT_PHNUM;
		memcpy(stiva, &nr, sizeof(unsigned long long));
		//phent
		stiva -= sizeof(unsigned long long);
		nr = E_file->e_phentsize;
		memcpy(stiva, &nr, sizeof(unsigned long long));
		stiva -= sizeof(unsigned long long);
		nr = AT_PHENT;
		memcpy(stiva, &nr, sizeof(unsigned long long));
		//phdr wrong
		stiva -= sizeof(unsigned long long);
		nr = (uintptr_t)((char *)E_file + E_file->e_phoff);
		memcpy(stiva, &nr, sizeof(unsigned long long));
		stiva -= sizeof(unsigned long long);
		nr = AT_PHDR;
		memcpy(stiva, &nr, sizeof(unsigned long long));
		//null after envp
		stiva -= sizeof(unsigned long long);
		nr = 0;
		memcpy(stiva, &nr, sizeof(unsigned long long));
		//envp adr
		for (int j = i; j >= 0; j--) {
			stiva -= sizeof(unsigned long long);
			nr = adrese_envp[j];
			memcpy(stiva, &nr, sizeof(unsigned long long));
		}
		//null after argv
		stiva -= sizeof(unsigned long long);
		nr = 0;
		memcpy(stiva, &nr, sizeof(unsigned long long));
		//argv adr
		for (int j = argc - 1; j >= 0; j--) {
			stiva -= sizeof(unsigned long long);
			nr = adrese_argv[j];
			memcpy(stiva, &nr, sizeof(unsigned long long));
		}
		stiva = (char *)((uintptr_t)stiva & ~0xF);
		stiva -= 8;
		//argc
		unsigned long long nr2;

		nr2 = argc;
		memcpy(stiva, &nr2, sizeof(unsigned long long));
	//}
	 void (*entry)() = (void(*)())(E_file->e_entry + load_base);

	// Transfer control
	sp = stiva;
	__asm__ __volatile__(
			"mov %0, %%rsp\n"
			"xor %%rbp, %%rbp\n"
			"jmp *%1\n"
			:
			: "r"(sp), "r"(entry)
			: "memory"
			);
}

int main(int argc, char **argv, char **envp)
{
	if (argc < 2) {
		fprintf(stderr, "Usage: %s <static-elf-binary>\n", argv[0]);
		exit(1);
	}

	load_and_run(argv[1], argc - 1, &argv[1], envp);
	return 0;
}
