#ifndef FREQUENCY_HOPPING_H
#define FREQUENCY_HOPPING_H

#include "config.h"
#include <stdint.h>
#include <stdbool.h>

/***************************
 * DATA STRUCTURES
 ***************************/

/**
 * Frequency Hopping Handle Structure
 * (STM32 HAL style - allows multiple independent hopping engines)
 */
typedef struct
{
    uint8_t hopTable[FH_MAX_CHANNELS];      /* Array of channel frequencies */
    uint8_t totalChannels;                   /* Number of valid channels */
    uint8_t currentIndex;                    /* Current position in hop table */
    uint16_t hopInterval;                    /* Hop interval in milliseconds */
    uint32_t hopCounter;                     /* Total number of hops performed */
    uint8_t cycleCompleted;                  /* Flag: cycle completed */
    
    /* Blacklist management */
    uint8_t blacklist[FH_MAX_BLACKLIST];    /* Blacklisted channels */
    uint8_t blacklistCount;                  /* Number of blacklisted channels */
    
    /* LFSR state for pseudo-random hopping */
    uint8_t lfsrState;                       /* LFSR state (seed) */
    uint8_t pseudoRandomMode;                /* 0: Sequential, 1: LFSR-based */
    
    /* Statistics */
    uint32_t totalHops;                      /* Total hops since init */
    uint32_t cycleCount;                     /* Number of completed cycles */
    uint8_t lastChannel;                     /* Last hopped channel */
    
    /* Synchronization */
    uint8_t syncLocked;                      /* 0: Not synced, 1: Synced */
    uint32_t lastSyncTime;                   /* Timestamp of last sync */
    uint8_t missedPackets;                   /* Consecutive missed packets */
    
    /* State management */
    uint8_t isActive;                        /* 0: Inactive, 1: Active */
    uint8_t state;                           /* Current state (for future use) */
    
} FH_HandleTypeDef;

/***************************
 * E220 DRIVER PROTOTYPES
 ***************************/

/**
 * Initialize the E220 module UART transport and prepare the module for use.
 * @return: 0 = success, -1 = error
 */
int32_t E220_Init(void);

/**
 * Apply a new channel through the E220 UART configuration protocol.
 * @param channel: Channel number to write to the module.
 * @return: 0 = success, -1 = error
 */
int32_t E220_SetChannel(uint8_t channel);

/**
 * Transmit user payload bytes through the E220 UART transport.
 * @param data: Payload bytes to send.
 * @param length: Payload length in bytes.
 * @return: 0 = success, -1 = error
 */
int32_t E220_Send(const uint8_t *data, uint16_t length);

/**
 * Receive a payload from the E220 UART transport.
 * @param data: Output buffer for received bytes.
 * @param length: Pointer to the received payload length.
 * @return: 0 = success, -1 = error
 */
int32_t E220_Receive(uint8_t *data, uint16_t *length);

/***************************
 * FUNCTION DECLARATIONS
 ***************************/

/**
 * Initialize Frequency Hopping
 * @param handle: Pointer to FH handle
 * @param hopTable: Pointer to array of channel frequencies
 * @param channelCount: Number of channels in table
 * @return: 0 = success, -1 = error
 */
int32_t FH_Init(FH_HandleTypeDef *handle, uint8_t *hopTable, uint8_t channelCount);

/**
 * De-initialize Frequency Hopping
 * @param handle: Pointer to FH handle
 * @return: 0 = success
 */
int32_t FH_DeInit(FH_HandleTypeDef *handle);

/**
 * Get next channel in hopping sequence
 * @param handle: Pointer to FH handle
 * @return: Next channel frequency
 */
uint8_t FH_NextChannel(FH_HandleTypeDef *handle);

/**
 * Get previous channel
 * @param handle: Pointer to FH handle
 * @return: Previous channel frequency
 */
uint8_t FH_PreviousChannel(FH_HandleTypeDef *handle);

/**
 * Get current channel without advancing
 * @param handle: Pointer to FH handle
 * @return: Current channel frequency
 */
uint8_t FH_GetCurrentChannel(FH_HandleTypeDef *handle);

/**
 * Reset hopping to first channel
 * @param handle: Pointer to FH handle
 * @return: 0 = success
 */
int32_t FH_Reset(FH_HandleTypeDef *handle);

/**
 * Set hop interval
 * @param handle: Pointer to FH handle
 * @param intervalMs: Hop interval in milliseconds
 * @return: 0 = success
 */
int32_t FH_SetHopInterval(FH_HandleTypeDef *handle, uint16_t intervalMs);

/**
 * Get hop interval
 * @param handle: Pointer to FH handle
 * @return: Hop interval in milliseconds
 */
uint16_t FH_GetHopInterval(FH_HandleTypeDef *handle);

/**
 * Add channel to blacklist (will be skipped)
 * @param handle: Pointer to FH handle
 * @param channel: Channel frequency to blacklist
 * @return: 0 = success, -1 = blacklist full
 */
int32_t FH_BlacklistChannel(FH_HandleTypeDef *handle, uint8_t channel);

/**
 * Remove channel from blacklist
 * @param handle: Pointer to FH handle
 * @param channel: Channel frequency to remove
 * @return: 0 = success, -1 = not found
 */
int32_t FH_UnBlacklistChannel(FH_HandleTypeDef *handle, uint8_t channel);

/**
 * Check if channel is blacklisted
 * @param handle: Pointer to FH handle
 * @param channel: Channel frequency to check
 * @return: 1 = blacklisted, 0 = not blacklisted
 */
uint8_t FH_IsChannelBlacklisted(FH_HandleTypeDef *handle, uint8_t channel);

/**
 * Enable pseudo-random hopping (LFSR)
 * @param handle: Pointer to FH handle
 * @param seed: Initial LFSR seed
 * @return: 0 = success
 */
int32_t FH_EnablePseudoRandom(FH_HandleTypeDef *handle, uint8_t seed);

/**
 * Disable pseudo-random hopping (use sequential)
 * @param handle: Pointer to FH handle
 * @return: 0 = success
 */
int32_t FH_DisablePseudoRandom(FH_HandleTypeDef *handle);

/**
 * Synchronize with transmitter
 * @param handle: Pointer to FH handle
 * @param expectedChannel: Expected channel from sync packet
 * @return: 0 = synced, -1 = out of sync
 */
int32_t FH_Synchronize(FH_HandleTypeDef *handle, uint8_t expectedChannel);

/**
 * Check if synchronized
 * @param handle: Pointer to FH handle
 * @return: 1 = synced, 0 = not synced
 */
uint8_t FH_IsSynchronized(FH_HandleTypeDef *handle);

/**
 * Get statistics
 * @param handle: Pointer to FH handle
 * @param totalHops: Pointer to store total hops
 * @param cycleCount: Pointer to store cycle count
 * @param lastChannel: Pointer to store last channel
 * @return: 0 = success
 */
int32_t FH_GetStatistics(FH_HandleTypeDef *handle, uint32_t *totalHops, 
                         uint32_t *cycleCount, uint8_t *lastChannel);

/**
 * Print debug information
 * @param handle: Pointer to FH handle
 * @return: void
 */
void FH_PrintDebugInfo(FH_HandleTypeDef *handle);

/**
 * Activate/Deactivate hopping
 * @param handle: Pointer to FH handle
 * @param active: 1 = active, 0 = inactive
 * @return: 0 = success
 */
int32_t FH_SetActive(FH_HandleTypeDef *handle, uint8_t active);

/**
 * Check if hopping is active
 * @param handle: Pointer to FH handle
 * @return: 1 = active, 0 = inactive
 */
uint8_t FH_IsActive(FH_HandleTypeDef *handle);

#endif /* FREQUENCY_HOPPING_H */
