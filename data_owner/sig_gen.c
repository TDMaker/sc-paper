#include <time.h>
#include "../utils/randys.h"
#include "../utils/sha256.h"
#define PAIRING_PARAM_FILE_PATH "../public_params/a.param"
#define STORAGE_LOCATION "../storage_provider/storage"
#define BLK_PATH STORAGE_LOCATION "/blks/%s_blk_%ld"
#define SIG_PATH STORAGE_LOCATION "/sigs/%s_sig_%ld"
#define SECRET_KEY_PATH "./secret.key"
#define ELEMENT_H_PATH "../public_params/h"
#define ELEMENT_NAME_PATH "../public_params/"
#define SOURCED_FILE_PATH "./the-sun-also-rises.mp3"
#define BLKING_INFO_PATH "../swap_zone/%s.info"
#define DEFAULT_BLK_SIZE 4096

int main(int argc, char *argv[])
{
    clock_t start, finish;
    size_t block_size = DEFAULT_BLK_SIZE;
    if (argc >= 3)
        block_size = str_to_int(argv[2]);
    pairing_t pairing;
    element_t h, secret_key, name;
    FILE *fp = NULL;

    if (pairing_init(pairing, PAIRING_PARAM_FILE_PATH) < 0)
        exit(0);
    element_init_G1(h, pairing);
    element_init_Zr(name, pairing);
    element_init_Zr(secret_key, pairing);

    element_random(name);

    int g1_size = pairing_length_in_bytes_compressed_G1(pairing);
    int zr_size = pairing_length_in_bytes_Zr(pairing);

    unsigned char *h_str = pbc_malloc(g1_size);
    read_str_from_file(h_str, g1_size, ELEMENT_H_PATH, fp);
    element_from_bytes_compressed(h, h_str);

    unsigned char *secret_key_str = pbc_malloc(zr_size);
    read_str_from_file(secret_key_str, zr_size, SECRET_KEY_PATH, fp);
    element_from_bytes(secret_key, secret_key_str);

    unsigned char *name_str = pbc_malloc(zr_size);
    element_to_bytes(name_str, name);
    unsigned char name_hr[64] = {0}; // hr means human read
    element_snprintf(name_hr, sizeof(name_hr), "%B", name);
    unsigned char write_path[100] = {0};
    sprintf(write_path, "%s%s", ELEMENT_NAME_PATH, name_hr);
    write_str_to_file(name_str, zr_size, write_path, fp);

    if ((fp = fopen(argc >= 3 ? argv[1] : SOURCED_FILE_PATH, "r")) == NULL)
    {
        printf("Fail to open the sourced file!\n");
        exit(0);
    }

    unsigned char hash_val[32] = {0};
    unsigned char hash_key[1024] = {0};
    unsigned char *file_block = malloc(block_size);

    element_t sigma_i, b_i, left_p, right_p;
    /**
     * sigma_i = (H(name||i)*h^{b_i})^primary_key
     * where left_p = H(name||i)
     *  and right_p = h^{b_i}
     *  sigma_i is reused as the base in the power calculation
     */

    element_init_Zr(b_i, pairing);
    element_init_G1(left_p, pairing);
    element_init_G1(right_p, pairing);
    element_init_G1(sigma_i, pairing);
    unsigned char *sigma_str = pbc_malloc(g1_size);

    unsigned char file_write_name[128];
    FILE *fp_write = NULL;
    size_t block_real_size = 0;

    memcpy(hash_key, name_hr, sizeof(name_hr));
    size_t seq_pos = strlen(hash_key);
    size_t index = 0;

    start = clock();
    while ((block_real_size = fread(file_block, 1, block_size, fp)) > 0)
    {
        sprintf(hash_key + seq_pos, "%ld", index);    // name||i
        sha256(hash_key, strlen(hash_key), hash_val); // H(name||i)
        element_from_hash(left_p, hash_val, 32);
        element_from_bytes(b_i, file_block);
        element_pow_zn(right_p, h, b_i);       // h^{b_i}
        element_mul(sigma_i, left_p, right_p); // base
        element_pow_zn(sigma_i, sigma_i, secret_key);
        element_to_bytes_compressed(sigma_str, sigma_i);

        sprintf(file_write_name, BLK_PATH, name_hr, index);
        write_str_to_file(file_block, block_real_size, file_write_name, fp_write);

        sprintf(file_write_name, SIG_PATH, name_hr, index);
        write_str_to_file(sigma_str, g1_size, file_write_name, fp_write);

        index++;
    }
    finish = clock();
    // printf("Total %ld chunks generated\n", count);
    // memset(hash_key, 0, 1024);
    unsigned char blking_info[32] = {0};
    sprintf(blking_info, "%ld", index - 1);
    sprintf(blking_info + sizeof(blking_info) / 2, "%ld", block_size);
    memset(file_write_name, 0, sizeof(file_write_name));
    sprintf(file_write_name, BLKING_INFO_PATH, name_hr);
    write_str_to_file(blking_info, sizeof(blking_info), file_write_name, fp);

    fclose(fp);
    free(file_block);

    pbc_free(sigma_str);
    pbc_free(h_str);
    pbc_free(secret_key_str);
    pbc_free(name_str);

    element_clear(h);
    element_clear(secret_key);
    element_clear(name);
    element_clear(sigma_i);
    element_clear(b_i);
    element_clear(left_p);
    element_clear(right_p);
    pairing_clear(pairing);

    printf("%f seconds\n", (double)(finish - start) / CLOCKS_PER_SEC);
    return 0;
}