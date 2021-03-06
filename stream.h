#ifndef RXPC_STREAM_H
#define RXPC_STREAM_H

#include <stdint.h>
#include <stddef.h>
#include <xpc/xpc.h>
#include "proto.h"

struct rxpc_session;
struct rxpc_stream;
struct rxpc_stream_pending_data;

typedef void (*rxpc_stream_opened_cb)(struct rxpc_stream *);
typedef void (*rxpc_stream_message_cb)(struct rxpc_stream *, struct rxpc_msg_header *header, const void *data);

struct rxpc_stream_callbacks {
    rxpc_stream_opened_cb opened;
    rxpc_stream_message_cb message;
};
struct rxpc_stream {
    struct rxpc_session *session;
    int32_t id;
    struct rxpc_stream_callbacks cbs;
    struct rxpc_stream_pending_data *send_data_begin, *send_data_end;

    char recv_header_data[sizeof(struct rxpc_msg_header)];
    size_t recv_header_pos;
    char *recv_data;
    size_t recv_data_pos;
};
struct rxpc_stream_pending_data {
    struct rxpc_stream_pending_data *next;
    size_t offset, size;
    uint8_t data[];
};

struct rxpc_stream *rxpc_stream_open(struct rxpc_session *session, struct rxpc_stream_callbacks *cbs);
struct rxpc_stream_pending_data *rxpc_stream_send_alloc(size_t size);
void rxpc_stream_send_raw(struct rxpc_stream *stream, struct rxpc_stream_pending_data *buffer);
void rxpc_stream_send(struct rxpc_stream *stream, uint8_t type, uint16_t flags, uint64_t msg_id, xpc_object_t data);

#endif //RXPC_STREAM_H