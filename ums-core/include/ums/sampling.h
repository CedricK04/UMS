#ifndef UMS_SAMPLING_H
#define UMS_SAMPLING_H

#include <stdint.h>
#include <stddef.h>

#define UMS_MAX_CHANNELS 6

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Sample structure (transmitted over interface)
 * Packed for efficient transmission
 */
typedef struct __attribute__((packed)) {
    uint32_t timestamp;      // 4 bytes
    uint8_t channels;        // 1 byte
    float values[UMS_MAX_CHANNELS];  // 24 bytes (6 * 4)
} ums_sample_t;              // Total: 29 bytes

/**
 * @brief Calculate sample size based on number of channels
 * 
 * @param channels Number of channels
 * @return uint32_t Size in bytes
 */
static inline uint32_t ums_sample_size(uint8_t channels) {
    return sizeof(uint32_t) + sizeof(uint8_t) + (channels * sizeof(float));
}

#ifdef __cplusplus
}
#endif

#endif /* UMS_SAMPLING_H */