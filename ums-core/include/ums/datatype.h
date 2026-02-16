//
//
//

#ifndef UMS_DATATYPE_H
#define UMS_DATATYPE_H

/**
 * Supported scalar datatypes for traced variables.
 *
 * Numeric values are intentionally spaced to allow future extensions
 * within each group without breaking existing serialized handshakes.
 *
 * UMS_FLOAT32  == IEEE 754 single-precision  (4 bytes)
 * UMS_FLOAT64  == IEEE 754 double-precision  (8 bytes)
 *   UMS_DOUBLE is a deprecated alias for UMS_FLOAT64 – kept for backwards
 *   compatibility, do NOT add UMS_DOUBLE64 (redundant and confusing).
 */
typedef enum ums_datatype_t {
    /* Unsigned integer group */
    UMS_UINT8  = 0,
    UMS_UINT16 = 1,
    UMS_UINT32 = 2,
    UMS_UINT64 = 3,

    /* Signed integer group */
    UMS_INT8   = 10,
    UMS_INT16  = 11,
    UMS_INT32  = 12,
    UMS_INT64  = 13,

    /* Floating-point group */
    UMS_FLOAT32 = 20,
    UMS_FLOAT64 = 21,
    UMS_DOUBLE  = UMS_FLOAT64, /**< Deprecated alias – use UMS_FLOAT64 */

    /* Miscellaneous */
    UMS_BOOL   = 30,
    UMS_STRING = 31, /**< Only valid in handshake metadata, not in sample payload */

    UMS_COUNT  = 32, /**< Sentinel – must stay last */
} ums_datatype_t;

/**
 * Returns the size in bytes for a given datatype.
 * Returns 0 for UMS_STRING (variable length) and unknown types.
 *
 * @param [in] type  ums_datatype_t value
 * @return           byte width, or 0 when undefined / variable
 */
static inline uint8_t ums_datatype_size(ums_datatype_t type)
{
    switch (type)
    {
    case UMS_UINT8:
    case UMS_INT8:
    case UMS_BOOL:   return 1U;

    case UMS_UINT16:
    case UMS_INT16:  return 2U;

    case UMS_UINT32:
    case UMS_INT32:
    case UMS_FLOAT32: return 4U;

    case UMS_UINT64:
    case UMS_INT64:
    case UMS_FLOAT64: return 8U;

    case UMS_STRING:
    case UMS_COUNT:
    default:          return 0U;
    }
}

#endif