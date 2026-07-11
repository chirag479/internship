#ifndef CONFIG_H
#define CONFIG_H

/***************************
 * FREQUENCY HOPPING CONFIG
 ***************************/

/* Maximum number of channels in hop table */
#define FH_MAX_CHANNELS          32u

/* Default hop interval in milliseconds */
#define FH_DEFAULT_HOP_INTERVAL  100u

/* Maximum blacklisted channels */
#define FH_MAX_BLACKLIST         10u

/* LFSR polynomial for pseudo-random generation */
#define LFSR_POLYNOMIAL          0xB8u

/* Synchronization timeout (ms) */
#define FH_SYNC_TIMEOUT          500u

/* Enable statistics tracking */
#define FH_ENABLE_STATS          1u

/* Enable debug output */
#define FH_DEBUG                 1u

/* E220-900M22S UART configuration */
#define E220_UART_BAUDRATE       9600u
#define E220_CONFIG_TIMEOUT_MS   250u
#define E220_CMD_TIMEOUT_MS      100u
#define E220_DEFAULT_CHANNEL     0u
#define E220_CHANNELS_MAX        32u
#define E220_TX_BUFFER_SIZE      64u
#define E220_RX_BUFFER_SIZE      64u
#define E220_NORMAL_MODE_M0      0u
#define E220_NORMAL_MODE_M1      0u
#define E220_CONFIG_MODE_M0      1u
#define E220_CONFIG_MODE_M1      1u
#define E220_AUX_READY_DELAY_MS  5u

#endif /* CONFIG_H */
