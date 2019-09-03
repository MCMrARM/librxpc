#ifndef RXPC_HTTP2_H
#define RXPC_HTTP2_H

#include <nghttp2/nghttp2.h>

struct rxpc_session {
    nghttp2_session *session;
    void *transport_data;
};

void rxpc_session_init(struct rxpc_session *s);
void rxpc_session_open(struct rxpc_session *s, nghttp2_session_callbacks *cb, void *transport_data);
int rxpc_session_send_pending(struct rxpc_session *s);
void rxpc_session_terminate(struct rxpc_session *s);

nghttp2_session_callbacks *rxpc_session_create_callbacks();

#endif //RXPC_HTTP2_H
