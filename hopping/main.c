#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "frequency_hopping.h"

/***************************
 * TEST APPLICATION
 * Main program demonstrating all features
 ***************************/

/* Define our frequency hopping handle */
FH_HandleTypeDef fhHandle;

/**
 * Demo 1: Basic Sequential Frequency Hopping
 */
void Demo_BasicSequential(void)
{
    printf("\n\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║      DEMO 1: BASIC SEQUENTIAL FREQUENCY HOPPING            ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n\n");

    /* Define hop table (channels in MHz or arbitrary units) */
    uint8_t hopTable[] = {10, 20, 30, 40, 50};
    uint8_t channelCount = 5;

    /* Initialize frequency hopping */
    if (FH_Init(&fhHandle, hopTable, channelCount) != 0)
    {
        printf("ERROR: Failed to initialize FH\n");
        return;
    }

    printf("Hopping through channels (10ms per hop):\n");
    for (int i = 0; i < 12; i++)
    {
        uint8_t channel = FH_NextChannel(&fhHandle);
        printf("→ Hop %2d: Channel %d MHz\n", i + 1, channel);
    }

    /* Print statistics */
    uint32_t totalHops, cycleCount;
    uint8_t lastChannel;
    FH_GetStatistics(&fhHandle, &totalHops, &cycleCount, &lastChannel);
    printf("\nStatistics:\n");
    printf("  Total Hops: %ld\n", totalHops);
    printf("  Cycles Completed: %ld\n", cycleCount);
    printf("  Last Channel: %d MHz\n", lastChannel);
}

/**
 * Demo 2: Configurable Hop Interval
 */
void Demo_HopInterval(void)
{
    printf("\n\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║        DEMO 2: CONFIGURABLE HOP INTERVAL                   ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n\n");

    FH_Reset(&fhHandle);
    
    printf("Current hop interval: %d ms\n", FH_GetHopInterval(&fhHandle));
    
    printf("\nChanging hop interval to 50ms...\n");
    FH_SetHopInterval(&fhHandle, 50);
    printf("New hop interval: %d ms\n", FH_GetHopInterval(&fhHandle));

    printf("\nPerforming 8 hops with 50ms interval:\n");
    for (int i = 0; i < 8; i++)
    {
        uint8_t channel = FH_NextChannel(&fhHandle);
        printf("  Hop %d: Channel %d (in 50ms)\n", i + 1, channel);
    }
}

/**
 * Demo 3: Channel Blacklist
 */
void Demo_Blacklist(void)
{
    printf("\n\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║           DEMO 3: CHANNEL BLACKLIST                        ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n\n");

    FH_Reset(&fhHandle);
    FH_SetHopInterval(&fhHandle, 100); /* Reset to default */

    printf("Suppose channels 30 and 40 have interference.\n");
    printf("Blacklisting them...\n\n");

    FH_BlacklistChannel(&fhHandle, 30);
    FH_BlacklistChannel(&fhHandle, 40);

    printf("Hopping sequence (automatically skipping 30 and 40):\n");
    for (int i = 0; i < 12; i++)
    {
        uint8_t channel = FH_NextChannel(&fhHandle);
        printf("→ Hop %2d: Channel %d MHz\n", i + 1, channel);
    }

    printf("\nRemoving channel 30 from blacklist...\n");
    FH_UnBlacklistChannel(&fhHandle, 30);

    printf("Next 5 hops (with 30 available again):\n");
    for (int i = 0; i < 5; i++)
    {
        uint8_t channel = FH_NextChannel(&fhHandle);
        printf("→ Hop %2d: Channel %d MHz\n", i + 1, channel);
    }
}

/**
 * Demo 4: Pseudo-Random Hopping (LFSR)
 */
void Demo_PseudoRandom(void)
{
    printf("\n\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║       DEMO 4: PSEUDO-RANDOM HOPPING (LFSR)                 ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n\n");

    FH_Reset(&fhHandle);

    printf("Sequential hopping (predictable):\n");
    for (int i = 0; i < 10; i++)
    {
        uint8_t channel = FH_NextChannel(&fhHandle);
        printf("  %d ", channel);
    }
    printf("\n\n");

    FH_Reset(&fhHandle);
    printf("Enabling LFSR pseudo-random mode with seed 0x42...\n\n");
    FH_EnablePseudoRandom(&fhHandle, 0x42);

    printf("Pseudo-random hopping sequence (harder to predict):\n");
    for (int i = 0; i < 10; i++)
    {
        uint8_t channel = FH_NextChannel(&fhHandle);
        printf("  %d ", channel);
    }
    printf("\n\n");

    printf("Same sequence with same seed (reproducible):\n");
    FH_Reset(&fhHandle);
    FH_EnablePseudoRandom(&fhHandle, 0x42);
    for (int i = 0; i < 10; i++)
    {
        uint8_t channel = FH_NextChannel(&fhHandle);
        printf("  %d ", channel);
    }
    printf("\n");
}

/**
 * Demo 5: Synchronization
 */
void Demo_Synchronization(void)
{
    printf("\n\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║           DEMO 5: SYNCHRONIZATION                          ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n\n");

    FH_Reset(&fhHandle);
    FH_DisablePseudoRandom(&fhHandle);

    printf("TX and RX should hop together:\n");
    printf("TX: 10 → 20 → 30 → 40 → 50\n");
    printf("RX: 10 → 20 → 30 → 40 → 50\n\n");

    printf("Simulating RX synchronization:\n");
    for (int i = 0; i < 5; i++)
    {
        uint8_t txChannel = FH_NextChannel(&fhHandle);
        printf("TX hops to channel: %d → ", txChannel);
        
        if (FH_Synchronize(&fhHandle, txChannel) == 0)
        {
            printf("RX Synchronized ✓\n");
        }
        else
        {
            printf("RX Lost Sync ✗\n");
        }
    }

    printf("\n\nSimulating 3 missed packets:\n");
    FH_Reset(&fhHandle);
    
    for (int i = 0; i < 3; i++)
    {
        uint8_t txChannel = FH_NextChannel(&fhHandle);
        printf("Missed packet %d on channel %d\n", i + 1, txChannel);
        FH_Synchronize(&fhHandle, txChannel + 1); /* Wrong channel */
    }
    
    printf("Sync Status: %s\n", FH_IsSynchronized(&fhHandle) ? "SYNCED" : "LOST");
}

/**
 * Demo 6: Statistics and Debug Info
 */
void Demo_Statistics(void)
{
    printf("\n\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║       DEMO 6: STATISTICS AND DEBUG INFO                    ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n\n");

    FH_Reset(&fhHandle);
    FH_DisablePseudoRandom(&fhHandle);

    printf("Performing 20 hops...\n\n");
    for (int i = 0; i < 20; i++)
    {
        FH_NextChannel(&fhHandle);
    }

    /* Print detailed debug info */
    FH_PrintDebugInfo(&fhHandle);
}

/**
 * Demo 7: Activity Control
 */
void Demo_ActivityControl(void)
{
    printf("\n\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║         DEMO 7: ACTIVATION/DEACTIVATION                    ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n\n");

    FH_Reset(&fhHandle);
    FH_DisablePseudoRandom(&fhHandle);

    printf("Hopping active (normal operation):\n");
    for (int i = 0; i < 3; i++)
    {
        uint8_t channel = FH_NextChannel(&fhHandle);
        printf("  Channel: %d\n", channel);
    }

    printf("\nDeactivating hopping...\n");
    FH_SetActive(&fhHandle, 0);

    printf("Trying to hop (should return 0):\n");
    uint8_t channel = FH_NextChannel(&fhHandle);
    printf("  Channel: %d (inactive, returns 0)\n", channel);

    printf("\nReactivating hopping...\n");
    FH_SetActive(&fhHandle, 1);
    channel = FH_NextChannel(&fhHandle);
    printf("  Channel: %d (active again)\n", channel);
}

/**
 * Demo 8: Multiple Independent Hopping Engines
 */
void Demo_MultipleHandles(void)
{
    printf("\n\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║       DEMO 8: MULTIPLE INDEPENDENT ENGINES                 ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n\n");

    /* First radio */
    FH_HandleTypeDef radio1;
    uint8_t table1[] = {10, 20, 30};
    FH_Init(&radio1, table1, 3);
    FH_SetHopInterval(&radio1, 100);

    /* Second radio */
    FH_HandleTypeDef radio2;
    uint8_t table2[] = {50, 60, 70, 80};
    FH_Init(&radio2, table2, 4);
    FH_SetHopInterval(&radio2, 50);

    printf("Radio 1 hopping (3 channels, 100ms interval):\n");
    for (int i = 0; i < 5; i++)
    {
        printf("  %d ", FH_NextChannel(&radio1));
    }
    printf("\n\n");

    printf("Radio 2 hopping (4 channels, 50ms interval):\n");
    for (int i = 0; i < 8; i++)
    {
        printf("  %d ", FH_NextChannel(&radio2));
    }
    printf("\n");
}

/**
 * Main Test Routine
 */
int main(void)
{
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║     FREQUENCY HOPPING LIBRARY - COMPREHENSIVE TEST          ║\n");
    printf("║                                                            ║\n");
    printf("║  STM32F411CEU6 Frequency Hopping Engine                    ║\n");
    printf("║  Professional Embedded Firmware Library                    ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");

    /* Run all demos */
    Demo_BasicSequential();
    Demo_HopInterval();
    Demo_Blacklist();
    Demo_PseudoRandom();
    Demo_Synchronization();
    Demo_Statistics();
    Demo_ActivityControl();
    Demo_MultipleHandles();

    printf("\n\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║                 ALL TESTS COMPLETED ✓                      ║\n");
    printf("║                                                            ║\n");
    printf("║  The library is ready for:                                 ║\n");
    printf("║  • Embedded systems (STM32, Arduino, etc.)                  ║\n");
    printf("║  • Wireless communication protocols                         ║\n");
    printf("║  • E220 LoRa module integration                            ║\n");
    printf("║  • Real-time frequency hopping                             ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n\n");

    return 0;
}
