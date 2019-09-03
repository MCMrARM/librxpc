#include "session.h"

#include <stdio.h>
#include "stream.h"
#include "proto.h"

static void _rxpc_session_send_settings(struct rxpc_session *s);
static void _rxpc_session_root_stream_opened(struct rxpc_stream *stream);
static void _rxpc_session_reply_stream_opened(struct rxpc_stream *stream);

void rxpc_session_init(struct rxpc_session *s) {
    s->session = NULL;
    s->stream_root = NULL;
    s->stream_reply = NULL;
}

void rxpc_session_open(struct rxpc_session *s, nghttp2_session_callbacks *cb, void *transport_data) {
    struct rxpc_stream_callbacks cbs = {0};
    nghttp2_option *opt;
    s->transport_data = transport_data;
    nghttp2_option_new(&opt);
    nghttp2_option_set_no_http_messaging(opt, 1);
    nghttp2_session_client_new2(&s->session, cb, s, opt);
    nghttp2_option_del(opt);
    _rxpc_session_send_settings(s);

    cbs.opened = _rxpc_session_root_stream_opened;
    s->stream_root = rxpc_stream_open(s, &cbs);
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

static void _rxpc_session_send_settings(struct rxpc_session *s) {
    int rv;
    nghttp2_settings_entry iv[1] = {
            {NGHTTP2_SETTINGS_MAX_CONCURRENT_STREAMS, 100}
    };

    rv = nghttp2_submit_settings(s->session, NGHTTP2_FLAG_NONE, iv, 1);
    if (rv != 0)
        fprintf(stderr, "xrpc: could not submit settings: %s", nghttp2_strerror(rv));
}

static void _rxpc_session_root_stream_opened(struct rxpc_stream *stream) {
    struct rxpc_stream_callbacks cbs = {0};
    xpc_object_t dict;

    dict = xpc_dictionary_create(NULL, NULL, 0);
    // optional: xpc_dictionary_set_int64("ServiceVersion", value);
    rxpc_stream_send(stream, RXPC_TYPE_HELLO, 0, 0, dict);
    xpc_free(dict);

    cbs.opened = _rxpc_session_reply_stream_opened;
    stream->session->stream_reply = rxpc_stream_open(stream->session, &cbs);
    rxpc_session_send_pending(stream->session);
}

static void _rxpc_session_reply_stream_opened(struct rxpc_stream *stream) {
    rxpc_stream_send(stream, RXPC_TYPE_HELLO, RXPC_FLAG_REPLY_CHANNEL, 0, NULL);
}

static int _rxpc_session_cb_frame_recv(nghttp2_session *session, const nghttp2_frame *frame, void *user_data) {
    struct rxpc_stream *stream = (struct rxpc_stream *)
            nghttp2_session_get_stream_user_data(session, frame->hd.stream_id);
    switch (frame->hd.type) {
        case NGHTTP2_HEADERS:
            fprintf(stderr, "rxpc: headers received\n");
            if (stream->cbs.opened)
                stream->cbs.opened(stream);
            break;
    }
    return 0;
}

static int _rxpc_session_cb_data_chunk_recv(nghttp2_session *session, uint8_t flags, int32_t stream_id,
        const uint8_t *data, size_t len, void *user_data) {
#ifdef DEBUG_RAW_IO
    printf("rxpc: stream %d: received data chunk\n", stream_id);
    for (int i = 0; i < len; i++)
        printf("%02x ", data[i]);
    printf("\n");
#endif
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

