#include "ums/ums_core.h"
#include "ums/sampling.h"
#include <string.h>

/* Triple buffer indices */
#define BUFFER_WRITE    0
#define BUFFER_PENDING  1
#define BUFFER_TRANSMIT 2
#define NUM_BUFFERS     3

/* UMS state structure */
typedef struct {
    /* Configuration */
    TransmissionInterface transmit_fn;
    CriticalSectionEnter enter_critical;
    CriticalSectionExit exit_critical;
    TimeProvider time_provider;
    
    /* Traced variables */
    const float* trace_vars[UMS_MAX_CHANNELS];
    uint8_t channel_count;
    
    /* Triple buffering - volatile for ISR safety */
    ums_sample_t buffers[NUM_BUFFERS];
    volatile uint8_t write_idx;
    volatile uint8_t pending_idx;
    volatile uint8_t transmit_idx;
    volatile bool transmission_active;
    
    /* State flags */
    volatile bool initialized;
    
    /* Timestamp counter (only used when time_provider is NULL) */
    volatile uint32_t timestamp;
} ums_state_t;

/* Static instance - single instance for minimal footprint */
static ums_state_t ums_state = {0};

/* Helper function to enter critical section */
static inline void enter_critical(void) {
    if (ums_state.enter_critical != NULL) {
        ums_state.enter_critical();
    }
}

/* Helper function to exit critical section */
static inline void exit_critical(void) {
    if (ums_state.exit_critical != NULL) {
        ums_state.exit_critical();
    }
}

bool UMSSetup(const ums_core_transmission_config_t *pConfig) {
    if (pConfig == NULL) {
        return false;
    }
    
    if (pConfig->transmit_fn == NULL) {
        return false;
    }
    
    if (ums_state.initialized) {
        return false; /* Already initialized */
    }
    
    /* Initialize state */
    memset(&ums_state, 0, sizeof(ums_state_t));
    
    ums_state.transmit_fn = pConfig->transmit_fn;
    ums_state.enter_critical = pConfig->enter_critical;
    ums_state.exit_critical = pConfig->exit_critical;
    ums_state.time_provider = pConfig->time_provider;
    
    ums_state.write_idx = BUFFER_WRITE;
    ums_state.pending_idx = BUFFER_PENDING;
    ums_state.transmit_idx = BUFFER_TRANSMIT;
    ums_state.transmission_active = false;
    ums_state.timestamp = 0;
    ums_state.channel_count = 0;
    ums_state.initialized = true;
    
    return true;
}

bool UMSTrace(const float* pVariable) {
    if (!ums_state.initialized) {
        return false;
    }
    
    if (pVariable == NULL) {
        return false;
    }
    
    if (ums_state.channel_count >= UMS_MAX_CHANNELS) {
        return false; /* Maximum channels reached */
    }
    
    ums_state.trace_vars[ums_state.channel_count] = pVariable;
    ums_state.channel_count++;
    
    return true;
}

bool UMSUpdate(void) {
    if (!ums_state.initialized) {
        return false;
    }
    
    if (ums_state.channel_count == 0) {
        return false;
    }
    
    /* Get pointer to write buffer */
    ums_sample_t *sample = &ums_state.buffers[ums_state.write_idx];
    
    /* Get timestamp - either from time provider or counter */
    uint32_t current_timestamp;
    if (ums_state.time_provider != NULL) {
        /* Use actual time from platform */
        current_timestamp = ums_state.time_provider();
    } else {
        /* Use simple counter */
        enter_critical();
        current_timestamp = ums_state.timestamp++;
        exit_critical();
    }
    
    sample->timestamp = current_timestamp;
    sample->channels = ums_state.channel_count;
    
    /* Copy current values from traced variables */
    for (uint8_t i = 0; i < ums_state.channel_count; i++) {
        sample->values[i] = *(ums_state.trace_vars[i]);
    }
    
    /* CRITICAL SECTION: Atomic buffer swap */
    enter_critical();
    
    /* Swap write and pending buffers */
    uint8_t temp = ums_state.write_idx;
    ums_state.write_idx = ums_state.pending_idx;
    ums_state.pending_idx = temp;
    
    /* Check if we need to start transmission */
    bool start_transmission = false;
    uint8_t tx_idx = 0;
    
    if (!ums_state.transmission_active) {
        ums_state.transmission_active = true;
        start_transmission = true;
        
        /* Swap pending and transmit buffers */
        temp = ums_state.pending_idx;
        ums_state.pending_idx = ums_state.transmit_idx;
        ums_state.transmit_idx = temp;
        
        tx_idx = ums_state.transmit_idx;
    }
    
    exit_critical();
    
    /* Start transmission outside critical section */
    if (start_transmission) {
        ums_sample_t *tx_sample = &ums_state.buffers[tx_idx];
        uint32_t tx_size = ums_sample_size(ums_state.channel_count);
        ums_state.transmit_fn(tx_sample, tx_size);
    }
    
    return true;
}

void UMSTransmissionComplete(void) {
    if (!ums_state.initialized) {
        return;
    }
    
    /* CRITICAL SECTION: Check and update buffer state */
    enter_critical();
    
    bool start_transmission = false;
    uint8_t tx_idx = 0;
    
    /* Check if pending buffer has new data (indices are different) */
    if (ums_state.pending_idx != ums_state.transmit_idx) {
        /* Swap pending and transmit buffers */
        uint8_t temp = ums_state.pending_idx;
        ums_state.pending_idx = ums_state.transmit_idx;
        ums_state.transmit_idx = temp;
        
        start_transmission = true;
        tx_idx = ums_state.transmit_idx;
    } else {
        /* No pending data, mark transmission as inactive */
        ums_state.transmission_active = false;
    }
    
    exit_critical();
    
    /* Start next transmission outside critical section */
    if (start_transmission) {
        ums_sample_t *tx_sample = &ums_state.buffers[tx_idx];
        uint32_t tx_size = ums_sample_size(ums_state.channel_count);
        ums_state.transmit_fn(tx_sample, tx_size);
    }
}

void UMSDestroy(void) {
    enter_critical();
    memset(&ums_state, 0, sizeof(ums_state_t));
    exit_critical();
}

uint8_t UMSGetChannelCount(void) {
    return ums_state.channel_count;
}

bool UMSIsInitialized(void) {
    return ums_state.initialized;
}