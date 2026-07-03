#ifndef CONFIG_H
#define CONFIG_H

/***************************
 * FREQUENCY HOPPING CONFIG
 ***************************/

/* Maximum number of channels in hop table */
#define FH_MAX_CHANNELS          32

/* Default hop interval in milliseconds */
#define FH_DEFAULT_HOP_INTERVAL  100

/* Maximum blacklisted channels */
#define FH_MAX_BLACKLIST         10

/* LFSR polynomial for pseudo-random generation */
#define LFSR_POLYNOMIAL          0xB8

/* Synchronization timeout (ms) */
#define FH_SYNC_TIMEOUT          500

/* Enable statistics tracking */
#define FH_ENABLE_STATS          1

/* Enable debug output */
#define FH_DEBUG                 1

#endif /* CONFIG_H */
