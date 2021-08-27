#ifndef psd_funcs_h
#define psd_funcs_h

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pbc/pbc.h>

void psd_permute(unsigned int *array, size_t n, unsigned int seed, unsigned int N);
size_t str_to_int(unsigned char *ptr);
int sizefile(unsigned char *file_name);
int pairing_init(pairing_t pairing, unsigned char *pairing_param_file_name);
void write_str_to_file(unsigned char *_str, size_t _size, unsigned char *file_name, FILE *fp);
void read_str_from_file(unsigned char *_str, size_t _size, unsigned char *file_name, FILE *fp);
#endif /* psd_funcs_h */