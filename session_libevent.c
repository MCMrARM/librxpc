#include "session_libevent.h"

#include <event2/event.h>
#include <event2/buffer.h>
#include <netinet/tcp.h>
#include "session.h"

static void _rxpc_ev_read_callback(struct bufferevent *bev, void *ptr) {
    struct rxpc_libevent_session *session_data = (struct rxpc_libevent_session *) ptr;
    ssize_t readlen;
    struct evbuffer *input = bufferevent_get_input(bev);
    size_t datalen = evbuffer_get_length(input);
    unsigned char *data = evbuffer_pullup(input, -1);

    readlen = nghttp2_session_mem_recv(session_data->session.session, data, datalen);
    if (readlen < 0) {
        fprintf(stderr, "rxpc: read error: %s\n", nghttp2_strerror((int)readlen));
        goto fail;
    }
    if (evbuffer_drain(input, (size_t)readlen) != 0) {
        fprintf(stderr, "rxpc: read error: evbuffer_drain failed\n");
        goto fail;
    }
    if (rxpc_session_send_pending(&session_data->session) != 0)
        goto fail;
    return;

fail:
    rxpc_session_terminate(&session_data->session);
}

static void _rxpc_ev_write_callback(struct bufferevent *bev, void *ptr) {
    struct rxpc_libevent_session *session_data = (struct rxpc_libevent_session *) ptr;
    if (nghttp2_session_want_read(session_data->session.session) == 0 &&
        nghttp2_session_want_write(session_data->session.session) == 0 &&
        evbuffer_get_length(bufferevent_get_output(bev)) == 0) {
        rxpc_session_send_pending(&session_data->session);
    }
}

static ssize_t _rxpc_session_cb_send(nghttp2_session *session, const uint8_t *data,
        size_t length, int flags, void *user_data);

static void _rxpc_ev_event_callback(struct bufferevent *bev, short events, void *ptr) {
    struct rxpc_libevent_session *session_data = (struct rxpc_libevent_session *) ptr;
    if (events & BEV_EVENT_CONNECTED) {
        int fd = bufferevent_getfd(bev);
        int val;
        nghttp2_session_callbacks *callbacks;

        val = 1;
        setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char *)&val, sizeof(val));

        callbacks = rxpc_session_create_callbacks(&session_data->session);
        nghttp2_session_callbacks_set_send_callback(callbacks, _rxpc_session_cb_send);
        rxpc_session_open(&session_data->session, callbacks, session_data);
        nghttp2_session_callbacks_del(callbacks);

        if (rxpc_session_send_pending(&session_data->session))
            rxpc_session_terminate(&session_data->session);
    } else {
        rxpc_session_terminate(&session_data->session);
    }
}

struct rxpc_libevent_session *rxpc_libevent_session_create(struct event_base *evbase) {
    struct rxpc_libevent_session *session_data = malloc(sizeof(struct rxpc_libevent_session));
    rxpc_session_init(&session_data->session);
    session_data->bev = bufferevent_socket_new(evbase, -1, BEV_OPT_DEFER_CALLBACKS | BEV_OPT_CLOSE_ON_FREE);
    bufferevent_enable(session_data->bev, EV_READ | EV_WRITE);
    bufferevent_setcb(session_data->bev, _rxpc_ev_read_callback, _rxpc_ev_write_callback, _rxpc_ev_event_callback,
            session_data);
    return session_data;
}


static ssize_t _rxpc_session_cb_send(nghttp2_session *session, const uint8_t *data, size_t length,
        int flags, void *user_data) {
    struct rxpc_session *r_session = (struct rxpc_session *) user_data;
    struct rxpc_libevent_session *session_data = (struct rxpc_libevent_session *) r_session->transport_data;
    bufferevent_write(session_data->bev, data, length);
    return (ssize_t) length;
}
