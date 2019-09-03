#ifndef RXPC_STREAM_H
#define RXPC_STREAM_H

#include <stdint.h>
#include <stddef.h>

struct rxpc_session;
struct rxpc_stream;
struct rxpc_stream_pending_data;

typedef void (*rxpc_stream_opened_cb)(struct rxpc_stream *);

struct rxpc_stream_callbacks {
    rxpc_stream_opened_cb opened;
};
struct rxpc_stream {
    struct rxpc_session *session;
    int32_t id;
    struct rxpc_stream_callbacks cbs;
    struct rxpc_stream_pending_data *send_data_begin, *send_data_end;
};
struct rxpc_stream_pending_data {
    struct rxpc_stream_pending_data *next;
    size_t offset, size;
    char data[];
};

struct rxpc_stream *rxpc_stream_open(struct rxpc_session *session, struct rxpc_stream_callbacks *cbs);
struct rxpc_stream_pending_data *rxpc_stream_send_alloc(size_t size);
void rxpc_stream_send_raw(struct rxpc_stream *stream, struct rxpc_stream_pending_data *buffer);

#endif //RXPC_STREAM_H