//
//
//

#ifndef UMS_ERROR_H
#define UMS_ERROR_H

typedef enum ums_err_t {
    UMS_FAIL             = 0,
    UMS_SUCCESS          = 1,

    UMS_NULL_POINTER,
    UMS_RANGE_ERROR,
    UMS_NOT_INITIALIZED,
    UMS_INVALID_VARIABLE_REGISTRATION,
    UMS_SAMPLING_ERROR,
    UMS_INVALID_PARAMETER,
    UMS_BUFFER_FULL,
} ums_err_t;

#endif