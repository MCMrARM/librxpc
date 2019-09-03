#ifndef RXPC_STREAM_H
#define RXPC_STREAM_H

#include <stdint.h>

struct rxpc_session;
struct rxpc_stream;

typedef void (*rxpc_stream_opened_cb)(struct rxpc_stream *);

struct rxpc_stream_callbacks {
    rxpc_stream_opened_cb opened;
};
struct rxpc_stream {
    struct rxpc_session *session;
    int32_t id;
    struct rxpc_stream_callbacks cbs;
};

struct rxpc_stream *rxpc_stream_open(struct rxpc_session *session, struct rxpc_stream_callbacks *cbs);

#endif //RXPC_STREAM_H