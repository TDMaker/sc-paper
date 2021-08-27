#include <time.h>
#include "../utils/sha256.h"
#include "../utils/randys.h"
#define RANDOM_R_PATH "./data/%s/r"
#define RANDOM_S_PATH "./data/%s/s"
#define HASH_VALUE_PATH "./data/%s/rs.sha"
#define SECURITY_LEVEL 256
// 使用 gmp lib 生成两个大数，r 和 s，相加生成 sum，r 和 s 分别字面量化存文件；
// 将 sum 转换成 raw bytes，求哈希，再存文件。

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        puts("Need one parameter contractAddr.");
        exit(0);
    }
    unsigned char *sc_addr = argv[1];
    unsigned char buf[1024] = {0};
    unsigned char file_path_buf[128] = {0};
    mpz_t num_r, num_s, sum;
    gmp_randstate_t state;
    mpz_init(num_r);
    mpz_init(num_s);
    mpz_init(sum);
    gmp_randinit_mt(state);
    unsigned long int *seed = (unsigned long int *)malloc(sizeof(unsigned long int) * 2);
    memset(seed, 0, sizeof(unsigned long int) * 2);
    FILE *fp = NULL;
    do
    {
        read_str_from_file((unsigned char *)seed, sizeof(unsigned long int) * 2, "/dev/urandom", fp);
        gmp_randseed_ui(state, seed[0]);
        mpz_urandomb(num_r, state, SECURITY_LEVEL);
        gmp_randseed_ui(state, seed[1]);
        mpz_urandomb(num_s, state, SECURITY_LEVEL);
        mpz_add(sum, num_r, num_s);
    } while (mpz_sizeinbase(sum, 2) > SECURITY_LEVEL); // in case of uint256 overflow in solidity

    gmp_sprintf(buf, "%Zd", num_r);
    sprintf(file_path_buf, RANDOM_R_PATH, sc_addr);
    write_str_to_file(buf, strlen(buf), file_path_buf, fp);

    memset(buf, 0, sizeof(buf));
    gmp_sprintf(buf, "%Zd", num_s);
    memset(file_path_buf, 0, sizeof(file_path_buf));
    sprintf(file_path_buf, RANDOM_S_PATH, sc_addr);
    write_str_to_file(buf, strlen(buf), file_path_buf, fp);

    memset(buf, 0, sizeof(buf));
    size_t *countp = (size_t *)malloc(sizeof(size_t));
    mpz_export(buf, countp, 1, 1, 1, 0, sum);

    unsigned char hash_val[32];
    sha256(buf, *countp, hash_val);
    memset(file_path_buf, 0, sizeof(file_path_buf));
    sprintf(file_path_buf, HASH_VALUE_PATH, sc_addr);
    write_str_to_file(hash_val, sizeof(hash_val), file_path_buf, fp);

    free(seed);
    free(countp);
    return 0;
}