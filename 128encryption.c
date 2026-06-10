#include <stdio.h>
#include <stdint.h>

/* CRC16 Function */
uint16_t crc16(uint8_t *data, uint8_t len)
{
    uint16_t crc = 0xFFFF;
    uint8_t i, j;

    for(i = 0; i < len; i++)
    {
        crc ^= data[i];

        for(j = 0; j < 8; j++)
        {
            if(crc & 1)
                crc = (crc >> 1) ^ 0xA001;
            else
                crc >>= 1;
        }
    }

    return crc;
}

/* Encryption */
void encrypt(uint8_t *data, uint8_t len, uint8_t key)
{
    uint8_t i;

    for(i = 0; i < len; i++)
    {
        /* Dynamic XOR Key */
        data[i] ^= (key + i);

        /* Rotate Left by 3 */
        data[i] = (uint8_t)((data[i] << 3) |
                            (data[i] >> 5));

        /* Nibble Swap */
        data[i] = (uint8_t)(((data[i] & 0xF0) >> 4) |
                            ((data[i] & 0x0F) << 4));
    }
}

/* Decryption */
void decrypt(uint8_t *data, uint8_t len, uint8_t key)
{
    uint8_t i;

    for(i = 0; i < len; i++)
    {
        /* Reverse Nibble Swap */
        data[i] = (uint8_t)(((data[i] & 0xF0) >> 4) |
                            ((data[i] & 0x0F) << 4));

        /* Reverse Rotate */
        data[i] = (uint8_t)((data[i] >> 3) |
                            (data[i] << 5));

        /* Reverse XOR */
        data[i] ^= (key + i);
    }
}

/* Print Array */
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
    uint8_t data[] =
    {
        0x34,
        0x55,
        0x33,
        0x78,
        0x55
    };

    uint8_t len = sizeof(data) / sizeof(data[0]);
    uint8_t key = 0x5A;

    uint16_t crc_before;
    uint16_t crc_after;

    printf("Original Data : ");
    printArray(data, len);

    crc_before = crc16(data, len);

    encrypt(data, len, key);

    printf("Encrypted Data: ");
    printArray(data, len);

    decrypt(data, len, key);

    printf("Decrypted Data: ");
    printArray(data, len);

    crc_after = crc16(data, len);

    printf("CRC Before : %04X\n", crc_before);
    printf("CRC After  : %04X\n", crc_after);

    if(crc_before == crc_after)
    {
        printf("CRC Verification Passed\n");
    }
    else
    {
        printf("CRC Verification Failed\n");
    }

    return 0;
}