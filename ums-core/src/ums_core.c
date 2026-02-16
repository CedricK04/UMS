//
//
//

#include "string.h"

#include "ums/triple_buffer.h"
#include "ums/ums_core.h"

/**
 * triple buffer array to store the sample_packets in.
 * During transmission, send the data on the address of the index of the array that is the write index,
 * but calculate the size of the data in that function and only transmit the necessary length for optimization.
 */
sample_packet_t     ums_triple_buffer[3];
volatile uint8_t    idx_write           = 0;
volatile uint8_t    idx_read            = 1;
volatile uint8_t    idx_spare           = 2;

static transmit_function s_transmit_function_ptr;

data_channel_t      registry[UMS_MAX_CHANNELS];
uint8_t             channel_count       = 0;
bool                g_ums_initialized   = false;
uint16_t            g_actual_frame_size = sizeof(uint32_t);
volatile bool       g_dma_busy          = false;

/**
 * Creates a new sample with the current values of the traced variables.
 * Writes the sample packet to the write index and swaps the write index with the spare index.
 * @return ums_err_t error code. 1 = UMS_SUCCESS.
 */
static ums_err_t ums_create_sample(void)
{
    if (channel_count == 0)
    {
        return UMS_RANGE_ERROR;
    }

    ums_triple_buffer[idx_write].timestamp = ums_platform_get_timestamp();
    uint16_t offset = 0;

    for (uint8_t i = 0; i < channel_count; i++)
    {
        const uint8_t var_size = ums_datatype_size(registry[i].var_type);
        memcpy(&ums_triple_buffer[idx_write].data[offset], registry[i].var_ptr, var_size);
        offset += var_size;
    }

    ums_platform_enter_critical();
    const uint8_t temp = idx_spare;
    idx_spare = idx_write;
    idx_write = temp;
    ums_platform_exit_critical();

    g_dma_busy = true;
    s_transmit_function_ptr((void*)&ums_triple_buffer[idx_spare], g_actual_frame_size);

    return UMS_SUCCESS;
}

ums_err_t ums_setup(const transmit_function transmit_function_ptr)
{
    if (!transmit_function_ptr)
    {
        return UMS_NULL_POINTER;
    }
    s_transmit_function_ptr = transmit_function_ptr;
    g_ums_initialized = true;

    return UMS_SUCCESS;
}

ums_err_t ums_trace(void *var_ptr, const char *var_name_ptr, const ums_datatype_t var_type)
{
    if (!g_ums_initialized)
    {
        return UMS_NOT_INITIALIZED;
    }
    if (!var_ptr)
    {
        return UMS_INVALID_VARIABLE_REGISTRATION;
    }
    if (!var_name_ptr)
    {
        return UMS_NULL_POINTER;
    }
    if (ums_datatype_size(var_type) == 0)
    {
        return UMS_INVALID_PARAMETER;
    }
    if (channel_count == UMS_MAX_CHANNELS)
    {
        return UMS_RANGE_ERROR;
    }

    registry[channel_count].var_ptr = var_ptr;
    registry[channel_count].var_type = var_type;
    registry[channel_count].var_name_ptr = var_name_ptr;

    channel_count++;
    g_actual_frame_size += ums_datatype_size(var_type);

    return UMS_SUCCESS;
}

ums_err_t ums_update(void)
{
    if (!g_ums_initialized)
    {
        return UMS_NOT_INITIALIZED;
    }
    if (channel_count == 0)
    {
        return UMS_RANGE_ERROR;
    }
    if (g_dma_busy)
    {
        return UMS_BUFFER_FULL;
    }

    if (ums_create_sample() != UMS_SUCCESS)
    {
        return UMS_SAMPLING_ERROR;
    }
    return UMS_SUCCESS;
}

void ums_transfer_complete_callback(void)
{
    const uint8_t temp = idx_spare;
    idx_spare = idx_read;
    idx_read = temp;
    g_dma_busy = false;
}

ums_err_t ums_destroy(void)
{
    if (!g_ums_initialized)
    {
        return UMS_NOT_INITIALIZED;
    }

    for (uint8_t i = 0; i < channel_count; i++)
    {
        registry[i].var_ptr = nullptr;
        registry[i].var_name_ptr = nullptr;
        registry[i].var_type = 0;
    }

    g_dma_busy = false;
    channel_count = 0;
    g_ums_initialized = false;
    g_actual_frame_size = sizeof(uint32_t);

    idx_write = 0;
    idx_read = 1;
    idx_spare = 2;

    return UMS_SUCCESS;
}

__attribute__((weak)) void ums_platform_enter_critical(void)
{
    //
}

__attribute__((weak)) void ums_platform_exit_critical(void)
{
    //
}

__attribute__((weak)) uint32_t ums_platform_get_timestamp(void)
{
    return 0U;
}