#include "frequency_hopping.h"
#include <string.h>
#include <stdio.h>

#if defined(__has_include)
# if __has_include("stm32f4xx_hal.h")
#  include "stm32f4xx_hal.h"
#  define E220_STM32_HAL_ENABLED 1u
# else
#  define E220_STM32_HAL_ENABLED 0u
# endif
#else
# define E220_STM32_HAL_ENABLED 0u
#endif

/***************************
 * LFSR (Linear Feedback Shift Register)
 * For pseudo-random frequency hopping
 ***************************/

/**
 * Galois LFSR - generates pseudo-random sequence
 * @param lfsr: Current LFSR state
 * @return: Next LFSR state
 */
static uint8_t LFSR_NextBit(uint8_t lfsr)
{
    uint8_t feedback = (lfsr >> 7) ^ (lfsr >> 5) ^ (lfsr >> 4) ^ (lfsr >> 3);
    return ((lfsr << 1) | feedback) & 0xFF;
}

/***************************
 * E220 HARDWARE HELPERS
 ***************************/

/* Keep the module state private to this translation unit. */
static uint8_t g_e220Initialized = 0u;
static uint8_t g_e220CurrentChannel = E220_DEFAULT_CHANNEL;
static uint8_t g_e220TxBuffer[E220_TX_BUFFER_SIZE];
static uint8_t g_e220RxBuffer[E220_RX_BUFFER_SIZE];

#if E220_STM32_HAL_ENABLED
extern UART_HandleTypeDef huart1;
#ifndef E220_M0_GPIO_PORT
#define E220_M0_GPIO_PORT GPIOA
#endif
#ifndef E220_M0_GPIO_PIN
#define E220_M0_GPIO_PIN GPIO_PIN_0
#endif
#ifndef E220_M1_GPIO_PORT
#define E220_M1_GPIO_PORT GPIOA
#endif
#ifndef E220_M1_GPIO_PIN
#define E220_M1_GPIO_PIN GPIO_PIN_1
#endif
#ifndef E220_AUX_GPIO_PORT
#define E220_AUX_GPIO_PORT GPIOA
#endif
#ifndef E220_AUX_GPIO_PIN
#define E220_AUX_GPIO_PIN GPIO_PIN_2
#endif
#endif

/**
 * Return a millisecond tick for timeout logic.
 * On a real STM32 target this is typically HAL_GetTick().
 */
static uint32_t E220_HalGetTick(void)
{
#if E220_STM32_HAL_ENABLED
    return HAL_GetTick();
#else
    return 0u;
#endif
}

/**
 * Delay the caller for a small amount of time.
 * This is used between the M0/M1 transition and the UART write sequence.
 */
static void E220_HalDelayMs(uint32_t ms)
{
#if E220_STM32_HAL_ENABLED
    HAL_Delay(ms);
#else
    (void)ms;
#endif
}

/**
 * Write a byte sequence to the E220 UART interface.
 * This is the real STM32 HAL UART transmit path.
 */
static int32_t E220_HalUartWrite(const uint8_t *data, uint16_t len)
{
#if E220_STM32_HAL_ENABLED
    if (data == NULL || len == 0u)
        return -1;
    if (HAL_UART_Transmit(&huart1, (uint8_t *)data, len, E220_CMD_TIMEOUT_MS) != HAL_OK)
        return -1;
    return 0;
#else
    (void)data;
    (void)len;
    return -1;
#endif
}

/**
 * Read a byte sequence from the E220 UART interface.
 * This is the real STM32 HAL UART receive path.
 */
static int32_t E220_HalUartRead(uint8_t *data, uint16_t *len, uint32_t timeoutMs)
{
#if E220_STM32_HAL_ENABLED
    if (data == NULL || len == NULL)
        return -1;
    *len = 0u;
    if (HAL_UART_Receive(&huart1, data, 1u, timeoutMs) != HAL_OK)
        return -1;
    *len = 1u;
    return 0;
#else
    (void)timeoutMs;
    if (data == NULL || len == NULL)
        return -1;
    *len = 0u;
    return -1;
#endif
}

/**
 * Drive the E220 M0/M1 pins during configuration mode entry and exit.
 * These pins are driven directly through STM32 GPIO HAL calls.
 */
static int32_t E220_HalSetModePins(uint8_t m0, uint8_t m1)
{
#if E220_STM32_HAL_ENABLED
    HAL_GPIO_WritePin(E220_M0_GPIO_PORT, E220_M0_GPIO_PIN, (m0 != 0u) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(E220_M1_GPIO_PORT, E220_M1_GPIO_PIN, (m1 != 0u) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    return 0;
#else
    (void)m0;
    (void)m1;
    return -1;
#endif
}

/**
 * Wait until the E220 AUX pin becomes ready after a mode change.
 * The GPIO line is polled until it becomes HIGH or the timeout expires.
 */
static int32_t E220_HalWaitAUX(uint32_t timeoutMs)
{
#if E220_STM32_HAL_ENABLED
    uint32_t startTick = E220_HalGetTick();
    while ((E220_HalGetTick() - startTick) < timeoutMs)
    {
        if (HAL_GPIO_ReadPin(E220_AUX_GPIO_PORT, E220_AUX_GPIO_PIN) == GPIO_PIN_SET)
            return 0;
        E220_HalDelayMs(E220_AUX_READY_DELAY_MS);
    }
    return -1;
#else
    (void)timeoutMs;
    (void)E220_HalGetTick();
    return -1;
#endif
}

/**
 * Enter E220 configuration mode by driving M0/M1 and sending the UART entry command.
 * The frame bytes below are the minimal UART commands used to start a configuration write.
 */
static int32_t E220_EnterConfigMode(void)
{
    uint8_t cmd[4] = {0xC0u, 0x00u, 0x00u, 0x00u};
    /* Enter configuration mode by driving M0 and M1 high. */
    if (E220_HalSetModePins(E220_CONFIG_MODE_M0, E220_CONFIG_MODE_M1) != 0)
        return -1;
    E220_HalDelayMs(20u);

    /* Wait until AUX reports the module is ready before sending the entry frame. */
    if (E220_HalWaitAUX(E220_CONFIG_TIMEOUT_MS) != 0)
        return -1;

    /* Send the official configuration-mode entry frame. */
    if (E220_HalUartWrite(cmd, sizeof(cmd)) != 0)
        return -1;
    E220_HalDelayMs(20u);
    return 0;
}

/**
 * Save the temporary E220 register write to non-volatile memory.
 */
static int32_t E220_SaveConfig(void)
{
    uint8_t cmd[4] = {0xC2u, 0x00u, 0x00u, 0x00u};
    /* Wait until AUX is ready before issuing the save command. */
    if (E220_HalWaitAUX(E220_CONFIG_TIMEOUT_MS) != 0)
        return -1;
    if (E220_HalUartWrite(cmd, sizeof(cmd)) != 0)
        return -1;
    E220_HalDelayMs(20u);
    return 0;
}

/**
 * Exit configuration mode and return the module to normal operation.
 */
static int32_t E220_ExitConfigMode(void)
{
    uint8_t cmd[4] = {0xC3u, 0x00u, 0x00u, 0x00u};
    /* Wait until AUX is ready before sending the exit frame. */
    if (E220_HalWaitAUX(E220_CONFIG_TIMEOUT_MS) != 0)
        return -1;
    if (E220_HalUartWrite(cmd, sizeof(cmd)) != 0)
        return -1;
    if (E220_HalSetModePins(E220_NORMAL_MODE_M0, E220_NORMAL_MODE_M1) != 0)
        return -1;
    E220_HalDelayMs(20u);
    return E220_HalWaitAUX(E220_CONFIG_TIMEOUT_MS);
}

/**
 * Initialize the E220 UART transport and mark the radio as ready.
 */
int32_t E220_Init(void)
{
    if (g_e220Initialized)
        return 0;
    E220_HalDelayMs(10u);
    if (E220_HalSetModePins(E220_NORMAL_MODE_M0, E220_NORMAL_MODE_M1) != 0)
        return -1;
    E220_HalDelayMs(10u);
    g_e220Initialized = 1u;
    g_e220CurrentChannel = E220_DEFAULT_CHANNEL;
    return 0;
}

/**
 * Apply a new E220 channel by entering configuration mode, writing the new channel,
 * saving the register, and then returning the module to normal mode.
 */
int32_t E220_SetChannel(uint8_t channel)
{
    uint8_t cmd[4];
    if (channel >= E220_CHANNELS_MAX)
        return -1;
    if (g_e220Initialized == 0u && E220_Init() != 0)
        return -1;

    /* Build the E220 CHAN-register write frame using the UART config protocol. */
    cmd[0] = 0xC1u;
    cmd[1] = 0x00u;
    cmd[2] = channel;
    cmd[3] = 0x00u;

    /* Enter configuration mode and wait for AUX to indicate readiness. */
    if (E220_EnterConfigMode() != 0)
        return -1;

    /* Wait for AUX before issuing the channel-register write. */
    if (E220_HalWaitAUX(E220_CONFIG_TIMEOUT_MS) != 0)
        return -1;
    if (E220_HalUartWrite(cmd, sizeof(cmd)) != 0)
        return -1;
    if (E220_SaveConfig() != 0)
        return -1;
    if (E220_ExitConfigMode() != 0)
        return -1;
    g_e220CurrentChannel = channel;
    return 0;
}

/**
 * Send a user payload through the E220 UART transport.
 */
int32_t E220_Send(const uint8_t *data, uint16_t length)
{
    if (data == NULL || length == 0u || length > E220_TX_BUFFER_SIZE)
        return -1;
    if (g_e220Initialized == 0u && E220_Init() != 0)
        return -1;
    memcpy(g_e220TxBuffer, data, length);

    /* Wait until AUX is HIGH before transmitting a payload. */
    if (E220_HalWaitAUX(E220_CONFIG_TIMEOUT_MS) != 0)
        return -1;
    if (E220_HalUartWrite(g_e220TxBuffer, length) != 0)
        return -1;
    E220_HalDelayMs(5u);
    return 0;
}

/**
 * Receive a payload from the E220 UART transport when one is pending.
 */
int32_t E220_Receive(uint8_t *data, uint16_t *length)
{
    uint16_t rxLen = 0u;
    if (data == NULL || length == NULL)
        return -1;
    if (g_e220Initialized == 0u && E220_Init() != 0)
        return -1;
    if (E220_HalUartRead(g_e220RxBuffer, &rxLen, E220_CMD_TIMEOUT_MS) != 0)
        return -1;
    if (rxLen > E220_RX_BUFFER_SIZE)
        return -1;
    memcpy(data, g_e220RxBuffer, rxLen);
    *length = rxLen;
    return (rxLen > 0u) ? 0 : -1;
}

/***************************
 * CORE FUNCTIONS
 ***************************/

/**
 * Initialize Frequency Hopping
 */
int32_t FH_Init(FH_HandleTypeDef *handle, uint8_t *hopTable, uint8_t channelCount)
{
    if (handle == NULL || hopTable == NULL || channelCount == 0 || channelCount > FH_MAX_CHANNELS)
    {
        return -1; /* Invalid parameters */
    }

    /* Initialize handle structure */
    memset(handle, 0, sizeof(FH_HandleTypeDef));

    /* Copy hop table */
    memcpy(handle->hopTable, hopTable, channelCount);
    handle->totalChannels = channelCount;
    
    /* Set default values */
    handle->currentIndex = 0;
    handle->hopInterval = FH_DEFAULT_HOP_INTERVAL;
    handle->hopCounter = 0;
    handle->cycleCompleted = 0;
    handle->blacklistCount = 0;
    handle->lfsrState = 1; /* Default LFSR seed */
    handle->pseudoRandomMode = 0; /* Sequential by default */
    handle->isActive = 1; /* Active by default */
    handle->syncLocked = 1; /* Assume synchronized */
    handle->lastSyncTime = 0;
    handle->missedPackets = 0;
    
#if FH_DEBUG
    printf("[FH] Initialized with %d channels, hop interval: %d ms\n", 
           channelCount, FH_DEFAULT_HOP_INTERVAL);
#endif

    return 0;
}

/**
 * De-initialize Frequency Hopping
 */
int32_t FH_DeInit(FH_HandleTypeDef *handle)
{
    if (handle == NULL)
        return -1;

    handle->isActive = 0;
    memset(handle, 0, sizeof(FH_HandleTypeDef));
    
#if FH_DEBUG
    printf("[FH] De-initialized\n");
#endif

    return 0;
}

/**
 * Check if channel is blacklisted (internal helper)
 */
static uint8_t FH_IsChannelBlacklistedInternal(FH_HandleTypeDef *handle, uint8_t channel)
{
    for (uint8_t i = 0; i < handle->blacklistCount; i++)
    {
        if (handle->blacklist[i] == channel)
            return 1;
    }
    return 0;
}

/**
 * Get next channel with blacklist support
 */
static uint8_t FH_GetNextChannelIndex(FH_HandleTypeDef *handle)
{
    uint8_t nextIndex = (handle->currentIndex + 1) % handle->totalChannels;
    uint8_t attempts = 0;
    
    /* Skip blacklisted channels */
    while (FH_IsChannelBlacklistedInternal(handle, handle->hopTable[nextIndex]) && 
           attempts < handle->totalChannels)
    {
        nextIndex = (nextIndex + 1) % handle->totalChannels;
        attempts++;
    }
    
    return nextIndex;
}

/**
 * Get next channel in hopping sequence
 */
uint8_t FH_NextChannel(FH_HandleTypeDef *handle)
{
    if (handle == NULL || !handle->isActive || handle->totalChannels == 0)
        return 0;

    uint8_t nextIndex;
    
    if (handle->pseudoRandomMode)
    {
        /* LFSR-based pseudo-random hopping */
        handle->lfsrState = LFSR_NextBit(handle->lfsrState);
        nextIndex = handle->lfsrState % handle->totalChannels;
        
        /* Skip blacklisted channels */
        uint8_t attempts = 0;
        while (FH_IsChannelBlacklistedInternal(handle, handle->hopTable[nextIndex]) && 
               attempts < handle->totalChannels)
        {
            handle->lfsrState = LFSR_NextBit(handle->lfsrState);
            nextIndex = handle->lfsrState % handle->totalChannels;
            attempts++;
        }
    }
    else
    {
        /* Sequential hopping */
        nextIndex = FH_GetNextChannelIndex(handle);
    }

    handle->currentIndex = nextIndex;
    handle->lastChannel = handle->hopTable[nextIndex];
    handle->hopCounter++;
    handle->totalHops++;

    /* Check for cycle completion */
    if (nextIndex == 0)
    {
        handle->cycleCompleted = 1;
        handle->cycleCount++;
#if FH_DEBUG
        printf("[FH] Cycle completed (Total cycles: %lu)\n", (unsigned long)handle->cycleCount);
#endif
    }
    else
    {
        handle->cycleCompleted = 0;
    }

#if FH_DEBUG
    printf("[FH] Hop to channel: %d (Index: %d, Total hops: %lu)\n", 
           handle->lastChannel, nextIndex, (unsigned long)handle->totalHops);
#endif

    /* Apply the newly selected channel to the real E220 module. */
    if (E220_SetChannel(handle->lastChannel) != 0)
    {
#if FH_DEBUG
        printf("[FH][E220] Failed to apply channel %u via UART\n", handle->lastChannel);
#endif
    }
    else
    {
#if FH_DEBUG
        printf("[FH][E220] Applied channel %u via UART\n", handle->lastChannel);
#endif

        /* Send a small user payload only after the channel change succeeded. */
        {
            static const uint8_t payload[] = "HOP";
            (void)E220_Send(payload, (uint16_t)(sizeof(payload) - 1u));
        }
    }

    return handle->lastChannel;
}

/**
 * Get previous channel
 */
uint8_t FH_PreviousChannel(FH_HandleTypeDef *handle)
{
    if (handle == NULL || handle->totalChannels == 0)
        return 0;

    uint8_t prevIndex = (handle->currentIndex - 1 + handle->totalChannels) % handle->totalChannels;
    
    /* Skip blacklisted channels */
    uint8_t attempts = 0;
    while (FH_IsChannelBlacklistedInternal(handle, handle->hopTable[prevIndex]) && 
           attempts < handle->totalChannels)
    {
        prevIndex = (prevIndex - 1 + handle->totalChannels) % handle->totalChannels;
        attempts++;
    }

    handle->currentIndex = prevIndex;
    handle->lastChannel = handle->hopTable[prevIndex];

#if FH_DEBUG
    printf("[FH] Previous channel: %d\n", handle->lastChannel);
#endif

    return handle->lastChannel;
}

/**
 * Get current channel without advancing
 */
uint8_t FH_GetCurrentChannel(FH_HandleTypeDef *handle)
{
    if (handle == NULL || handle->totalChannels == 0)
        return 0;

    return handle->hopTable[handle->currentIndex];
}

/**
 * Reset hopping to first channel
 */
int32_t FH_Reset(FH_HandleTypeDef *handle)
{
    if (handle == NULL)
        return -1;

    handle->currentIndex = 0;
    handle->hopCounter = 0;
    handle->cycleCount = 0;
    handle->totalHops = 0;
    handle->lfsrState = 1;
    handle->syncLocked = 1;
    handle->missedPackets = 0;

#if FH_DEBUG
    printf("[FH] Reset to first channel: %d\n", handle->hopTable[0]);
#endif

    return 0;
}

/**
 * Set hop interval
 */
int32_t FH_SetHopInterval(FH_HandleTypeDef *handle, uint16_t intervalMs)
{
    if (handle == NULL || intervalMs == 0)
        return -1;

    handle->hopInterval = intervalMs;

#if FH_DEBUG
    printf("[FH] Hop interval set to: %d ms\n", intervalMs);
#endif

    return 0;
}

/**
 * Get hop interval
 */
uint16_t FH_GetHopInterval(FH_HandleTypeDef *handle)
{
    if (handle == NULL)
        return 0;

    return handle->hopInterval;
}

/**
 * Add channel to blacklist
 */
int32_t FH_BlacklistChannel(FH_HandleTypeDef *handle, uint8_t channel)
{
    if (handle == NULL || handle->blacklistCount >= FH_MAX_BLACKLIST)
        return -1;

    /* Check if already blacklisted */
    if (FH_IsChannelBlacklistedInternal(handle, channel))
        return 0;

    handle->blacklist[handle->blacklistCount] = channel;
    handle->blacklistCount++;

#if FH_DEBUG
    printf("[FH] Channel %d blacklisted (Total blacklisted: %d)\n", 
           channel, handle->blacklistCount);
#endif

    return 0;
}

/**
 * Remove channel from blacklist
 */
int32_t FH_UnBlacklistChannel(FH_HandleTypeDef *handle, uint8_t channel)
{
    if (handle == NULL)
        return -1;

    for (uint8_t i = 0; i < handle->blacklistCount; i++)
    {
        if (handle->blacklist[i] == channel)
        {
            /* Shift remaining items */
            for (uint8_t j = i; j < handle->blacklistCount - 1; j++)
            {
                handle->blacklist[j] = handle->blacklist[j + 1];
            }
            handle->blacklistCount--;

#if FH_DEBUG
            printf("[FH] Channel %d removed from blacklist\n", channel);
#endif

            return 0;
        }
    }

    return -1; /* Not found */
}

/**
 * Check if channel is blacklisted
 */
uint8_t FH_IsChannelBlacklisted(FH_HandleTypeDef *handle, uint8_t channel)
{
    if (handle == NULL)
        return 0;

    return FH_IsChannelBlacklistedInternal(handle, channel);
}

/**
 * Enable pseudo-random hopping
 */
int32_t FH_EnablePseudoRandom(FH_HandleTypeDef *handle, uint8_t seed)
{
    if (handle == NULL)
        return -1;

    handle->pseudoRandomMode = 1;
    handle->lfsrState = (seed == 0) ? 1 : seed; /* Avoid zero seed */

#if FH_DEBUG
    printf("[FH] Pseudo-random mode ENABLED (seed: %d)\n", handle->lfsrState);
#endif

    return 0;
}

/**
 * Disable pseudo-random hopping
 */
int32_t FH_DisablePseudoRandom(FH_HandleTypeDef *handle)
{
    if (handle == NULL)
        return -1;

    handle->pseudoRandomMode = 0;

#if FH_DEBUG
    printf("[FH] Pseudo-random mode DISABLED (sequential)\n");
#endif

    return 0;
}

/**
 * Synchronize with transmitter
 */
int32_t FH_Synchronize(FH_HandleTypeDef *handle, uint8_t expectedChannel)
{
    if (handle == NULL)
        return -1;

    uint8_t currentChannel = FH_GetCurrentChannel(handle);

    if (currentChannel == expectedChannel)
    {
        handle->syncLocked = 1;
        handle->missedPackets = 0;

#if FH_DEBUG
        printf("[FH] SYNCHRONIZED on channel %d\n", expectedChannel);
#endif

        return 0;
    }
    else
    {
        handle->missedPackets++;
        
        if (handle->missedPackets >= 3)
        {
            handle->syncLocked = 0;

#if FH_DEBUG
            printf("[FH] LOST SYNC - Expected: %d, Got: %d (Missed: %d)\n", 
                   expectedChannel, currentChannel, handle->missedPackets);
#endif

            return -1;
        }
    }

    return 0;
}

/**
 * Check if synchronized
 */
uint8_t FH_IsSynchronized(FH_HandleTypeDef *handle)
{
    if (handle == NULL)
        return 0;

    return handle->syncLocked;
}

/**
 * Get statistics
 */
int32_t FH_GetStatistics(FH_HandleTypeDef *handle, uint32_t *totalHops, 
                         uint32_t *cycleCount, uint8_t *lastChannel)
{
    if (handle == NULL)
        return -1;

    if (totalHops != NULL)
        *totalHops = handle->totalHops;
    if (cycleCount != NULL)
        *cycleCount = handle->cycleCount;
    if (lastChannel != NULL)
        *lastChannel = handle->lastChannel;

    return 0;
}

/**
 * Print debug information
 */
void FH_PrintDebugInfo(FH_HandleTypeDef *handle)
{
    if (handle == NULL)
        return;

    printf("\n========== FREQUENCY HOPPING DEBUG INFO ==========\n");
    printf("Active:              %s\n", handle->isActive ? "YES" : "NO");
    printf("Current Index:       %d\n", handle->currentIndex);
    printf("Current Channel:     %d\n", FH_GetCurrentChannel(handle));
    printf("Hop Interval:        %d ms\n", handle->hopInterval);
    printf("Total Hops:          %lu\n", (unsigned long)handle->totalHops);
    printf("Cycle Count:         %lu\n", (unsigned long)handle->cycleCount);
    printf("Total Channels:      %d\n", handle->totalChannels);
    printf("Blacklisted Count:   %d\n", handle->blacklistCount);
    printf("Random Mode:         %s\n", handle->pseudoRandomMode ? "LFSR" : "Sequential");
    printf("LFSR State:          0x%02X\n", handle->lfsrState);
    printf("Synchronized:        %s\n", handle->syncLocked ? "YES" : "NO");
    printf("Missed Packets:      %d\n", handle->missedPackets);
    
    printf("Hop Table:           [");
    for (uint8_t i = 0; i < handle->totalChannels; i++)
    {
        printf("%d", handle->hopTable[i]);
        if (i < handle->totalChannels - 1)
            printf(", ");
    }
    printf("]\n");

    if (handle->blacklistCount > 0)
    {
        printf("Blacklist:           [");
        for (uint8_t i = 0; i < handle->blacklistCount; i++)
        {
            printf("%d", handle->blacklist[i]);
            if (i < handle->blacklistCount - 1)
                printf(", ");
        }
        printf("]\n");
    }

    printf("===================================================\n\n");
}

/**
 * Activate/Deactivate hopping
 */
int32_t FH_SetActive(FH_HandleTypeDef *handle, uint8_t active)
{
    if (handle == NULL)
        return -1;

    handle->isActive = active;

#if FH_DEBUG
    printf("[FH] Hopping %s\n", active ? "ACTIVATED" : "DEACTIVATED");
#endif

    return 0;
}

/**
 * Check if hopping is active
 */
uint8_t FH_IsActive(FH_HandleTypeDef *handle)
{
    if (handle == NULL)
        return 0;

    return handle->isActive;
}
