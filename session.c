#include "session.h"

#include <stdio.h>

static void _rxpc2_session_send_settings(struct rxpc_session *s);

void rxpc_session_init(struct rxpc_session *s) {
    s->session = NULL;
}

void rxpc_session_open(struct rxpc_session *s, nghttp2_session_callbacks *cb, void *transport_data) {
    nghttp2_option *opt;
    s->transport_data = transport_data;
    nghttp2_option_new(&opt);
    nghttp2_option_set_no_http_messaging(opt, 1);
    nghttp2_session_client_new2(&s->session, cb, s, opt);
    nghttp2_option_del(opt);
    _rxpc2_session_send_settings(s);

    // TODO: Open root channel
}

void rxpc_session_terminate(struct rxpc_session *s) {
    // TODO:
}

int rxpc_session_send_pending(struct rxpc_session *s) {
    int ret = nghttp2_session_send(s->session);
    if (ret != 0) {
        fprintf(stderr, "xrpc: session send error: %s\n", nghttp2_strerror(ret));
        return -1;
    }
    return 0;
}

static void _rxpc2_session_send_settings(struct rxpc_session *s) {
    int rv;
    nghttp2_settings_entry iv[1] = {
            {NGHTTP2_SETTINGS_MAX_CONCURRENT_STREAMS, 100}
    };

    rv = nghttp2_submit_settings(s->session, NGHTTP2_FLAG_NONE, iv, 1);
    if (rv != 0)
        fprintf(stderr, "xrpc: could not submit settings: %s", nghttp2_strerror(rv));
}


static int _rxpc_session_cb_frame_recv(nghttp2_session *session, const nghttp2_frame *frame, void *user_data) {
    // struct rxpc_session_data *session_data = (struct rxpc_session_data *) user_data;
    switch (frame->hd.type) {
        case NGHTTP2_HEADERS:
            fprintf(stderr, "rxpc: headers received\n");
            break;
    }
    return 0;
}

static int _rxpc_session_cb_data_chunk_recv(nghttp2_session *session, uint8_t flags, int32_t stream_id,
        const uint8_t *data, size_t len, void *user_data) {
    fprintf(stderr, "rxpc: stream %d: received data chunk\n", stream_id);
    return 0;
}

static int _rxpc_session_cb_stream_close(nghttp2_session *session, int32_t stream_id, nghttp2_error_code error_code,
        void *user_data) {
    int ret;

    fprintf(stderr, "rxpc: stream %d closed with code %d\n", stream_id, error_code);
    ret = nghttp2_session_terminate_session(session, NGHTTP2_NO_ERROR);
    if (ret != 0)
        return NGHTTP2_ERR_CALLBACK_FAILURE;
    return 0;
}


nghttp2_session_callbacks *rxpc_session_create_callbacks() {
    nghttp2_session_callbacks *callbacks;

    nghttp2_session_callbacks_new(&callbacks);
    nghttp2_session_callbacks_set_on_frame_recv_callback(callbacks, _rxpc_session_cb_frame_recv);
    nghttp2_session_callbacks_set_on_data_chunk_recv_callback(callbacks, _rxpc_session_cb_data_chunk_recv);
    nghttp2_session_callbacks_set_on_stream_close_callback(callbacks, _rxpc_session_cb_stream_close);

    return callbacks;
}

