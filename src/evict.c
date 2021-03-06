/* -*- Mode: C; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2013 Couchbase, Inc.
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

#include "internal.h"

struct server_info_st {
    int vb;
    int idx;
};

LIBCOUCHBASE_API
lcb_error_t lcb_evict(lcb_t instance,
                      const void *command_cookie,
                      lcb_size_t num,
                      const lcb_evict_cmd_t *const *items)
{
    lcb_server_t *server = NULL;
    lcb_size_t ii;
    int vb;
    struct server_info_st *servers = NULL;

    /* we need a vbucket config before we can start getting data.. */
    if (instance->vbucket_config == NULL) {
        switch (instance->type) {
        case LCB_TYPE_CLUSTER:
            return lcb_synchandler_return(instance, LCB_EBADHANDLE);
        case LCB_TYPE_BUCKET:
        default:
            return lcb_synchandler_return(instance, LCB_CLIENT_ETMPFAIL);
        }
    }

    servers = malloc(num * sizeof(struct server_info_st));
    if (servers == NULL) {
        return lcb_synchandler_return(instance, LCB_CLIENT_ENOMEM);
    }
    for (ii = 0; ii < num; ++ii) {
        const void *key = items[ii]->v.v0.key;
        lcb_size_t nkey = items[ii]->v.v0.nkey;
        const void *hashkey = items[ii]->v.v0.hashkey;
        lcb_size_t nhashkey = items[ii]->v.v0.nhashkey;

        if (nhashkey == 0) {
            hashkey = key;
            nhashkey = nkey;
        }
        (void)vbucket_map(instance->vbucket_config, hashkey, nhashkey,
                          &servers[ii].vb, &servers[ii].idx);
        if (servers[ii].idx < 0 || servers[ii].idx > (int)instance->nservers) {
            free(servers);
            return lcb_synchandler_return(instance, LCB_NO_MATCHING_SERVER);
        }
    }

    for (ii = 0; ii < num; ++ii) {
        protocol_binary_request_no_extras req;
        const void *key = items[ii]->v.v0.key;
        lcb_size_t nkey = items[ii]->v.v0.nkey;
        server = instance->servers + servers[ii].idx;
        vb = servers[ii].vb;

        memset(&req, 0, sizeof(req));
        req.message.header.request.magic = PROTOCOL_BINARY_REQ;
        req.message.header.request.opcode = (uint8_t)CMD_EVICT_KEY;
        req.message.header.request.extlen = 0;
        req.message.header.request.keylen = ntohs((lcb_uint16_t)nkey);
        req.message.header.request.datatype = PROTOCOL_BINARY_RAW_BYTES;
        req.message.header.request.vbucket = ntohs((lcb_uint16_t)vb);
        req.message.header.request.bodylen = ntohl((lcb_uint32_t)(nkey));
        req.message.header.request.cas = items[ii]->v.v0.cas;
        req.message.header.request.opaque = ++instance->seqno;
        TRACE_EVICT_BEGIN(&req, key, nkey);
        lcb_server_start_packet(server, command_cookie,
                                req.bytes, sizeof(req.bytes));
        lcb_server_write_packet(server, key, nkey);
        lcb_server_end_packet(server);
        lcb_server_send_packets(server);
    }
    free(servers);

    return lcb_synchandler_return(instance, LCB_SUCCESS);
}
