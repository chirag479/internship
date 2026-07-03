#include "frequency_hopping.h"
#include <string.h>
#include <stdio.h>

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
        printf("[FH] Cycle completed (Total cycles: %ld)\n", handle->cycleCount);
#endif
    }
    else
    {
        handle->cycleCompleted = 0;
    }

#if FH_DEBUG
    printf("[FH] Hop to channel: %d (Index: %d, Total hops: %ld)\n", 
           handle->lastChannel, nextIndex, handle->totalHops);
#endif

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
    printf("Total Hops:          %ld\n", handle->totalHops);
    printf("Cycle Count:         %ld\n", handle->cycleCount);
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
