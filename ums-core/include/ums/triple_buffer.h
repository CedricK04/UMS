//
//
//

#ifndef UMS_TRIPLE_BUFFER_H
#define UMS_TRIPLE_BUFFER_H

#include "stdint.h"

#include "ums/datatype.h"

#ifdef __cplusplus
extern "C" {
#endif

#define UMS_MAX_CHANNELS    16
#define UMS_MAX_FRAME_SIZE  ((UMS_MAX_CHANNELS * 8U) + sizeof(uint32_t))

    /**
     * Metadata from each channel that is traced.
     * var_ptr points to the address of the traced variable.
     * var_type refers to the datatype of the traced variable.
     * var_name_ptr is a char array containing the name of the variable, only to be used in the handshake msg.
     */
    typedef struct data_channel_t
    {
        void*           var_ptr;
        ums_datatype_t  var_type;
        const char*     var_name_ptr;
    } data_channel_t;

    /**
     * Datatype to cover the maximum size needed by the triple buffer.
     * Size = 132 bytes at 4 alignment.
     * timestamp = device specific timestamp of the sample creation time.
     * data = value of its traced variable, is an array. Each index in 1 byte.
     */
    typedef struct sample_packet_t
    {
        uint32_t    timestamp;
        uint8_t     data[UMS_MAX_FRAME_SIZE - sizeof(uint32_t)];
    } sample_packet_t;

#ifdef __cplusplus
}
#endif

#endif