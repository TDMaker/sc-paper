#include <time.h>
#include "../utils/sha256.h"
#include "../utils/randys.h"
#define PAIRING_PARAM_FILE_PATH "../public_params/a.param"
#define STORAGE_LOCATION "../storage_provider/storage"
#define BLK_PATH STORAGE_LOCATION "/blks/%s_blk_%d"
#define SIG_PATH STORAGE_LOCATION "/sigs/%s_sig_%d"
#define ELEMENT_NAME_PATH "../public_params/"
#define BLKING_INFO_PATH "../swap_zone/%s.info"
#define SUM_R_PATH "./data/%s/sum_r"
#define SUM_S_PATH "./data/%s/sum_s"
#define PROOF_MU_PATH "./data/%s/mu"
#define PROOF_SIGMA_PATH "./data/%s/sigma"

int main(int argc, char *argv[])
{
    if (argc < 4)
    {
        puts("Need two parameters of fileName, chaAmnt, contractAddr.");
        exit(0);
    }
    unsigned char *name_hr = argv[1];
    size_t chal_amount = str_to_int(argv[2]);
    unsigned char *sc_addr = argv[3];
    unsigned char buf[1024] = {0};
    unsigned char file_path_buf[128] = {0};
    clock_t start, finish;
    mpz_t sum_r_pz, sum_s_pz;
    pairing_t pairing;
    element_t name;
    FILE *fp = NULL;

    if (pairing_init(pairing, PAIRING_PARAM_FILE_PATH) < 0)
        exit(0);

    mpz_init(sum_r_pz);
    mpz_init(sum_s_pz);
    sprintf(file_path_buf, SUM_R_PATH, sc_addr);
    read_str_from_file(buf, sizeof(buf), file_path_buf, fp);
    mpz_get_str(buf, 10, sum_r_pz);

    memset(buf, 0, sizeof(buf));
    memset(file_path_buf, 0, sizeof(file_path_buf));
    sprintf(file_path_buf, SUM_S_PATH, sc_addr);
    read_str_from_file(buf, sizeof(buf), file_path_buf, fp);
    mpz_get_str(buf, 10, sum_s_pz);

    unsigned long int r_long = mpz_get_ui(sum_r_pz);
    unsigned long int s_long = mpz_get_ui(sum_s_pz);

    memset(buf, 0, sizeof(buf));
    sprintf(buf, "%s%s", ELEMENT_NAME_PATH, name_hr);
    int zr_size = pairing_length_in_bytes_Zr(pairing);
    unsigned char *name_str = pbc_malloc(zr_size);
    read_str_from_file(name_str, zr_size, buf, fp);
    element_init_Zr(name, pairing);
    element_from_bytes(name, name_str);

    memset(buf, 0, sizeof(buf));
    sprintf(buf, BLKING_INFO_PATH, name_hr);
    unsigned char info[32] = {0};
    read_str_from_file(info, sizeof(info), buf, fp);
    size_t blk_total = str_to_int(info);
    size_t blk_size = str_to_int(info + sizeof(info) / 2);

    unsigned int *u_s = (unsigned int *)malloc(sizeof(unsigned int) * chal_amount);
    psd_permute(u_s, chal_amount, (unsigned int)r_long, blk_total);
    pbc_random_set_deterministic(s_long);

    FILE *fp_blk, *fp_sig;
    unsigned char fname_blk[128] = {0};
    unsigned char fname_sig[128] = {0};

    element_t mu, prod, sigma, expon, b_i, sigma_i, v_i;
    element_init_Zr(mu, pairing);
    element_set0(mu);
    element_init_Zr(prod, pairing);
    element_init_G1(sigma, pairing);
    element_set1(sigma);
    element_init_G1(sigma_i, pairing);
    element_init_G1(expon, pairing);
    element_init_Zr(b_i, pairing);
    element_init_Zr(v_i, pairing);
    int g1_size = pairing_length_in_bytes_compressed_G1(pairing);
    unsigned char *block_buf = malloc(blk_size);
    unsigned char *sig_str = pbc_malloc(g1_size);
    start = clock();
    for (size_t i = 0; i < chal_amount; i++)
    {
        sprintf(fname_blk, BLK_PATH, name_hr, u_s[i]);
        sprintf(fname_sig, SIG_PATH, name_hr, u_s[i]);

        read_str_from_file(block_buf, blk_size, fname_blk, fp_blk);
        read_str_from_file(sig_str, g1_size, fname_sig, fp_sig);

        element_from_bytes(b_i, block_buf);
        element_from_bytes_compressed(sigma_i, sig_str);
        element_random(v_i);

        element_mul_zn(prod, b_i, v_i);
        element_add(mu, mu, prod);

        element_pow_zn(expon, sigma_i, v_i);
        element_mul(sigma, sigma, expon);
    }
    finish = clock();
    unsigned char *mu_str = pbc_malloc(zr_size);
    unsigned char *sigma_str = pbc_malloc(g1_size);

    element_to_bytes(mu_str, mu);
    element_to_bytes_compressed(sigma_str, sigma);

    memset(file_path_buf, 0, sizeof(file_path_buf));
    sprintf(file_path_buf, PROOF_MU_PATH, sc_addr);
    write_str_to_file(mu_str, zr_size, file_path_buf, fp);
    memset(file_path_buf, 0, sizeof(file_path_buf));
    sprintf(file_path_buf, PROOF_SIGMA_PATH, sc_addr);
    write_str_to_file(sigma_str, g1_size, file_path_buf, fp);

    free(u_s);
    free(block_buf);
    pbc_free(name_str);
    pbc_free(sig_str);
    pbc_free(mu_str);
    pbc_free(sigma_str);
    mpz_clear(sum_r_pz);
    mpz_clear(sum_s_pz);
    element_clear(name);
    pairing_clear(pairing);

    printf("%f seconds\n", (double)(finish - start) / CLOCKS_PER_SEC);
    return 0;
}