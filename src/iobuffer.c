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

static lcb_buffer_t *lcb_allocate_buffer(lcb_allocator_t *, size_t);

lcb_allocator_t default_allocator = {
    lcb_allocate_buffer
};

static void buffer_destructor(lcb_buffer_t *buffer)
{
    int use;
    lcb_mutex_enter(&buffer->management.mutex);
    use = --buffer->management.refcount;
    lcb_mutex_exit(&buffer->management.mutex);
    if (use == 0) {
        lcb_mutex_destroy(&buffer->management.mutex);
        lcb_ringbuffer_destruct(&buffer->ringbuffer);
        free(buffer);
    }
}

static lcb_buffer_t *lcb_allocate_buffer(lcb_allocator_t *allocator,
                                         size_t size)
{
    lcb_buffer_t *ret = calloc(1, sizeof(*ret));
    if (ret == NULL) {
        return NULL;
    }

    if (lcb_ringbuffer_initialize(&ret->ringbuffer, size) == 0) {
        free(ret);
        return NULL;
    }

    lcb_mutex_initialize(&ret->management.mutex);

    ret->management.destructor = buffer_destructor;
    ret->management.allocator = allocator;
    ret->management.refcount = 1;

    return ret;
}


LIBCOUCHBASE_API
lcb_allocator_t *lcb_get_default_allocator(void) {
    return &default_allocator;
}
