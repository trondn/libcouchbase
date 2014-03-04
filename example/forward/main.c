/* -*- Mode: C; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2014 Couchbase, Inc.
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

#include <stdio.h>
#include <libcouchbase/couchbase.h>
#include <memcached/protocol_binary.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#ifdef _WIN32
#define PRIu64 "I64u"
#else
#include <inttypes.h>
#endif

#define MAXPACKETS 32

struct {
    lcb_buffer_t *buffer;
    lcb_iovec_t iov[2];
} packets[MAXPACKETS];

int num_packets;

static const char *opcode2text(uint8_t opcode) {
    switch (opcode) {
    case PROTOCOL_BINARY_CMD_GET: return "get";
    case PROTOCOL_BINARY_CMD_SET: return "set";
    case PROTOCOL_BINARY_CMD_ADD: return "add";
    case PROTOCOL_BINARY_CMD_REPLACE: return "replace";
    case PROTOCOL_BINARY_CMD_DELETE: return "delete";
    case PROTOCOL_BINARY_CMD_INCREMENT: return "incr";
    case PROTOCOL_BINARY_CMD_DECREMENT: return "decr";
    case PROTOCOL_BINARY_CMD_QUIT: return "quit";
    case PROTOCOL_BINARY_CMD_FLUSH: return "flush";
    case PROTOCOL_BINARY_CMD_GETQ: return "getq";
    case PROTOCOL_BINARY_CMD_NOOP: return "noop";
    case PROTOCOL_BINARY_CMD_VERSION: return "version";
    case PROTOCOL_BINARY_CMD_GETK: return "getk";
    case PROTOCOL_BINARY_CMD_GETKQ: return "getkq";
    case PROTOCOL_BINARY_CMD_APPEND: return "append";
    case PROTOCOL_BINARY_CMD_PREPEND: return "prepend";
    case PROTOCOL_BINARY_CMD_STAT: return "stat";
    case PROTOCOL_BINARY_CMD_SETQ: return "setq";
    case PROTOCOL_BINARY_CMD_ADDQ: return "addq";
    case PROTOCOL_BINARY_CMD_REPLACEQ: return "replaceq";
    case PROTOCOL_BINARY_CMD_DELETEQ: return "deleteq";
    case PROTOCOL_BINARY_CMD_INCREMENTQ: return "incrq";
    case PROTOCOL_BINARY_CMD_DECREMENTQ: return "decrq";
    case PROTOCOL_BINARY_CMD_QUITQ: return "quitq";
    case PROTOCOL_BINARY_CMD_FLUSHQ: return "flushq";
    case PROTOCOL_BINARY_CMD_APPENDQ: return "appendq";
    case PROTOCOL_BINARY_CMD_PREPENDQ: return "prependq";
    case PROTOCOL_BINARY_CMD_VERBOSITY: return "verbosity";
    case PROTOCOL_BINARY_CMD_TOUCH: return "touch";
    case PROTOCOL_BINARY_CMD_GAT: return "gat";
    case PROTOCOL_BINARY_CMD_GATQ: return "gatq";
    case PROTOCOL_BINARY_CMD_HELLO: return "hello";
    case PROTOCOL_BINARY_CMD_SASL_LIST_MECHS: return "sasl_list_mechs";
    case PROTOCOL_BINARY_CMD_SASL_AUTH: return "sasl_auth";
    case PROTOCOL_BINARY_CMD_SASL_STEP: return "sasl_step";
    case PROTOCOL_BINARY_CMD_RGET: return "rget";
    case PROTOCOL_BINARY_CMD_RSET: return "rset";
    case PROTOCOL_BINARY_CMD_RSETQ: return "resetq";
    case PROTOCOL_BINARY_CMD_RAPPEND: return "rappend";
    case PROTOCOL_BINARY_CMD_RAPPENDQ: return "rappendq";
    case PROTOCOL_BINARY_CMD_RPREPEND: return "rpeprend";
    case PROTOCOL_BINARY_CMD_RPREPENDQ: return "rprependq";
    case PROTOCOL_BINARY_CMD_RDELETE: return "rdelete";
    case PROTOCOL_BINARY_CMD_RDELETEQ: return "rdeleteq";
    case PROTOCOL_BINARY_CMD_RINCR: return "rincr";
    case PROTOCOL_BINARY_CMD_RINCRQ: return "rincrq";
    case PROTOCOL_BINARY_CMD_RDECR: return "rdecr";
    case PROTOCOL_BINARY_CMD_RDECRQ: return "rdecrq";
    case PROTOCOL_BINARY_CMD_SET_VBUCKET: return "set_vbucket";
    case PROTOCOL_BINARY_CMD_GET_VBUCKET: return "get_vbucket";
    case PROTOCOL_BINARY_CMD_DEL_VBUCKET: return "del_vbucket";
    case PROTOCOL_BINARY_CMD_TAP_CONNECT: return "tap_connect";
    case PROTOCOL_BINARY_CMD_TAP_MUTATION: return "tap_mutation";
    case PROTOCOL_BINARY_CMD_TAP_DELETE: return "tap_delete";
    case PROTOCOL_BINARY_CMD_TAP_FLUSH: return "tap_flush";
    case PROTOCOL_BINARY_CMD_TAP_OPAQUE: return "tap_opaque";
    case PROTOCOL_BINARY_CMD_TAP_VBUCKET_SET: return "tap_vbucket_set";
    case PROTOCOL_BINARY_CMD_TAP_CHECKPOINT_START: return "tap_checkpoint_start";
    case PROTOCOL_BINARY_CMD_TAP_CHECKPOINT_END: return "tap_checkpoint_end";
    case PROTOCOL_BINARY_CMD_UPR_OPEN: return "upr_open";
    case PROTOCOL_BINARY_CMD_UPR_ADD_STREAM: return "upr_add_stream";
    case PROTOCOL_BINARY_CMD_UPR_CLOSE_STREAM: return "upr_close_stream";
    case PROTOCOL_BINARY_CMD_UPR_STREAM_REQ: return "upr_stream_req";
    case PROTOCOL_BINARY_CMD_UPR_GET_FAILOVER_LOG: return "upr_get_failover_log";
    case PROTOCOL_BINARY_CMD_UPR_STREAM_END: return "upr_stream_end";
    case PROTOCOL_BINARY_CMD_UPR_SNAPSHOT_MARKER: return "upr_snapshot_marker";
    case PROTOCOL_BINARY_CMD_UPR_MUTATION: return "upr_mutation";
    case PROTOCOL_BINARY_CMD_UPR_DELETION: return "upr_deletion";
    case PROTOCOL_BINARY_CMD_UPR_EXPIRATION: return "upr_expiration";
    case PROTOCOL_BINARY_CMD_UPR_FLUSH: return "upr_flush";
    case PROTOCOL_BINARY_CMD_UPR_SET_VBUCKET_STATE: return "upr_set_vbucket_state";
    case PROTOCOL_BINARY_CMD_UPR_RESERVED1: return "upr_reserved1";
    case PROTOCOL_BINARY_CMD_UPR_RESERVED2: return "upr_reserved2";
    case PROTOCOL_BINARY_CMD_UPR_RESERVED3: return "upr_reserved3";
    case PROTOCOL_BINARY_CMD_UPR_RESERVED4: return "upr_reserved4";
    case PROTOCOL_BINARY_CMD_SCRUB: return "scrub";
    case PROTOCOL_BINARY_CMD_ISASL_REFRESH: return "isasl_refresh";
    case PROTOCOL_BINARY_CMD_SSL_CERTS_REFRESH: return "ssl_certs_refresh";
    default:
        return "unknown";
    }
}

static const char *status2text(uint16_t status) {
    switch (status) {
    case PROTOCOL_BINARY_RESPONSE_SUCCESS: return "success";
    case PROTOCOL_BINARY_RESPONSE_KEY_ENOENT: return "enoent";
    case PROTOCOL_BINARY_RESPONSE_KEY_EEXISTS: return "eexists";
    case PROTOCOL_BINARY_RESPONSE_E2BIG: return "e2big";
    case PROTOCOL_BINARY_RESPONSE_EINVAL: return "einval";
    case PROTOCOL_BINARY_RESPONSE_NOT_STORED: return "not stored";
    case PROTOCOL_BINARY_RESPONSE_DELTA_BADVAL: return "delta badval";
    case PROTOCOL_BINARY_RESPONSE_NOT_MY_VBUCKET: return "not my vbucket";
    case PROTOCOL_BINARY_RESPONSE_AUTH_ERROR: return "auth error";
    case PROTOCOL_BINARY_RESPONSE_AUTH_CONTINUE: return "auth continue";
    case PROTOCOL_BINARY_RESPONSE_ERANGE: return "erange";
    case PROTOCOL_BINARY_RESPONSE_ROLLBACK: return "rollback";
    case PROTOCOL_BINARY_RESPONSE_UNKNOWN_COMMAND: return "unknown cmd";
    case PROTOCOL_BINARY_RESPONSE_ENOMEM: return "enomem";
    case PROTOCOL_BINARY_RESPONSE_NOT_SUPPORTED: return "not supported";
    case PROTOCOL_BINARY_RESPONSE_EINTERNAL: return "einternal";
    case PROTOCOL_BINARY_RESPONSE_EBUSY: return "ebusy";
    case PROTOCOL_BINARY_RESPONSE_ETMPFAIL: return "tmpfail";
    default:
        return "unknown";
    }
}

static void decode_response(const lcb_iovec_t iov[2]) {
    /* Currently I don't support wrapping bufs */
    protocol_binary_response_header header;
    uint8_t *packet = (uint8_t*)iov[0].iov_base;

    assert(iov[1].iov_len == 0);
    memcpy(header.bytes, iov[0].iov_base, sizeof(header.bytes));
    fprintf(stdout, "Command: %02x: %s\n", header.response.opcode,
            opcode2text(header.response.opcode));
    if (header.response.keylen != 0) {
        fprintf(stdout, "Key: ");
        fwrite(packet + sizeof(header.bytes) + header.response.extlen,
               1, ntohs(header.response.keylen), stdout);
        fprintf(stdout, "\n");
    }

    fprintf(stdout, "Status: %s\n", status2text(ntohs(header.response.status)));

}

static void dump(const lcb_iovec_t iov[2]) {
    lcb_size_t ii;
    lcb_size_t total = iov[0].iov_len + iov[1].iov_len;
    uint8_t *ptr = (uint8_t*)iov[0].iov_base;

    fprintf(stdout, "***************************************\n");

    switch (*ptr) {
    case PROTOCOL_BINARY_REQ:
        fprintf(stdout, "Package dump of request package %u bytes\n",
                (unsigned int)total);
        break;
    case PROTOCOL_BINARY_RES:
        fprintf(stdout, "Package dump of response package %u bytes\n",
                (unsigned int)total);
        decode_response(iov);
        break;
    default:
        fprintf(stdout, "Package dump of INVALID package %u bytes\n",
                (unsigned int)total);
    }

    for (ii = 0; ii < total; ++ii, ++ptr) {
        if (ii == iov[0].iov_len) {
            ptr = (uint8_t*)iov[1].iov_base;
        }
        if (ii > 0 && (ii % 8 == 0)) {
            fprintf(stdout, "\n");
        }
        fprintf(stdout, "%02x ", *ptr);
    }

    fprintf(stdout, "\n");


    fflush(stdout);
}


static void error_callback(lcb_t instance,
                           lcb_error_t error,
                           const char *errinfo)
{
    fprintf(stderr, "ERROR: %s (0x%x), %s\n",
            lcb_strerror(instance, error), error, errinfo);
    exit(EXIT_FAILURE);
}

static int packet_fwd_callback(lcb_t instance,
                               const void *cookie,
                               lcb_error_t err,
                               const lcb_packet_fwd_resp_t *resp)
{
    (void)instance;
    (void)cookie;
    if (err == LCB_SUCCESS) {
        assert(num_packets < MAXPACKETS);
        packets[num_packets].buffer = resp->v.v0.buffer;
        memcpy(packets[num_packets].iov, resp->v.v0.iov,
               sizeof(packets[num_packets].iov));

        /* We're not using multiple threads so I don't need to lock! */
        packets[num_packets].buffer->management.refcount++;

        ++num_packets;
    }
    return 1;
}

static void insert_get_request(lcb_packet_buf_t *p, const char *key) {
    protocol_binary_request_get req;
    uint16_t nkey = (uint16_t)strlen(key);

    memset(&req, 0, sizeof(req));
    req.message.header.request.magic = PROTOCOL_BINARY_REQ;
    req.message.header.request.opcode = PROTOCOL_BINARY_CMD_GET;
    req.message.header.request.keylen = ntohs((lcb_uint16_t)nkey);
    req.message.header.request.datatype = PROTOCOL_BINARY_RAW_BYTES;
    req.message.header.request.bodylen = ntohl((lcb_uint32_t)nkey);

    p->iov[0].iov_base = p->bufinfo->ringbuffer.write_head;
    p->iov[0].iov_len = sizeof(req.bytes) + nkey;
    p->iov[1].iov_base = NULL;
    p->iov[1].iov_len = 0;

    lcb_ringbuffer_write(&p->bufinfo->ringbuffer,
                         req.bytes, sizeof(req.bytes));
    lcb_ringbuffer_write(&p->bufinfo->ringbuffer, key, nkey);

    dump(p->iov);

}

static void insert_set_request(lcb_packet_buf_t *p,
                               const char *key,
                               const char *value) {
    protocol_binary_request_set req;
    uint16_t nkey = (uint16_t)strlen(key);
    uint32_t nvalue = (uint32_t)strlen(value);
    memset(&req, 0, sizeof(req));

    req.message.header.request.magic = PROTOCOL_BINARY_REQ;
    req.message.header.request.keylen = ntohs((lcb_uint16_t)nkey);
    req.message.header.request.extlen = 8;
    req.message.header.request.datatype = PROTOCOL_BINARY_RAW_BYTES;
    req.message.header.request.cas = 0;
    req.message.header.request.bodylen = ntohl((lcb_uint32_t)nkey +
                                               8 + nvalue);
    req.message.header.request.opcode = PROTOCOL_BINARY_CMD_SET;

    p->iov[0].iov_base = p->bufinfo->ringbuffer.write_head;
    p->iov[0].iov_len = sizeof(req.bytes) + nkey + nvalue;
    p->iov[1].iov_base = NULL;
    p->iov[1].iov_len = 0;

    lcb_ringbuffer_write(&p->bufinfo->ringbuffer,
                         req.bytes, sizeof(req.bytes));
    lcb_ringbuffer_write(&p->bufinfo->ringbuffer, key, nkey);
    lcb_ringbuffer_write(&p->bufinfo->ringbuffer, value, nvalue);

    dump(p->iov);
}

static void send_packets(lcb_t instance) {
    lcb_allocator_t *allocator = lcb_get_default_allocator();
    lcb_buffer_t *buffer = allocator->allocate(allocator, 4096);
    lcb_error_t err;
    lcb_packet_fwd_cmd_t cmd0;
    lcb_packet_fwd_cmd_t cmd1;
    lcb_packet_fwd_cmd_t cmd2;
    lcb_packet_fwd_cmd_t cmd3;
    lcb_packet_fwd_cmd_t *cmds[4];

    memset(&cmd0, 0, sizeof(cmd0));
    memset(&cmd1, 0, sizeof(cmd1));
    memset(&cmd2, 0, sizeof(cmd2));
    memset(&cmd3, 0, sizeof(cmd3));

    cmds[0] = &cmd0;
    cmds[1] = &cmd1;
    cmds[2] = &cmd2;
    cmds[3] = &cmd3;

    cmd0.v.v0.to_master = 1;
    cmd1.v.v0.to_master = 1;
    cmd2.v.v0.to_master = 1;
    cmd3.v.v0.to_master = 1;
    cmd0.v.v0.buffer.bufinfo = buffer;
    cmd1.v.v0.buffer.bufinfo = buffer;
    cmd2.v.v0.buffer.bufinfo = buffer;
    cmd3.v.v0.buffer.bufinfo = buffer;

    insert_get_request(&cmd0.v.v0.buffer, "foo");
    insert_get_request(&cmd1.v.v0.buffer, "my second key");
    insert_set_request(&cmd2.v.v0.buffer, "foo", "bar");
    insert_get_request(&cmd3.v.v0.buffer, "foo");

    err = lcb_forward_packet(instance, NULL, 4,
                             (const lcb_packet_fwd_cmd_t *const *)cmds);

    buffer->management.destructor(buffer);
    assert(err == LCB_SUCCESS);
}

int main(void)
{
    lcb_error_t err;
    lcb_t instance;
    struct lcb_create_st create_options;
    int ii;

    memset(&create_options, 0, sizeof(create_options));
    create_options.v.v0.host = "localhost:9000";

    err = lcb_create(&instance, &create_options);
    if (err != LCB_SUCCESS) {
        fprintf(stderr, "Failed to create libcouchbase instance: %s\n",
                lcb_strerror(NULL, err));
        return 1;
    }
    (void)lcb_set_error_callback(instance, error_callback);

    /* Initiate the connect sequence in libcouchbase */
    if ((err = lcb_connect(instance)) != LCB_SUCCESS) {
        fprintf(stderr, "Failed to initiate connect: %s\n",
                lcb_strerror(NULL, err));
        lcb_destroy(instance);
        return 1;
    }

    (void)lcb_set_packet_fwd_callback(instance, packet_fwd_callback);

    /* Run the event loop and wait until we've connected */
    lcb_wait(instance);
    send_packets(instance);

    /* Wait for all responses! */
    lcb_wait(instance);
    lcb_destroy(instance);

    /* Dump all of the packets we received */
    for (ii = 0; ii < num_packets; ++ii) {
        dump(packets[ii].iov);
        packets[ii].buffer->management.destructor(packets[ii].buffer);
    }

    return 0;
}
