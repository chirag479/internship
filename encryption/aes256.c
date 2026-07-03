#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "aes.h"

/* ================= AES-256 KEY ================= */

static uint8_t aes_key[32] =
{
    0x60,0x3D,0xEB,0x10,
    0x15,0xCA,0x71,0xBE,
    0x2B,0x73,0xAE,0xF0,
    0x85,0x7D,0x77,0x81,

    0x1F,0x35,0x2C,0x07,
    0x3B,0x61,0x08,0xD7,
    0x2D,0x98,0x10,0xA3,
    0x09,0x14,0xDF,0xF4
};

/* ================= IV ================= */

static uint8_t iv[16] =
{
    0x00,0x01,0x02,0x03,
    0x04,0x05,0x06,0x07,
    0x08,0x09,0x0A,0x0B,
    0x0C,0x0D,0x0E,0x0F
};

/* ================= CRC32 ================= */

uint32_t crc32(uint8_t *data, uint32_t len)
{
    uint32_t crc = 0xFFFFFFFF;

    for(uint32_t i = 0; i < len; i++)
    {
        crc ^= data[i];

        for(uint8_t j = 0; j < 8; j++)
        {
            if(crc & 1)
                crc = (crc >> 1) ^ 0xEDB88320;
            else
                crc >>= 1;
        }
    }

    return ~crc;
}

/* ================= PRINT ================= */

void printArray(uint8_t *data, uint32_t len)
{
    for(uint32_t i = 0; i < len; i++)
    {
        printf("%02X ", data[i]);
    }

    printf("\n");
}

/* ================= AES ENCRYPT ================= */

void encryptPayload(uint8_t *data,
                    uint32_t len)
{
    struct AES_ctx ctx;

    uint8_t local_iv[16];

    memcpy(local_iv, iv, 16);

    AES_init_ctx_iv(&ctx,
                    aes_key,
                    local_iv);

    AES_CBC_encrypt_buffer(&ctx,
                           data,
                           len);
}

/* ================= AES DECRYPT ================= */

void decryptPayload(uint8_t *data,
                    uint32_t len)
{
    struct AES_ctx ctx;

    uint8_t local_iv[16];

    memcpy(local_iv, iv, 16);

    AES_init_ctx_iv(&ctx,
                    aes_key,
                    local_iv);

    AES_CBC_decrypt_buffer(&ctx,
                           data,
                           len);
}

/* ================= MAIN ================= */

int main(void)
{
    uint8_t payload[16] =
    {
        'D','R','O','N',
        'E','_','D','A',
        'T','A','_','2',
        '5','6','!','!'
    };

    uint32_t crc_before;
    uint32_t crc_after;

    printf("Original Data:\n");
    printArray(payload, 16);

    crc_before = crc32(payload, 16);

    printf("\nCRC Before = %08X\n",
           crc_before);

    encryptPayload(payload, 16);

    printf("\nEncrypted Data:\n");
    printArray(payload, 16);

    /* Uncomment to test CRC failure */

    /*
    payload[5] ^= 0x01;
    */

    decryptPayload(payload, 16);

    printf("\nDecrypted Data:\n");
    printArray(payload, 16);

    crc_after = crc32(payload, 16);

    printf("\nCRC After = %08X\n",
           crc_after);

    if(crc_before == crc_after)
    {
        printf("\nCRC VERIFICATION PASSED\n");
    }
    else
    {
        printf("\nCRC VERIFICATION FAILED\n");
    }

    return 0;
}