/* -*- Mode: C; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2010-2013 Couchbase, Inc.
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 */

/**
 * This file contains the protocol handling and callback dispatching for
 * commands.
 *
 * @author Trond Norbye
 * @todo add more documentation
 */
#include "internal.h"

static void swallow_command(lcb_server_t *c,
                            protocol_binary_response_header *header,
                            int was_connected)
{
    lcb_size_t nr;
    protocol_binary_request_header req;
    if (was_connected &&
            (header->response.opcode != PROTOCOL_BINARY_CMD_STAT ||
             header->response.keylen == 0)) {
        nr = lcb_ringbuffer_read(&c->cmd_log, req.bytes, sizeof(req));
        lcb_assert(nr == sizeof(req));
        lcb_ringbuffer_consumed(&c->cmd_log, ntohl(req.request.bodylen));
        lcb_ringbuffer_consumed(&c->output_cookies,
                                sizeof(struct lcb_command_data_st));
    }
}

/**
 * Returns 1 if retried, 0 if the command should fail, -1 for an internal
 * error
 */
static int handle_not_my_vbucket(lcb_server_t *c,
                                 protocol_binary_request_header *oldreq,
                                 struct lcb_command_data_st *oldct)
{
    int idx;
    char *body;
    lcb_size_t nbody, nr;
    lcb_server_t *new_srv;
    struct lcb_command_data_st ct;
    protocol_binary_request_header req;
    hrtime_t now;

    if (c->instance->compat.type == LCB_CACHED_CONFIG) {
        lcb_schedule_config_cache_refresh(c->instance);
    }

    /* re-schedule command to new server */
    idx = vbucket_found_incorrect_master(c->instance->vbucket_config,
                                         ntohs(oldreq->request.vbucket),
                                         (int)c->index);

    if (idx == -1) {
        return 0;
    }

    now = gethrtime();

    if (oldct->real_start) {
        hrtime_t min_ok = now - (c->connection.timeout.usec * 1000);
        if (oldct->real_start < min_ok) {
            /** Timed out in a 'natural' manner */
            return 0;
        }
    }

    req = *oldreq;

    lcb_assert((lcb_size_t)idx < c->instance->nservers);
    new_srv = c->instance->servers + idx;

    nr = lcb_ringbuffer_read(&c->cmd_log, req.bytes, sizeof(req));
    lcb_assert(nr == sizeof(req));

    req.request.opaque = ++c->instance->seqno;
    nbody = ntohl(req.request.bodylen);
    body = malloc(nbody);
    if (body == NULL) {
        lcb_error_handler(c->instance, LCB_CLIENT_ENOMEM, NULL);
        return -1;
    }
    nr = lcb_ringbuffer_read(&c->cmd_log, body, nbody);
    lcb_assert(nr == nbody);
    nr = lcb_ringbuffer_read(&c->output_cookies, &ct, sizeof(ct));
    lcb_assert(nr == sizeof(ct));

    /* Preserve the cookie and reset timestamp for the command. This
     * means that the library will retry the command until it will
     * get code different from LCB_NOT_MY_VBUCKET */
    if (!ct.real_start) {
        ct.real_start = ct.start;
    }
    ct.start = now;

    lcb_server_retry_packet(new_srv, &ct, &req, sizeof(req));
    /* FIXME dtrace instrumentation */
    lcb_server_write_packet(new_srv, body, nbody);
    lcb_server_end_packet(new_srv);
    lcb_server_send_packets(new_srv);
    free(body);

    return 1;
}

static int lcb_parse_single_with_specialized_callbacks(lcb_server_t *c,
                                                       hrtime_t stop)
{
    protocol_binary_request_header req;
    protocol_binary_response_header header;
    lcb_size_t nr;
    char *packet;
    lcb_size_t packetsize;
    struct lcb_command_data_st ct;
    lcb_connection_t conn = &c->connection;

    if (lcb_ringbuffer_ensure_alignment(&conn->input.buffer->ringbuffer) != 0) {
        lcb_error_handler(c->instance, LCB_EINTERNAL,
                          NULL);
        return -1;
    }

    nr = lcb_ringbuffer_peek(&conn->input.buffer->ringbuffer,
                             header.bytes, sizeof(header));
    if (nr < sizeof(header)) {
        return 0;
    }

    packetsize = ntohl(header.response.bodylen) + (lcb_uint32_t)sizeof(header);
    if (conn->input.buffer->ringbuffer.nbytes < packetsize) {
        return 0;
    }

    /* Is it already timed out? */
    nr = lcb_ringbuffer_peek(&c->cmd_log, req.bytes, sizeof(req));
    if (nr < sizeof(req) || /* the command log doesn't know about it */
            (header.response.opaque < req.request.opaque &&
             header.response.opaque > 0)) { /* sasl comes with zero opaque */
        /* already processed. */
        lcb_ringbuffer_consumed(&conn->input.buffer->ringbuffer, packetsize);
        return 1;
    }

    packet = conn->input.buffer->ringbuffer.read_head;
    /* we have everything! */

    if (!lcb_ringbuffer_is_continous(&conn->input.buffer->ringbuffer,
                                     LCB_RINGBUFFER_READ, packetsize)) {
        /* The buffer isn't continous.. for now just copy it out and
        ** operate on the copy ;)
        */
        if ((packet = malloc(packetsize)) == NULL) {
            lcb_error_handler(c->instance, LCB_CLIENT_ENOMEM, NULL);
            return -1;
        }
        nr = lcb_ringbuffer_read(&conn->input.buffer->ringbuffer, packet, packetsize);
        if (nr != packetsize) {
            lcb_error_handler(c->instance, LCB_EINTERNAL,
                              NULL);
            free(packet);
            return -1;
        }
    }

    nr = lcb_ringbuffer_peek(&c->output_cookies, &ct, sizeof(ct));
    if (nr != sizeof(ct)) {
        lcb_error_handler(c->instance, LCB_EINTERNAL,
                          NULL);
        if (packet != conn->input.buffer->ringbuffer.read_head) {
            free(packet);
        }
        return -1;
    }
    ct.vbucket = ntohs(req.request.vbucket);

    switch (header.response.magic) {
    case PROTOCOL_BINARY_REQ:
        /*
         * The only way to get request packets is if someone started
         * to send us TAP requests, and we don't support that anymore
         */
        lcb_error_handler(c->instance, LCB_EINTERNAL,
                          "Protocol error. someone sent us a command!");
        return -1;
    case PROTOCOL_BINARY_RES: {
        int was_connected = c->connection_ready;
        if (lcb_server_purge_implicit_responses(c, header.response.opaque, stop, 0) != 0) {
            if (packet != conn->input.buffer->ringbuffer.read_head) {
                free(packet);
            }
            return -1;
        }

        if (c->instance->histogram) {
            lcb_record_metrics(c->instance, stop - ct.start,
                               header.response.opcode);
        }

        if (ntohs(header.response.status) != PROTOCOL_BINARY_RESPONSE_NOT_MY_VBUCKET
                || header.response.opcode == CMD_GET_REPLICA
                || header.response.opcode == CMD_OBSERVE) {
            if (lcb_dispatch_response(c, &ct, (void *)packet) == -1) {
                /*
                 * Internal error.. we received an unsupported response
                 * id. This should _ONLY_ happen at development time because
                 * we won't receive response packets with other opcodes
                 * than we send. Let's abort here to make it easy for
                 * the developer to know what happened..
                 */
                lcb_error_handler(c->instance, LCB_EINTERNAL,
                                  "Received unknown command response");
                abort();
                return -1;
            }

            /* keep command and cookie until we get complete STAT response */
            swallow_command(c, &header, was_connected);

        } else {
            int rv = handle_not_my_vbucket(c, &req, &ct);

            if (rv == -1) {
                return -1;

            } else if (rv == 0) {
                lcb_dispatch_response(c, &ct, (void *)packet);
                swallow_command(c, &header, was_connected);
            }

        }
        break;
    }

    default:
        lcb_error_handler(c->instance,
                          LCB_PROTOCOL_ERROR,
                          NULL);
        if (packet != conn->input.buffer->ringbuffer.read_head) {
            free(packet);
        }
        return -1;
    }

    if (packet != conn->input.buffer->ringbuffer.read_head) {
        free(packet);
    } else {
        lcb_ringbuffer_consumed(&conn->input.buffer->ringbuffer, packetsize);
    }
    return 1;
}

static int lcb_parse_single_with_packet_forward(lcb_server_t *c,
                                                hrtime_t stop)
{
    protocol_binary_request_header req;
    protocol_binary_response_header header;
    lcb_size_t nr;
    lcb_size_t avail;
    int was_connected = c->connection_ready;
    uint16_t status;
    lcb_size_t packetsize;
    struct lcb_command_data_st ct;
    lcb_connection_t conn = &c->connection;
    lcb_t instance = c->instance;

    /* Check if we have the entire packet */
    nr = lcb_ringbuffer_peek(&conn->input.buffer->ringbuffer,
                             header.bytes, sizeof(header));
    if (nr < sizeof(header)) {
        return 0;
    }

    packetsize = ntohl(header.response.bodylen) + (lcb_uint32_t)sizeof(header);
    avail = lcb_ringbuffer_get_nbytes(&conn->input.buffer->ringbuffer);

    if (avail < packetsize) {
        /* We need more data! */
        return 0;
    }

    /* Is it already timed out? */
    nr = lcb_ringbuffer_peek(&c->cmd_log, req.bytes, sizeof(req));
    if (nr < sizeof(req) || /* the command log doesn't know about it */
            (header.response.opaque < req.request.opaque &&
             header.response.opaque > 0)) { /* sasl comes with zero opaque */
        /* already processed. */
        lcb_ringbuffer_consumed(&conn->input.buffer->ringbuffer, packetsize);
        return 1;
    }

    /* We've got the entire packet!! */
    assert(header.response.magic == PROTOCOL_BINARY_RES);

    nr = lcb_ringbuffer_peek(&c->output_cookies, &ct, sizeof(ct));
    if (nr != sizeof(ct)) {
        lcb_error_handler(c->instance, LCB_EINTERNAL, NULL);
        return -1;
    }
    ct.vbucket = ntohs(req.request.vbucket);

    if (lcb_server_purge_implicit_responses(c, header.response.opaque, stop, 0)) {
        return -1;
    }

    if (c->instance->histogram) {
        lcb_record_metrics(c->instance, stop - ct.start,
                           header.response.opcode);
    }

    status = ntohs(header.response.status);
    if (status != PROTOCOL_BINARY_RESPONSE_NOT_MY_VBUCKET ||
        header.response.opcode == CMD_GET_REPLICA ||
        header.response.opcode == CMD_OBSERVE) {
        /* Prepare the callback data */
        lcb_packet_fwd_resp_t resp;
        lcb_error_t err = LCB_SUCCESS;
        resp.version = 0;
        resp.v.v0.buffer = conn->input.buffer;
        lcb_ringbuffer_get_iov(&resp.v.v0.buffer->ringbuffer,
                               LCB_RINGBUFFER_READ,
                               resp.v.v0.iov);
        /* The IOV should only contain the exact bits for the packet */
        if (resp.v.v0.iov[0].iov_len > packetsize) {
            resp.v.v0.iov[0].iov_len = packetsize;
            resp.v.v0.iov[1].iov_len = 0;
        } else {
            resp.v.v0.iov[1].iov_len = packetsize - resp.v.v0.iov[0].iov_len;
        }

        /* fire the callback */
        conn->input.locked |= instance->callbacks.packet_fwd(instance,
                                                             ct.cookie,
                                                             err,
                                                             &resp);
        lcb_ringbuffer_consumed(&conn->input.buffer->ringbuffer, packetsize);

        /* keep command and cookie until we get complete STAT response */
        swallow_command(c, &header, was_connected);

    } else {
        int rv = handle_not_my_vbucket(c, &req, &ct);
        if (rv == -1) {
            return -1;
        } else if (rv == 0) {
            /* Prepare the callback data */
            lcb_packet_fwd_resp_t resp;
            lcb_error_t err = LCB_SUCCESS;
            resp.version = 0;
            resp.v.v0.buffer = conn->input.buffer;
            lcb_ringbuffer_get_iov(&resp.v.v0.buffer->ringbuffer,
                                   LCB_RINGBUFFER_READ,
                                   resp.v.v0.iov);

            /* The IOV should only contain the exact bits for the packet */
            if (resp.v.v0.iov[0].iov_len > packetsize) {
                resp.v.v0.iov[0].iov_len = packetsize;
                resp.v.v0.iov[1].iov_len = 0;
            } else {
                resp.v.v0.iov[1].iov_len = packetsize - resp.v.v0.iov[0].iov_len;
            }

            /* fire the callback */
            conn->input.locked |= instance->callbacks.packet_fwd(instance,
                                                                 ct.cookie,
                                                                 err,
                                                                 &resp);
            lcb_ringbuffer_consumed(&conn->input.buffer->ringbuffer, packetsize);

            /* keep command and cookie until we get complete STAT response */
            swallow_command(c, &header, was_connected);
        }

    }

    return 1;
}

int lcb_proto_parse_single(lcb_server_t *c, hrtime_t stop)
{
    if (c->instance->callbacks.packet_fwd) {
        return lcb_parse_single_with_packet_forward(c, stop);
    } else {
        return lcb_parse_single_with_specialized_callbacks(c, stop);
    }
}
