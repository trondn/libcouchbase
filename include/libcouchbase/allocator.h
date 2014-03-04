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
#ifndef LIBCOUCHBASE_ALLOCATOR_H
#define LIBCOUCHBASE_ALLOCATOR_H 1

#ifdef __cplusplus
extern "C" {
#endif

    /* Forward decl */
    typedef struct lcb_allocator_st lcb_allocator_t;
    struct lcb_allocator_st {
        lcb_buffer_t *(*allocate)(lcb_allocator_t *allocator, size_t size);
    };



    LIBCOUCHBASE_API
    lcb_allocator_t *lcb_get_default_allocator(void);





#ifdef __cplusplus
}
#endif

#endif
