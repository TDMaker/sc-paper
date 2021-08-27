#include <time.h>
#include "../utils/sha256.h"
#include "../utils/randys.h"
#define PAIRING_PARAM_FILE_PATH "../public_params/a.param"
#define ELEMENT_H_PATH "../public_params/h"
#define ELEMENT_G_PATH "../public_params/g"
#define PUBLIC_KEY_PATH "../public_params/public.key"
#define ELEMENT_NAME_PATH "../public_params/"
#define BLKING_INFO_PATH "../swap_zone/%s.info"
#define SUM_R_PATH "./data/%s/sum_r"
#define SUM_S_PATH "./data/%s/sum_s"
#define RIGHT_EQ_PATH "./data/%s/right_equation"
#define RANDOM_R_PATH "./data/%s/r"
#define HASH_VALUE2_PATH "./data/%s/rightr.sha"
#define PROOF_MU_PATH "./data/%s/mu"
#define PROOF_SIGMA_PATH "./data/%s/sigma"
#define RESULT_PATH "./data/%s/result"

int main(int argc, char *argv[])
{
    if (argc < 4)
    {
        puts("Need three parameters of fileName, chaAmnt, contractAddr");
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
    element_t mu, sigma, g, h, public_key, name;
    FILE *fp = NULL;
    unsigned long int result = 0;

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

    memset(file_path_buf, 0, sizeof(file_path_buf));
    sprintf(file_path_buf, BLKING_INFO_PATH, name_hr);
    read_str_from_file(buf, 32, file_path_buf, fp);
    size_t blk_total = str_to_int(buf);
    size_t blk_size = str_to_int(buf + 16);

    // 以下为生成伪排列函数和伪随机函数
    unsigned int *u_s = (unsigned int *)malloc(sizeof(unsigned int) * chal_amount);
    psd_permute(u_s, chal_amount, (unsigned int)r_long, blk_total);
    pbc_random_set_deterministic(s_long);

    element_init_Zr(mu, pairing);
    element_init_G1(sigma, pairing);
    element_init_G2(g, pairing);
    element_init_G1(h, pairing);
    element_init_G2(public_key, pairing);
    element_init_Zr(name, pairing);
    int zr_size = pairing_length_in_bytes_Zr(pairing);
    int g1_size = pairing_length_in_bytes_compressed_G1(pairing);
    int g2_size = pairing_length_in_bytes_compressed_G2(pairing);

    unsigned char *h_str = pbc_malloc(g1_size);
    read_str_from_file(h_str, g1_size, ELEMENT_H_PATH, fp);
    element_from_bytes_compressed(h, h_str);

    unsigned char *g2_str = pbc_malloc(g2_size);
    read_str_from_file(g2_str, g2_size, ELEMENT_G_PATH, fp);
    element_from_bytes_compressed(g, g2_str);

    read_str_from_file(g2_str, g2_size, PUBLIC_KEY_PATH, fp);
    element_from_bytes_compressed(public_key, g2_str);

    unsigned char *mu_str = pbc_malloc(zr_size);
    memset(file_path_buf, 0, sizeof(file_path_buf));
    sprintf(file_path_buf, PROOF_MU_PATH, sc_addr);
    read_str_from_file(mu_str, zr_size, file_path_buf, fp);
    element_from_bytes(mu, mu_str);

    unsigned char *sigma_str = pbc_malloc(g1_size);
    memset(file_path_buf, 0, sizeof(file_path_buf));
    sprintf(file_path_buf, PROOF_SIGMA_PATH, sc_addr);
    read_str_from_file(sigma_str, g1_size, file_path_buf, fp);
    element_from_bytes_compressed(sigma, sigma_str);

    unsigned char *name_str = pbc_malloc(zr_size);
    memset(file_path_buf, 0, sizeof(file_path_buf));
    sprintf(file_path_buf, "%s%s", ELEMENT_NAME_PATH, name_hr);
    read_str_from_file(name_str, zr_size, file_path_buf, fp);
    element_from_bytes(name, name_str);

    element_t v_i, left_p, right_p, prod;
    element_init_Zr(v_i, pairing);
    element_init_G1(left_p, pairing);
    element_init_G1(right_p, pairing);
    element_init_G1(prod, pairing);
    element_set1(prod);

    unsigned char hash_val[32] = {0};
    memset(buf, 0, sizeof(buf));
    memcpy(buf, name_hr, strlen(name_hr));
    size_t seq_pos = strlen(buf);
    start = clock();
    for (size_t i = 0; i < chal_amount; i++)
    {
        sprintf(buf + seq_pos, "%d", u_s[i]);
        sha256(buf, strlen(buf), hash_val);
        element_from_hash(left_p, hash_val, 32);
        element_random(v_i);
        element_pow_zn(left_p, left_p, v_i);
        element_mul(prod, prod, left_p);
    }
    element_pow_zn(right_p, h, mu);
    element_mul(prod, prod, right_p);

    element_t temp_left, temp_right;
    element_init_GT(temp_left, pairing);
    element_init_GT(temp_right, pairing);

    element_pairing(temp_left, sigma, g);
    element_pairing(temp_right, prod, public_key);

    if (!element_cmp(temp_left, temp_right))
    {
        result = 1;
        printf("Integrity verifies\n");
    }
    else
    {
        result = 1;
        printf("*BUG* Integrity does not verify *BUG*\n");
    }
    finish = clock();

    buf[0] = result + '0';
    memset(file_path_buf, 0, sizeof(file_path_buf));
    sprintf(file_path_buf, RESULT_PATH, sc_addr);
    write_str_to_file(buf, 1, file_path_buf, fp);

    mpz_t truncated_right_pz, local_r_pz, sum_pz;
    mpz_init(truncated_right_pz);
    mpz_init(local_r_pz);
    mpz_init(sum_pz);

    int zt_size = element_length_in_bytes(temp_right);
    unsigned char *right_str = pbc_malloc(zt_size);
    element_to_bytes(right_str, temp_right);
    // truncate to 31 bytes to prevent overflow after sum in smart contract.
    mpz_import(truncated_right_pz, 31, 1, sizeof(unsigned char), 0, 0, right_str);

    memset(buf, 0, sizeof(buf));
    memset(file_path_buf, 0, sizeof(file_path_buf));
    sprintf(file_path_buf, RANDOM_R_PATH, sc_addr);
    read_str_from_file(buf, sizeof(buf), file_path_buf, fp);
    mpz_init_set_str(local_r_pz, buf, 10);

    mpz_add(sum_pz, truncated_right_pz, local_r_pz);
    mpz_add_ui(sum_pz, sum_pz, result);

    memset(buf, 0, sizeof(buf));
    gmp_sprintf(buf, "%Zd", truncated_right_pz);
    memset(file_path_buf, 0, sizeof(file_path_buf));
    sprintf(file_path_buf, RIGHT_EQ_PATH, sc_addr);
    write_str_to_file(buf, strlen(buf), file_path_buf, fp);

    size_t *countp = (size_t *)malloc(sizeof(size_t));
    memset(buf, 0, sizeof(buf));
    mpz_export(buf, countp, 1, 1, 1, 0, sum_pz);
    sha256(buf, *countp, hash_val);
    memset(file_path_buf, 0, sizeof(file_path_buf));
    sprintf(file_path_buf, HASH_VALUE2_PATH, sc_addr);
    write_str_to_file(hash_val, sizeof(hash_val), file_path_buf, fp);

    free(u_s);
    free(countp);
    pbc_free(h_str);
    pbc_free(g2_str);
    pbc_free(sigma_str);
    pbc_free(mu_str);
    pbc_free(name_str);
    pbc_free(right_str);
    mpz_clear(sum_r_pz);
    mpz_clear(sum_s_pz);
    mpz_clear(truncated_right_pz);
    mpz_clear(local_r_pz);
    mpz_clear(sum_pz);
    element_clear(mu);
    element_clear(sigma);
    element_clear(g);
    element_clear(h);
    element_clear(public_key);
    element_clear(name);
    element_clear(v_i);
    element_clear(left_p);
    element_clear(right_p);
    element_clear(prod);
    element_clear(temp_left);
    element_clear(temp_right);
    pairing_clear(pairing);

    printf("%f seconds\n", (double)(finish - start) / CLOCKS_PER_SEC);
    return 0;
}