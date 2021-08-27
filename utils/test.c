#include <stdio.h>
#include "sha256.h"
#include <stdlib.h>
int main(int argc, char *argv[])
{
    unsigned char in[] = "hello world";
    unsigned char buff[32]; //必须带unsigned ,sha256消息摘要输出为256位,即32字节
    memset(buff, 0, 32);
    puts("start sha256 hash \n");
    sha256(in, strlen(in), buff);
    printf("\nThe sha256 hash is :\n");
    for (int i = 0; i < 32; i++)
    {

        printf("%x", buff[i]);
    }
    puts("\nend sha256 hash \n");
    return 0;
}