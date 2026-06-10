#include <stdio.h>
#include <stdint.h>

void encrypt(uint8_t *data, uint8_t len, uint8_t idx)
{
    uint8_t i;

    switch(idx % 8)
    {
        case 0:
            for(i = 0; i < len; i++)
                data[i] ^= 0x5A;
            break;

        case 1:
            for(i = 0; i < len; i++)
                data[i] = (uint8_t)((data[i] << 3) | (data[i] >> 5));
            break;

        case 2:
            for(i = 0; i < len; i++)
                data[i] = (uint8_t)((data[i] << 1) | (data[i] >> 7));
            break;

        case 3:
            for(i = 0; i < len; i++)
                data[i] ^= 0xA7;
            break;

        case 4:
            for(i = 0; i < len; i++)
                data[i] = (uint8_t)(((data[i] & 0xF0) >> 4) |
                                    ((data[i] & 0x0F) << 4));
            break;

        case 5:
            for(i = 0; i < len; i++)
                data[i] = (uint8_t)(~data[i]);
            break;

        case 6:
            for(i = 0; i < len; i++)
                data[i] = (uint8_t)((data[i] << 2) | (data[i] >> 6));
            break;

        case 7:
            for(i = 0; i < len; i++)
                data[i] ^= 0xC3;
            break;
    }
}

void decrypt(uint8_t *data, uint8_t len, uint8_t idx)
{
    uint8_t i;

    switch(idx % 8)
    {
        case 0:
            for(i = 0; i < len; i++)
                data[i] ^= 0x5A;
            break;

        case 1:
            for(i = 0; i < len; i++)
                data[i] = (uint8_t)((data[i] >> 3) | (data[i] << 5));
            break;

        case 2:
            for(i = 0; i < len; i++)
                data[i] = (uint8_t)((data[i] >> 1) | (data[i] << 7));
            break;

        case 3:
            for(i = 0; i < len; i++)
                data[i] ^= 0xA7;
            break;

        case 4:
            for(i = 0; i < len; i++)
                data[i] = (uint8_t)(((data[i] & 0xF0) >> 4) |
                                    ((data[i] & 0x0F) << 4));
            break;

        case 5:
            for(i = 0; i < len; i++)
                data[i] = (uint8_t)(~data[i]);
            break;

        case 6:
            for(i = 0; i < len; i++)
                data[i] = (uint8_t)((data[i] >> 2) | (data[i] << 6));
            break;

        case 7:
            for(i = 0; i < len; i++)
                data[i] ^= 0xC3;
            break;
    }
}

void printArray(uint8_t *data, uint8_t len)
{
    uint8_t i;

    for(i = 0; i < len; i++)
    {
        printf("%02X ", data[i]);
    }
    printf("\n");
}

int main(void)
{
    uint8_t data[] = {0x16, 0xFF, 0x75, 0x96, 0x55};
    uint8_t len = sizeof(data) / sizeof(data[0]);
    uint8_t idx = 0;   /* Change from 0 to 7 to test different cases */

    printf("Original Data : ");
    printArray(data, len);

    encrypt(data, len, idx);
    printf("Encrypted Data: ");
    printArray(data, len);

    decrypt(data, len, idx);
    printf("Decrypted Data: ");
    printArray(data, len);

    return 0;
}