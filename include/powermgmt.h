#ifndef POWERMGMT_H
#define POWERMGMT_H

constexpr int POWERMGMT_MAX_FREQ_MHZ = 160;
constexpr int POWERMGMT_MIN_FREQ_MHZ = 40;

/**
 * Enable or disable automatic and light sleep.
 */
void powermgmt_sleep(bool enable);

#endif
