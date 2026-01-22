#ifndef UMS_CORE_H
#define UMS_CORE_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Transmission interface callback type
 * @param pData Pointer to data buffer to transmit
 * @param length Length of data in bytes
 */
typedef void (*TransmissionInterface)(const void *pData, uint32_t length);

/**
 * @brief Critical section enter callback (disable interrupts)
 */
typedef void (*CriticalSectionEnter)(void);

/**
 * @brief Critical section exit callback (enable interrupts)
 */
typedef void (*CriticalSectionExit)(void);

/**
 * @brief Time provider callback - returns current time in microseconds
 * @return Current time in microseconds (typically from system tick/timer)
 */
typedef uint32_t (*TimeProvider)(void);

/**
 * @brief Transmission configuration structure
 */
typedef struct {
    TransmissionInterface transmit_fn;
    CriticalSectionEnter enter_critical;  /* Optional: NULL for no protection */
    CriticalSectionExit exit_critical;    /* Optional: NULL for no protection */
    TimeProvider time_provider;           /* Optional: NULL uses simple counter */
} ums_core_transmission_config_t;

/**
 * @brief Setup UMS with transmission interface
 * 
 * @param pConfig Pointer to transmission configuration
 * @return true on success
 * @return false on failure
 */
bool UMSSetup(const ums_core_transmission_config_t *pConfig);

/**
 * @brief Add a variable to trace
 * 
 * @param pVariable Pointer to float variable to trace
 * @return true on success
 * @return false on failure (max channels reached)
 */
bool UMSTrace(const float* pVariable);

/**
 * @brief Update all traced variables and trigger transmission
 * Call this function at your desired sampling rate
 * 
 * @return true on success
 * @return false on failure
 */
bool UMSUpdate(void);

/**
 * @brief Transmission complete callback - call this from your transmission ISR/callback
 * Manages triple buffer rotation
 */
void UMSTransmissionComplete(void);

/**
 * @brief Cleanup and destroy UMS instance
 */
void UMSDestroy(void);

/**
 * @brief Get current number of traced channels
 * 
 * @return uint8_t Number of channels
 */
uint8_t UMSGetChannelCount(void);

/**
 * @brief Check if UMS is initialized
 * 
 * @return true if initialized
 * @return false if not initialized
 */
bool UMSIsInitialized(void);

#ifdef __cplusplus
}
#endif

#endif /* UMS_CORE_H */