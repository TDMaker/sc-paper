#include <time.h>
#include "../utils/sha256.h"
#include "../utils/randys.h"
#define PAIRING_PARAM_FILE_PATH "../public_params/a.param"
#define STORAGE_LOCATION "../storage_provider/storage"
#define BLK_PATH STORAGE_LOCATION "/blks/%s_blk_%ld"
#define SIG_PATH STORAGE_LOCATION "/sigs/%s_sig_%ld"
#define ELEMENT_G_PATH "../public_params/g"
#define ELEMENT_H_PATH "../public_params/h"
#define ELEMENT_NAME_PATH "../public_params/"
#define PUBLIC_KEY_PATH "../public_params/public.key"
#define BLKING_INFO_PATH "../swap_zone/%s.info"

int main(int argc, char *argv[])
{
    clock_t start, finish;
    pairing_t pairing;
    element_t g, h, public_key, name;
    FILE *fp = NULL;
    if (argc < 2)
    {
        puts("Input the file name to be verified.");
        exit(0);
    }
    if (pairing_init(pairing, PAIRING_PARAM_FILE_PATH) < 0)
        exit(0);
    element_init_G1(h, pairing);
    element_init_G2(g, pairing);
    element_init_G2(public_key, pairing);
    element_init_Zr(name, pairing);

    int zr_size = pairing_length_in_bytes_Zr(pairing);
    int g1_size = pairing_length_in_bytes_compressed_G1(pairing);
    int g2_size = pairing_length_in_bytes_compressed_G2(pairing);

    unsigned char read_path[64] = {0};
    sprintf(read_path, "%s%s", ELEMENT_NAME_PATH, argv[1]);
    unsigned char *name_str = pbc_malloc(zr_size);
    read_str_from_file(name_str, zr_size, read_path, fp);
    element_from_bytes(name, name_str);

    unsigned char name_hr[64] = {0};
    element_snprintf(name_hr, sizeof(name_hr), "%B", name);

    unsigned char *g1_str = pbc_malloc(g1_size);
    read_str_from_file(g1_str, g1_size, ELEMENT_H_PATH, fp);
    element_from_bytes_compressed(h, g1_str);

    unsigned char *g2_str = pbc_malloc(g2_size);
    read_str_from_file(g2_str, g2_size, ELEMENT_G_PATH, fp);
    element_from_bytes_compressed(g, g2_str);

    read_str_from_file(g2_str, g2_size, PUBLIC_KEY_PATH, fp);
    element_from_bytes_compressed(public_key, g2_str);

    FILE *fp_blk, *fp_sig;
    unsigned char hash_val[32] = {0};
    unsigned char hash_key[1024] = {0};
    unsigned char fname_blk[128] = {0};
    unsigned char fname_sig[128] = {0};

    unsigned char *sig_str = pbc_malloc(g1_size);
    //  sprintf(fname_blk, "%s/blks/%s_blk_%d", STORAGE_LOCATION, name_hr, 0);
    sprintf(fname_blk, BLKING_INFO_PATH, name_hr);
    unsigned char buf[32] = {0};
    read_str_from_file(buf, sizeof(buf), fname_blk, fp);
    size_t blk_total = str_to_int(buf);
    size_t blk_size = str_to_int(buf + sizeof(buf) / 2);

    unsigned char *block_buf = malloc(blk_size);
    memcpy(hash_key, name_hr, sizeof(name_hr));
    size_t seq_pos = strlen(name_hr);

    element_t sigma_i, b_i, left_p, right_p, base, temp1, temp2;
    element_init_Zr(b_i, pairing);
    element_init_G1(base, pairing);
    element_init_G1(sigma_i, pairing);
    element_init_G1(left_p, pairing); // left_p is reused
    element_init_G1(right_p, pairing);
    element_init_GT(temp1, pairing);
    element_init_GT(temp2, pairing);

    int is_valid = 1;
    start = clock();
    for (size_t index = 0; index < blk_total; index++)
    {
        sprintf(fname_blk, BLK_PATH, name_hr, index);
        sprintf(fname_sig, SIG_PATH, name_hr, index);

        read_str_from_file(block_buf, blk_size, fname_blk, fp_blk);
        read_str_from_file(sig_str, g1_size, fname_sig, fp_sig);

        sprintf(hash_key + seq_pos, "%ld", index);
        sha256(hash_key, strlen(hash_key), hash_val);

        element_from_bytes_compressed(sigma_i, sig_str);
        element_pairing(temp1, sigma_i, g);

        element_from_bytes(b_i, block_buf);
        element_from_hash(left_p, hash_val, sizeof(hash_val));
        element_pow_zn(right_p, h, b_i);
        element_mul(left_p, left_p, right_p);
        element_pairing(temp2, left_p, public_key);
        if (element_cmp(temp1, temp2))
        {
            printf("*FAILED* Tag verify failed on block %ld *FAILED*\n", index);
            is_valid = 0;
        }
    }
    finish = clock();
    printf("Tag %sverifies.\n", is_valid ? "" : "not ");

    pbc_free(name_str);
    pbc_free(g1_str);
    pbc_free(g2_str);
    pbc_free(sig_str);
    free(block_buf);
    element_clear(h);
    element_clear(g);
    element_clear(public_key);
    element_clear(name);
    pairing_clear(pairing);

    printf("%f seconds\n", (double)(finish - start) / CLOCKS_PER_SEC);
    return 0;
}
