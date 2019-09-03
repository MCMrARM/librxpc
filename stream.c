#include "stream.h"

#include <malloc.h>
#include "session.h"

static ssize_t _rxpc_stream_read(nghttp2_session *session, int32_t stream_id, uint8_t *buf, size_t length,
        uint32_t *data_flags, nghttp2_data_source *source, void *user_data);

struct rxpc_stream *rxpc_stream_open(struct rxpc_session *session, struct rxpc_stream_callbacks *cbs) {
    nghttp2_data_provider provider;
    struct rxpc_stream *ret = malloc(sizeof(struct rxpc_stream));
    ret->session = session;
    ret->cbs = *cbs;
    provider.source.ptr = session;
    provider.read_callback = _rxpc_stream_read;
    ret->id = nghttp2_submit_request(session->session, NULL, NULL, 0, &provider, ret);
    if (ret->id <= 0) {
        fprintf(stderr, "rxpc: failed to open new stream: %s\n", nghttp2_strerror(ret->id));
        free(ret);
        return NULL;
    }
    return ret;
}

static ssize_t _rxpc_stream_read(nghttp2_session *session, int32_t stream_id, uint8_t *buf, size_t length,
        uint32_t *data_flags, nghttp2_data_source *source, void *user_data) {
    *data_flags = NGHTTP2_DATA_FLAG_EOF | NGHTTP2_DATA_FLAG_NO_END_STREAM;
    return 0;
}