#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "frequency_hopping.h"

/***************************
 * MINIMAL E220 HOPPING EXAMPLE
 * This entry point keeps the existing frequency hopping API intact and
 * drives a real E220-900M22S channel change and user payload transmit.
 ***************************/

FH_HandleTypeDef fhHandle;

/**
 * Initialize the hopping engine and the E220 transport before the first hop.
 */
static int32_t App_Init(void)
{
    uint8_t hopTable[] = {10u, 20u, 30u, 40u, 50u};
    if (E220_Init() != 0)
    {
        printf("[APP] E220 transport init failed\n");
        return -1;
    }
    if (FH_Init(&fhHandle, hopTable, (uint8_t)(sizeof(hopTable) / sizeof(hopTable[0]))) != 0)
    {
        printf("[APP] Frequency hopping init failed\n");
        return -1;
    }
    FH_SetHopInterval(&fhHandle, 100u);
    return 0;
}

/**
 * Feed the hop engine one step at a time and transmit data on the new channel.
 */
static void App_RunTxSequence(void)
{
    uint8_t rxBuffer[32];
    uint16_t rxLen = 0u;
    int i;
    for (i = 0; i < 6; ++i)
    {
        uint8_t channel = FH_NextChannel(&fhHandle);
        char payload[24];
        snprintf(payload, sizeof(payload), "PKT-%u", (unsigned)channel);
        printf("[APP] Hop %d -> channel %u, payload=%s\n", i + 1, (unsigned)channel, payload);
        if (E220_Send((const uint8_t *)payload, (uint16_t)strlen(payload)) != 0)
        {
            printf("[APP] E220 transmit failed on channel %u\n", (unsigned)channel);
            continue;
        }
        if (E220_Receive(rxBuffer, &rxLen) == 0 && rxLen > 0u)
        {
            rxBuffer[rxLen] = '\0';
            printf("[APP] Received echo: %s\n", rxBuffer);
        }
    }
}

int main(void)
{
    if (App_Init() != 0)
        return 1;
    App_RunTxSequence();
    return 0;
}
