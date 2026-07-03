#ifndef E220_INTEGRATION_GUIDE_H
#define E220_INTEGRATION_GUIDE_H

/**
 * E220 INTEGRATION GUIDE
 * 
 * This guide shows how to integrate the Frequency Hopping Library
 * with the E220-900M22S LoRa module when you're ready.
 * 
 * Current Status: Library is ready, E220 driver can be added anytime
 * Integration Effort: ~2-3 hours for basic setup
 */

/*
 * STEP 1: CREATE E220 DRIVER HEADER (e220.h)
 * ============================================
 * 
 * #include <stdint.h>
 * 
 * #define E220_FREQ_MIN  850000000   // 850 MHz
 * #define E220_FREQ_MAX  930000000   // 930 MHz
 * #define E220_CHANNELS  32          // Available channels
 * 
 * typedef struct
 * {
 *     uint8_t channel;
 *     uint32_t frequency;
 *     uint8_t power;
 *     uint8_t bandwidth;
 * } E220_ConfigTypeDef;
 * 
 * int E220_Init(void);
 * int E220_SetChannel(uint8_t channel);
 * uint8_t E220_GetChannel(void);
 * int E220_SendData(uint8_t *data, uint16_t length);
 * int E220_ReceiveData(uint8_t *data, uint16_t *length);
 * 
 */

/*
 * STEP 2: CREATE E220 DRIVER IMPLEMENTATION (e220.c)
 * ====================================================
 * 
 * #include "e220.h"
 * #include "uart.h"  // Your UART driver
 * 
 * static E220_ConfigTypeDef e220Config;
 * 
 * // Map channel number to actual frequency
 * static uint32_t E220_ChannelToFreq(uint8_t channel)
 * {
 *     // Example: 5 MHz spacing between channels
 *     return 850000000 + (channel * 5000000);
 * }
 * 
 * int E220_Init(void)
 * {
 *     // Initialize UART for E220 communication
 *     UART_Init(UART3, 9600, 8, 1, 0);  // Common E220 baud rate
 *     
 *     e220Config.channel = 0;
 *     e220Config.power = 20;  // 20 dBm (max)
 *     
 *     return 0;
 * }
 * 
 * int E220_SetChannel(uint8_t channel)
 * {
 *     if (channel >= E220_CHANNELS)
 *         return -1;
 *     
 *     e220Config.channel = channel;
 *     e220Config.frequency = E220_ChannelToFreq(channel);
 *     
 *     // Send command to E220 via UART
 *     uint8_t cmd[] = {0xC0, 0x00, channel, 0x00};  // Example command
 *     UART_Write(UART3, cmd, sizeof(cmd));
 *     
 *     return 0;
 * }
 * 
 */

/*
 * STEP 3: UPDATE MAIN HOPPING LOOP
 * =================================
 * 
 * BEFORE (current test code):
 * 
 *     uint8_t channel = FH_NextChannel(&fhHandle);
 *     printf("Channel: %d\n", channel);
 * 
 * 
 * AFTER (with E220):
 * 
 *     uint8_t channel = FH_NextChannel(&fhHandle);
 *     E220_SetChannel(channel);  // ← ONE LINE CHANGE
 * 
 * 
 * That's it! No other changes needed.
 * 
 */

/*
 * STEP 4: TIMER-BASED AUTOMATIC HOPPING
 * ======================================
 * 
 * Instead of manually calling FH_NextChannel(), use a timer interrupt:
 * 
 * void TIM2_Init(void)
 * {
 *     // Configure TIM2 for 100ms interrupt
 *     // STM32 HAL example:
 *     htim2.Instance = TIM2;
 *     htim2.Init.Period = 99999;  // 100ms at 1MHz
 *     HAL_TIM_Base_Init(&htim2);
 *     HAL_TIM_Base_Start_IT(&htim2);
 * }
 * 
 * void TIM2_IRQHandler(void)  // Automatic every 100ms
 * {
 *     __HAL_TIM_CLEAR_IT(&htim2, TIM_IT_UPDATE);
 *     
 *     uint8_t channel = FH_NextChannel(&fhHandle);
 *     E220_SetChannel(channel);  // Change frequency
 * }
 * 
 */

/*
 * STEP 5: COMPLETE WORKING EXAMPLE
 * =================================
 * 
 * // In main.c
 * 
 * #include "frequency_hopping.h"
 * #include "e220.h"
 * #include "stm32f4xx_hal.h"
 * 
 * FH_HandleTypeDef fhHandle;
 * 
 * int main(void)
 * {
 *     HAL_Init();
 *     SystemClock_Config();
 *     
 *     // Initialize frequency hopping
 *     uint8_t channels[] = {0, 1, 2, 3, 4, 5, 6, 7};  // 8 channels
 *     FH_Init(&fhHandle, channels, 8);
 *     FH_SetHopInterval(&fhHandle, 100);  // 100ms per hop
 *     
 *     // Initialize E220 module
 *     E220_Init();
 *     
 *     // Set initial channel
 *     uint8_t initialChannel = FH_NextChannel(&fhHandle);
 *     E220_SetChannel(initialChannel);
 *     
 *     // Start automatic hopping (100ms timer interrupt)
 *     TIM2_Init();
 *     
 *     // Now E220 automatically hops every 100ms!
 *     while(1)
 *     {
 *         // Receive and process data
 *         uint8_t rxData[256];
 *         uint16_t rxLen;
 *         
 *         if (E220_ReceiveData(rxData, &rxLen) == 0)
 *         {
 *             // Process received packet
 *             ProcessPacket(rxData, rxLen);
 *         }
 *     }
 *     
 *     return 0;
 * }
 * 
 */

/*
 * STEP 6: SYNCHRONIZATION EXAMPLE
 * ================================
 * 
 * When receiving a packet with sync info:
 * 
 * void HandleSyncPacket(uint8_t *packet)
 * {
 *     uint8_t txChannel = packet[0];  // Channel TX is on
 *     
 *     if (FH_Synchronize(&fhHandle, txChannel) == 0)
 *     {
 *         // Synchronized! Set radio to expected channel
 *         E220_SetChannel(txChannel);
 *     }
 *     else
 *     {
 *         // Out of sync - force hop
 *         uint8_t newChannel = FH_NextChannel(&fhHandle);
 *         E220_SetChannel(newChannel);
 *     }
 * }
 * 
 */

/*
 * STEP 7: CHANNEL BLACKLIST WITH E220
 * =====================================
 * 
 * Adaptive frequency hopping:
 * 
 * void HandlePacketLoss(void)
 * {
 *     // If packet loss on current channel...
 *     uint8_t badChannel = FH_GetCurrentChannel(&fhHandle);
 *     
 *     // Blacklist it
 *     FH_BlacklistChannel(&fhHandle, badChannel);
 *     
 *     // Force hop to next (non-blacklisted) channel
 *     uint8_t newChannel = FH_NextChannel(&fhHandle);
 *     E220_SetChannel(newChannel);
 * }
 * 
 */

/*
 * TESTING CHECKLIST
 * =================
 * 
 * Before going live:
 * 
 * ✓ Test E220 UART communication
 * ✓ Verify channel frequency switching
 * ✓ Check timing of hops (100ms)
 * ✓ Verify TX/RX synchronization
 * ✓ Monitor RF performance (range, packet loss)
 * ✓ Test blacklist functionality
 * ✓ Verify ISR timing with oscilloscope
 * ✓ Check for memory leaks (heap size OK)
 * ✓ Test in actual flight conditions
 * 
 */

/*
 * FILE STRUCTURE AFTER INTEGRATION
 * ==================================
 * 
 * hopping/
 * ├── config.h                   (existing)
 * ├── frequency_hopping.h        (existing)
 * ├── frequency_hopping.c        (existing)
 * ├── e220.h                     (NEW - to create)
 * ├── e220.c                     (NEW - to create)
 * ├── main.c                     (update with E220 code)
 * ├── README.md                  (existing)
 * └── PROJECT_SUMMARY.md         (existing)
 * 
 */

/*
 * ESTIMATED EFFORT
 * ================
 * 
 * Task                          Time        Difficulty
 * ─────────────────────────────────────────────────────
 * Create e220.h                 30 min      Easy
 * Create e220.c (basic)         1 hour      Medium
 * Update main.c                 15 min      Easy
 * Test UART communication       1 hour      Medium
 * Test frequency hopping        30 min      Easy
 * Debug & optimize              1 hour      Hard
 * ─────────────────────────────────────────────────────
 * TOTAL                         ~4 hours    Medium
 * 
 */

/*
 * COMMON ISSUES & SOLUTIONS
 * =========================
 * 
 * Issue: E220 not responding on UART
 * Solution: Check baud rate (usually 9600), verify UART pins
 * 
 * Issue: Channels not changing properly
 * Solution: Verify frequency mapping function, check E220 command format
 * 
 * Issue: Hops happening at wrong interval
 * Solution: Check timer clock, verify TIM2_IRQHandler priority
 * 
 * Issue: TX/RX losing sync
 * Solution: Verify hop table is identical on both sides, check packet loss
 * 
 * Issue: RF range is short
 * Solution: Verify antenna connection, check E220 power setting, try lower hops/sec
 * 
 */

#endif /* E220_INTEGRATION_GUIDE_H */
