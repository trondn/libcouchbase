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
#include "internal.h"

/**
 * Send a preformatted packet to the cluster
 * This prototype copies the buffers into the internal structures
 * instead of just keeping a reference...
 */
LIBCOUCHBASE_API
lcb_error_t lcb_forward_packet(lcb_t instance,
                               const void *command_cookie,
                               lcb_size_t num,
                               const lcb_packet_fwd_cmd_t *const *commands)
{
    lcb_size_t ii;

    lcb_error_handler(instance, LCB_SUCCESS, NULL);

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

    /* Now handle all of the requests */
    for (ii = 0; ii < num; ++ii) {
        lcb_server_t *server;
        protocol_binary_request_no_extras *req;
        int idx;
        int vb = 0;

        /* @todo ensure that we support the current command */

        /* @todo currently the entire header needs to be in the first
         *       chunk */
        assert(commands[ii]->v.v0.buffer.iov[0].iov_len >= 24);

        /* I need to update the sequence number in the packet! */
        req = (void *)commands[ii]->v.v0.buffer.iov[0].iov_base;
        vb = ntohs(req->message.header.request.vbucket);

        if (commands[ii]->v.v0.to_master) {
            idx = vbucket_get_master(instance->vbucket_config, vb);
        } else {
            idx = vbucket_get_replica(instance->vbucket_config, vb,
                                      commands[ii]->v.v0.replica_index);
        }

        if (idx < 0 || idx > (int)instance->nservers) {
            return lcb_synchandler_return(instance, LCB_NO_MATCHING_SERVER);
        }

        server = instance->servers + idx;

        ++instance->seqno;
        /* avoid alignment crash by using memcpy */
        memcpy(&req->message.header.request.opaque, &instance->seqno, 4);

        lcb_server_start_packet(server, command_cookie,
                                commands[ii]->v.v0.buffer.iov[0].iov_base,
                                commands[ii]->v.v0.buffer.iov[0].iov_len);
        lcb_server_write_packet(server,
                                commands[ii]->v.v0.buffer.iov[1].iov_base,
                                commands[ii]->v.v0.buffer.iov[1].iov_len);
        lcb_server_end_packet(server);
        lcb_server_send_packets(server);
    }

    return lcb_synchandler_return(instance, LCB_SUCCESS);
}
