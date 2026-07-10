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

static uint32_t pkcs7_pad(uint8_t *data,
                          uint32_t len,
                          uint32_t bufferSize)
{
    uint32_t padLen = AES_BLOCKLEN - (len % AES_BLOCKLEN);

    if ((len + padLen) > bufferSize)
    {
        return 0;
    }

    memset(data + len, (uint8_t)padLen, padLen);
    return len + padLen;
}

static uint32_t pkcs7_unpad(uint8_t *data,
                            uint32_t len)
{
    if ((len == 0) || (len % AES_BLOCKLEN != 0))
    {
        return 0;
    }

    uint8_t padLen = data[len - 1];
    if ((padLen == 0) || (padLen > AES_BLOCKLEN))
    {
        return 0;
    }

    for (uint32_t i = len - padLen; i < len; i++)
    {
        if (data[i] != padLen)
        {
            return 0;
        }
    }

    return len - padLen;
}

/* ================= AES ENCRYPT ================= */

uint32_t encryptPayload(uint8_t *data,
                        uint32_t len,
                        uint32_t bufferSize)
{
    struct AES_ctx ctx;
    uint8_t local_iv[16];

    memcpy(local_iv, iv, 16);

    uint32_t paddedLen = pkcs7_pad(data, len, bufferSize);
    if (paddedLen == 0)
    {
        return 0;
    }

    AES_init_ctx_iv(&ctx,
                    aes_key,
                    local_iv);

    AES_CBC_encrypt_buffer(&ctx,
                           data,
                           paddedLen);

    return paddedLen;
}

/* ================= AES DECRYPT ================= */

uint32_t decryptPayload(uint8_t *data,
                        uint32_t len,
                        uint32_t *plainLen)
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

    uint32_t unpaddedLen = pkcs7_unpad(data, len);
    if (plainLen != NULL)
    {
        *plainLen = unpaddedLen;
    }

    return unpaddedLen;
}

/* ================= MAIN ================= */

int main(void)
{
    uint8_t payload[32] =
    {
        'D','R','O','N',
        'E','_','D','A',
        'T','A','_','2','5'
    };

    uint32_t crc_before;
    uint32_t crc_after;
    uint32_t plainLen = 13;
    uint32_t cipherLen;
    uint32_t decryptedLen;

    printf("Original Data:\n");
    printArray(payload, plainLen);

    crc_before = crc32(payload, plainLen);

    printf("\nCRC Before = %08X\n",
           crc_before);

    cipherLen = encryptPayload(payload, plainLen, sizeof(payload));
    if (cipherLen == 0)
    {
        printf("Failed to pad and encrypt data\n");
        return 1;
    }

    printf("\nEncrypted Data (%u bytes):\n", cipherLen);
    printArray(payload, cipherLen);

    /* Uncomment to test CRC failure */
    /* payload[5] ^= 0x01; */

    decryptedLen = decryptPayload(payload, cipherLen, &plainLen);
    if (decryptedLen == 0)
    {
        printf("Failed to decrypt or unpad data\n");
        return 1;
    }

    printf("\nDecrypted Data (%u bytes):\n", decryptedLen);
    printArray(payload, decryptedLen);

    crc_after = crc32(payload, decryptedLen);

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