#include "stream.h"

#include <malloc.h>
#include <string.h>
#include "session.h"
#include "proto.h"
#include <xpc/xpc_serialization.h>

static ssize_t _rxpc_stream_read(nghttp2_session *session, int32_t stream_id, uint8_t *buf, size_t length,
        uint32_t *data_flags, nghttp2_data_source *source, void *user_data);

struct rxpc_stream *rxpc_stream_open(struct rxpc_session *session, struct rxpc_stream_callbacks *cbs) {
    nghttp2_data_provider provider;
    struct rxpc_stream *ret = malloc(sizeof(struct rxpc_stream));
    ret->session = session;
    ret->cbs = *cbs;
    ret->send_data_begin = NULL;
    ret->send_data_end = NULL;
    provider.source.ptr = ret;
    provider.read_callback = _rxpc_stream_read;
    ret->id = nghttp2_submit_request(session->session, NULL, NULL, 0, &provider, ret);
    if (ret->id <= 0) {
        fprintf(stderr, "rxpc: failed to open new stream: %s\n", nghttp2_strerror(ret->id));
        free(ret);
        return NULL;
    }
    return ret;
}

struct rxpc_stream_pending_data *rxpc_stream_send_alloc(size_t size) {
    struct rxpc_stream_pending_data *ret = malloc(sizeof(struct rxpc_stream_pending_data) + size);
    ret->next = NULL;
    ret->offset = 0;
    ret->size = size;
    return ret;
}

void rxpc_stream_send_free(struct rxpc_stream_pending_data *buf) {
    free(buf);
}

void rxpc_stream_send_raw(struct rxpc_stream *stream, struct rxpc_stream_pending_data *buffer) {
    if (!buffer->size) {
        rxpc_stream_send_free(buffer);
        return;
    }
    if (!stream->send_data_begin)
        stream->send_data_begin = buffer;
    if (stream->send_data_end)
        stream->send_data_end->next = buffer;
    stream->send_data_end = buffer;
    nghttp2_session_resume_data(stream->session->session, stream->id);
}

void rxpc_stream_send(struct rxpc_stream *stream, uint8_t type, uint16_t flags, uint64_t msg_id, xpc_object_t data) {
    struct rxpc_msg_header *header;
    size_t data_size = data ? xpc_serialized_size(data) : 0;
    struct rxpc_stream_pending_data *sdata = rxpc_stream_send_alloc(
            sizeof(struct rxpc_msg_header) + data_size);
    header = (struct rxpc_msg_header *) sdata->data;
    header->magic = RXPC_MSG_MAGIC;
    header->version = RXPC_MSG_VERSION;
    header->type = type;
    header->flags = flags;
    header->msg_id = msg_id;
    header->length = data_size;
    if (data)
        xpc_serialize(data, &sdata->data[sizeof(struct rxpc_msg_header)]);
    rxpc_stream_send_raw(stream, sdata);
}

static ssize_t _rxpc_stream_read(nghttp2_session *session, int32_t stream_id, uint8_t *buf, size_t length,
        uint32_t *data_flags, nghttp2_data_source *source, void *user_data) {
    struct rxpc_stream *stream = (struct rxpc_stream *) source->ptr;
    struct rxpc_stream_pending_data *data = stream->send_data_begin;
    if (!data)
        return NGHTTP2_ERR_DEFERRED;
    if (length > data->size - data->offset)
        length = data->size - data->offset;
#ifdef DEBUG_RAW_IO
    printf("rxpc: stream %d: data write\n", stream_id);
    for (int i = 0; i < length; i++)
        printf("%02x ", data->data[data->offset + i]);
    printf("\n");
#endif
    memcpy(buf, &data->data[data->offset], length);
    data->offset += length;
    if (data->offset == data->size) {
        stream->send_data_begin = data->next;
        if (stream->send_data_end == data)
            stream->send_data_end = NULL;
        rxpc_stream_send_free(data);
    }
    return length;
}