/* -*- Mode: C; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2011-2012 Couchbase, Inc.
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
#ifndef LCB_RINGBUFFER_H
#define LCB_RINGBUFFER_H 1

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct lcb_ringbuffer_st {
        char *root;
        char *read_head;
        char *write_head;
        lcb_size_t size;
        lcb_size_t nbytes;
    } lcb_ringbuffer_t;

    typedef enum {
        LCB_RINGBUFFER_READ = 0x01,
        LCB_RINGBUFFER_WRITE = 0x02
    } lcb_ringbuffer_direction_t;

    LIBCOUCHBASE_API
    int lcb_ringbuffer_initialize(lcb_ringbuffer_t *buffer,
                                  lcb_size_t size);

    /**
     * Initialize a lcb_ringbuffer, taking ownership of an allocated
     * char buffer.  This function always succeeds.
     *
     * @param buffer a lcb_ringbuffer_t to be initialized
     * @param buf the buffer to steal
     * @param size the allocated size of the buffer
     */
    LIBCOUCHBASE_API
    void lcb_ringbuffer_take_buffer(lcb_ringbuffer_t *buffer,
                                    char *buf,
                                    lcb_size_t size);

    LIBCOUCHBASE_API
    void lcb_ringbuffer_reset(lcb_ringbuffer_t *buffer);

    LIBCOUCHBASE_API
    void lcb_ringbuffer_destruct(lcb_ringbuffer_t *buffer);

    LIBCOUCHBASE_API
    int lcb_ringbuffer_ensure_capacity(lcb_ringbuffer_t *buffer,
                                       lcb_size_t size);
    LIBCOUCHBASE_API
    lcb_size_t lcb_ringbuffer_get_size(lcb_ringbuffer_t *buffer);
    LIBCOUCHBASE_API
    void *lcb_ringbuffer_get_start(lcb_ringbuffer_t *buffer);
    LIBCOUCHBASE_API
    void *lcb_ringbuffer_get_read_head(lcb_ringbuffer_t *buffer);
    LIBCOUCHBASE_API
    void *lcb_ringbuffer_get_write_head(lcb_ringbuffer_t *buffer);
    LIBCOUCHBASE_API
    lcb_size_t lcb_ringbuffer_write(lcb_ringbuffer_t *buffer,
                                    const void *src,
                                    lcb_size_t nb);
    LIBCOUCHBASE_API
    lcb_size_t lcb_ringbuffer_strcat(lcb_ringbuffer_t *buffer,
                                     const char *str);
    LIBCOUCHBASE_API
    lcb_size_t lcb_ringbuffer_read(lcb_ringbuffer_t *buffer,
                                   void *dest,
                                   lcb_size_t nb);
    LIBCOUCHBASE_API
    lcb_size_t lcb_ringbuffer_peek(lcb_ringbuffer_t *buffer,
                                   void *dest,
                                   lcb_size_t nb);
    LIBCOUCHBASE_API
    lcb_size_t lcb_ringbuffer_peek_at(lcb_ringbuffer_t *buffer,
                                      lcb_size_t offset,
                                      void *dest,
                                      lcb_size_t nb);
    /* replace +nb+ bytes on +direction+ end of the buffer with src */
    LIBCOUCHBASE_API
    lcb_size_t lcb_ringbuffer_update(lcb_ringbuffer_t *buffer,
                                     lcb_ringbuffer_direction_t direction,
                                     const void *src, lcb_size_t nb);
    LIBCOUCHBASE_API
    void lcb_ringbuffer_get_iov(lcb_ringbuffer_t *buffer,
                                lcb_ringbuffer_direction_t direction,
                                struct lcb_iovec_st *iov);
    LIBCOUCHBASE_API
    void lcb_ringbuffer_produced(lcb_ringbuffer_t *buffer, lcb_size_t nb);
    LIBCOUCHBASE_API
    void lcb_ringbuffer_consumed(lcb_ringbuffer_t *buffer, lcb_size_t nb);
    LIBCOUCHBASE_API
    lcb_size_t lcb_ringbuffer_get_nbytes(lcb_ringbuffer_t *buffer);
    LIBCOUCHBASE_API
    int lcb_ringbuffer_is_continous(lcb_ringbuffer_t *buffer,
                                    lcb_ringbuffer_direction_t direction,
                                    lcb_size_t nb);

    LIBCOUCHBASE_API
    int lcb_ringbuffer_append(lcb_ringbuffer_t *src, lcb_ringbuffer_t *dest);
    LIBCOUCHBASE_API
    int lcb_ringbuffer_memcpy(lcb_ringbuffer_t *dst, lcb_ringbuffer_t *src,
                              lcb_size_t nbytes);

    /* Align the read head of the lcb_ringbuffer for platforms where it's needed */
    LIBCOUCHBASE_API
    int lcb_ringbuffer_ensure_alignment(lcb_ringbuffer_t *src);

#ifdef __cplusplus
}
#endif

#endif
