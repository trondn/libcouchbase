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

#ifndef LIBCOUCHBASE_LOCKS_H
#define LIBCOUCHBASE_LOCKS_H 1

#include <libcouchbase/visibility.h>
#include <libcouchbase/types.h>
#ifdef __cplusplus
extern "C" {
#endif

    LIBCOUCHBASE_API
    void lcb_mutex_initialize(lcb_mutex_t *mutex);

    LIBCOUCHBASE_API
    void lcb_mutex_destroy(lcb_mutex_t *mutex);

    LIBCOUCHBASE_API
    void lcb_mutex_enter(lcb_mutex_t *mutex);

    LIBCOUCHBASE_API
    void lcb_mutex_exit(lcb_mutex_t *mutex);


#ifdef __cplusplus
}
#endif

#endif
