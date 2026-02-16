//
//
//

#ifndef UMS_CORE_H
#define UMS_CORE_H

#include "ums/datatype.h"
#include "ums/error.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Function pointer to user-defined transmit function, e.g. "HAL_UART_TRANSMIT_DMA()"
 * Requires pointer to the data to be sent: void *data_ptr.
 * Requires uint16_t length in bytes of what to send.
 */
typedef void (*transmit_function)(void *data_ptr, uint16_t length);

/**
 * Setup for UMS, requires data transmission function_ptr.
 * @param [in] transmit_function_ptr function pointer to user-defined transmit function.
 * @return ums_err_t error code. 1 = UMS_SUCCESS.
 */
ums_err_t ums_setup(transmit_function transmit_function_ptr);

/**
 * Enabling tracing for a variable, to be called for each variable the user wants to trace.
 * @param [in] var_ptr pointer to the variable (must remain in scope).
 * @param [in] var_name_ptr string alias for traced variable.
 * @param [in] var_type datatype of the traced variable.
 * @return ums_err_t error code. 1 = UMS_SUCCESS.
 */
ums_err_t ums_trace(void *var_ptr, const char *var_name_ptr, ums_datatype_t var_type);

/**
 * Updates values of traced variables.
 * Creates data packet including timestamp.
 * Writes data packet to transmit buffer for automatic transmission.
 * @return ums_err_t error code. 1 = UMS_SUCCESS.
 */
ums_err_t ums_update(void);

/**
 * Swap the read and spare indexes for the triple buffer once a transfer was completed.
 * To be called on transfer complete, e.g. HAL_UART_TxCpltCallback()
 */
void ums_transfer_complete_callback(void);

/**
 * Clean-up for UMS, to be called when exiting intended scope.
 * @return ums_err_t error code. 1 = UMS_SUCCESS.
 */
ums_err_t ums_destroy(void);

/**
 * Enter critical section, default no implementation. Platform specific.
 */
void ums_platform_enter_critical(void);

/**
 * Exit critical section, default no implementation. Platform specific.
 */
void ums_platform_exit_critical(void);

/**
 * Get sample timestamp, default = 0U, should be platform specific.
 * @return uint32_t timestamp value.
 */
uint32_t ums_platform_get_timestamp(void);

#ifdef __cplusplus
}
#endif

#endif