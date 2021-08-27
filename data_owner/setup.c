#include "../utils/randys.h"
#define PAIRING_PARAM_FILE_PATH "../public_params/a.param"
#define ELEMENT_G_PATH "../public_params/g"
#define ELEMENT_H_PATH "../public_params/h"
#define SECRET_KEY_PATH "./secret.key"
#define PUBLIC_KEY_PATH "../public_params/public.key"

int main(int argc, char *argv[])
{
    FILE *fp = NULL;
    pairing_t pairing;
    element_t g, h, public_key, secret_key;

    if (pairing_init(pairing, argc == 1 ? PAIRING_PARAM_FILE_PATH : argv[1]) < 0)
        exit(0);
    element_init_G1(h, pairing);
    element_init_G2(g, pairing);
    element_init_G2(public_key, pairing);
    element_init_Zr(secret_key, pairing);

    int g1_size = pairing_length_in_bytes_compressed_G1(pairing);
    int g2_size = pairing_length_in_bytes_compressed_G2(pairing);
    int zr_size = pairing_length_in_bytes_Zr(pairing);

    unsigned char *g_str = pbc_malloc(g2_size);
    element_random(g);
    element_to_bytes_compressed(g_str, g);
    write_str_to_file(g_str, g2_size, ELEMENT_G_PATH, fp);

    unsigned char *h_str = pbc_malloc(g1_size);
    element_random(h);
    element_to_bytes_compressed(h_str, h);
    write_str_to_file(h_str, g1_size, ELEMENT_H_PATH, fp);

    unsigned char *secret_key_str = pbc_malloc(zr_size);
    element_random(secret_key);
    element_to_bytes(secret_key_str, secret_key);
    write_str_to_file(secret_key_str, zr_size, SECRET_KEY_PATH, fp);

    unsigned char *public_key_str = pbc_malloc(g2_size);
    element_pow_zn(public_key, g, secret_key);
    element_to_bytes_compressed(public_key_str, public_key);
    write_str_to_file(public_key_str, g2_size, PUBLIC_KEY_PATH, fp);

    pbc_free(g_str);
    pbc_free(h_str);
    pbc_free(secret_key_str);
    pbc_free(public_key_str);
    element_clear(h);
    element_clear(g);
    element_clear(public_key);
    element_clear(secret_key);
    pairing_clear(pairing);

    return 0;
}