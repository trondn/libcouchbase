/* -*- Mode: C; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2010 Couchbase, Inc.
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
 * @todo Add documentation about the library (how it works etc)
 *
 * @author Trond Norbye
 */
#ifndef LIBCOUCHBASE_COUCHBASE_H
#define LIBCOUCHBASE_COUCHBASE_H 1

#include <stdint.h>
#include <stddef.h>

#ifndef __cplusplus
#include <stdbool.h>
#endif

#include <memcached/vbucket.h>
#include <libcouchbase/visibility.h>
#include <libcouchbase/types.h>
#include <libcouchbase/callbacks.h>

struct event_base;

#ifdef __cplusplus
extern "C" {
#endif

    /**
     * Create an instance of libcouchbase
     * @param host The host (with optional port) to connect to retrieve the
     *             vbucket list from
     * @param user the username to use
     * @param passwd The password
     * @param bucket The bucket to connect to
     * @param base the libevent base we're for this instance
     * @return A handle to libcouchbase, or NULL if an error occured.
     */
    LIBCOUCHBASE_API
    libcouchbase_t libcouchbase_create(const char *host,
                                       const char *user,
                                       const char *passwd,
                                       const char *bucket,
                                       struct event_base *base);


    /**
     * Destroy (and release all allocated resources) an instance of libcouchbase.
     * Using instance after calling destroy will most likely cause your
     * application to crash.
     *
     * @param instance the instance to destroy.
     */
    LIBCOUCHBASE_API
    void libcouchbase_destroy(libcouchbase_t instance);

    /**
     * Create an instance of a libcouchbase tap filter (libcouchbase_tap_filter_t).
     */
    LIBCOUCHBASE_API
    libcouchbase_tap_filter_t libcouchbase_tap_filter_create(void);

    /**
     * Destroy (and release all allocated resources) an instance of a libcouchbase
     * tap filter (libcouchbase_tap_filter_t). Using the instance after calling
     * destroy will most likely cause your application to crash.
     * @param instance the instance to destroy.
     */
    LIBCOUCHBASE_API
    void libcouchbase_tap_filter_destroy(libcouchbase_tap_filter_t instance);

    /**
     * Set the backfill value for your tap stream.
     * @param instance the tap filter instance to modify.
     * @param backfill the oldest entry (from epoch) you're interested in.
     */
    LIBCOUCHBASE_API
    void libcouchbase_tap_filter_set_backfill(libcouchbase_tap_filter_t instance,
                                              uint64_t backfill);

    /**
     * Get the backfill value for your tap stream.
     * @param instance the tap filter instance to retrieve the value from.
     */
    LIBCOUCHBASE_API
    uint64_t libcouchbase_tap_filter_get_backfill(libcouchbase_tap_filter_t instance);

    /**
     * Set whether you are interested in keys and values, or only keys in your
     * tap stream.
     * @param instance the tap filter instance to modify.
     * @param keys_only true if you are only interested in keys, false if
     *                  you also want values.
     */
    LIBCOUCHBASE_API
    void libcouchbase_tap_filter_set_keys_only(libcouchbase_tap_filter_t instance,
                                               bool keys_only);

    /**
     * Get whether you are interested in keys and values, or only keys in your
     * tap stream.
     * @param instance the tap filter instance to retrieve the value from.
     */
    LIBCOUCHBASE_API
    bool libcouchbase_tap_filter_get_keys_only(libcouchbase_tap_filter_t instance);

    /**
     * Connect to the server and get the vbucket and serverlist.
     */
    LIBCOUCHBASE_API
    libcouchbase_error_t libcouchbase_connect(libcouchbase_t instance);

    /**
     * Associate a cookie with an instance of libcouchbase
     * @param instance the instance to associate the cookie to
     * @param cookie the cookie to associate with this instance.
     */
    LIBCOUCHBASE_API
    void libcouchbase_set_cookie(libcouchbase_t instance, const void *cookie);


    /**
     * Retrieve the cookie associated with this instance
     * @param instance the instance of libcouchbase
     * @return The cookie associated with this instance or NULL
     */
    LIBCOUCHBASE_API
    const void *libcouchbase_get_cookie(libcouchbase_t instance);

    /**
     * Set the packet filter for this instance
     * @param instance the instance of libcouchbase
     * @param filter the new packet filter to associate with this instance
     */
    LIBCOUCHBASE_API
    void libcouchbase_set_packet_filter(libcouchbase_t instance,
                                        libcouchbase_packet_filter_t filter);

    /**
     * Set the command handlers
     * @param instance the instance of libcouchbase
     * @param callback the new set of callbacks
     */
    LIBCOUCHBASE_API
    void libcouchbase_set_callbacks(libcouchbase_t instance,
                                    libcouchbase_callback_t *callbacks);

    /**
     * Use the TAP protocol to tap the cluster
     * @param instance the instance to tap
     * @param filter the tap filter to use
     * @param callbacks the calback to use
     * @param block set to true if you want libcouchbase to run the event
     *              dispatcher loop
     */
    LIBCOUCHBASE_API
    void libcouchbase_tap_cluster(libcouchbase_t instance,
                                  libcouchbase_tap_filter_t filter,
                                  bool block);

    /**
     * Execute all of the batched requests
     * @param instance the instance containing the requests
     */
    LIBCOUCHBASE_API
    void libcouchbase_execute(libcouchbase_t instance);

    /**
     * Get a number of values from the cache. You need to run the
     * event loop yourself (or call libcouchbase_execute) to retrieve
     * the data. You might want to alter the expiry time for the object
     * you're fetcing, and to do so you should specify the new expirty
     * time in the exp parameter. To use an ordinary mget use NULL
     * for the exp parameter.
     *
     * @param instance the instance used to batch the requests from
     * @param num_keys the number of keys to get
     * @param keys the array containing the keys to get
     * @param nkey the array containing the lengths of the keys
     * @param exp the new expiration time for the object
     * @return The status of the operation
     */
    LIBCOUCHBASE_API
    libcouchbase_error_t libcouchbase_mget(libcouchbase_t instance,
                                           size_t num_keys,
                                           const void * const *keys,
                                           const size_t *nkey,
                                           const time_t *exp);

    /**
     * Get a number of values from the cache. You need to run the
     * event loop yourself (or call libcouchbase_execute) to retrieve
     * the data. You might want to alter the expiry time for the object
     * you're fetcing, and to do so you should specify the new expirty
     * time in the exp parameter. To use an ordinary mget use NULL
     * for the exp parameter.
     *
     * @param instance the instance used to batch the requests from
     * @param hashkey the key to use for hashing
     * @param nhashkey the number of bytes in hashkey
     * @param num_keys the number of keys to get
     * @param keys the array containing the keys to get
     * @param nkey the array containing the lengths of the keys
     * @return The status of the operation
     */
    LIBCOUCHBASE_API
    libcouchbase_error_t libcouchbase_mget_by_key(libcouchbase_t instance,
                                                  const void *hashkey,
                                                  size_t nhashkey,
                                                  size_t num_keys,
                                                  const void * const *keys,
                                                  const size_t *nkey,
                                                  const time_t *exp);


    /**
     * Touch (set expiration time) on a number of values in the cache
     * You need to run the event loop yourself (or call
     * libcouchbase_execute) to retrieve the results of the operations.
     *
     * @param instance the instance used to batch the requests from
     * @param num_keys the number of keys to get
     * @param keys the array containing the keys to get
     * @param nkey the array containing the lengths of the keys
     * @return The status of the operation
     */
    LIBCOUCHBASE_API
    libcouchbase_error_t libcouchbase_mtouch(libcouchbase_t instance,
                                             size_t num_keys,
                                             const void * const *keys,
                                             const size_t *nkey,
                                             const time_t *exp);

    /**
     * Touch (set expiration time) on a number of values in the cache
     * You need to run the event loop yourself (or call
     * libcouchbase_execute) to retrieve the results of the operations.
     *
     * Set <code>nhashkey</code> to 0 if you want to hash each individual
     * key.
     *
     * @param instance the instance used to batch the requests from
     * @param hashkey the key to use for hashing
     * @param nhashkey the number of bytes in hashkey
     * @param num_keys the number of keys to get
     * @param keys the array containing the keys to get
     * @param nkey the array containing the lengths of the keys
     * @param exp the new expiration time for the items
     * @return The status of the operation
     */
    LIBCOUCHBASE_API
    libcouchbase_error_t libcouchbase_mtouch_by_key(libcouchbase_t instance,
                                                    const void *hashkey,
                                                    size_t nhashkey,
                                                    size_t num_keys,
                                                    const void * const *keys,
                                                    const size_t *nkey,
                                                    const time_t *exp);

    /**
     * Spool a store operation to the cluster. The operation <b>may</b> be
     * sent immediately, but you won't be sure (or get the result) until you
     * run the event loop (or call libcouchbase_execute).
     *
     * @param instance the handle to libcouchbase
     * @param operation constraints for the storage operation (add/replace etc)
     * @param key the key to set
     * @param nkey the number of bytes in the key
     * @param bytes the value to set
     * @param nbytes the size of the value
     * @param flags the user-defined flag section for the item
     * @param exp When the object should expire
     * @param cas the cas identifier for the existing object if you want to
     *            ensure that you're only replacing/append/prepending a
     *            specific object. Specify 0 if you don't want to limit to
     *            any cas value.
     * @return Status of the operation.
     */
    LIBCOUCHBASE_API
    libcouchbase_error_t libcouchbase_store(libcouchbase_t instance,
                                            libcouchbase_storage_t operation,
                                            const void *key, size_t nkey,
                                            const void *bytes, size_t nbytes,
                                            uint32_t flags, time_t exp,
                                            uint64_t cas);

    /**
     * Spool a store operation to the cluster. The operation <b>may</b> be
     * sent immediately, but you won't be sure (or get the result) until you
     * run the event loop (or call libcouchbase_execute).
     *
     * @param instance the handle to libcouchbase
     * @param operation constraints for the storage operation (add/replace etc)
     * @param hashkey the key to use for hashing
     * @param nhashkey the number of bytes in hashkey
     * @param key the key to set
     * @param nkey the number of bytes in the key
     * @param bytes the value to set
     * @param nbytes the size of the value
     * @param flags the user-defined flag section for the item
     * @param exp When the object should expire
     * @param cas the cas identifier for the existing object if you want to
     *            ensure that you're only replacing/append/prepending a
     *            specific object. Specify 0 if you don't want to limit to
     *            any cas value.
     * @return Status of the operation.
     */
    LIBCOUCHBASE_API
    libcouchbase_error_t libcouchbase_store_by_key(libcouchbase_t instance,
                                                   libcouchbase_storage_t operation,
                                                   const void *hashkey,
                                                   size_t nhashkey,
                                                   const void *key,
                                                   size_t nkey,
                                                   const void *bytes,
                                                   size_t nbytes,
                                                   uint32_t flags,
                                                   time_t exp,
                                                   uint64_t cas);

    /**
     * Spool an arithmetic operation to the cluster. The operation <b>may</b> be
     * sent immediately, but you won't be sure (or get the result) until you
     * run the event loop (or call libcouchbase_execute).
     *
     * @param instance the handle to libcouchbase
     * @param key the key to set
     * @param nkey the number of bytes in the key
     * @param delta The amount to add / subtract
     * @param exp When the object should expire
     * @param create set to true if you want the object to be created if it
     *               doesn't exist.
     * @param initial The initial value of the object if we create it
     * @return Status of the operation.
     */
    LIBCOUCHBASE_API
    libcouchbase_error_t libcouchbase_arithmetic(libcouchbase_t instance,
                                                 const void *key, size_t nkey,
                                                 int64_t delta, time_t exp,
                                                 bool create, uint64_t initial);

    /**
     * Spool an arithmetic operation to the cluster. The operation <b>may</b> be
     * sent immediately, but you won't be sure (or get the result) until you
     * run the event loop (or call libcouchbase_execute).
     *
     * @param instance the handle to libcouchbase
     * @param hashkey the key to use for hashing
     * @param nhashkey the number of bytes in hashkey
     * @param key the key to set
     * @param nkey the number of bytes in the key
     * @param delta The amount to add / subtract
     * @param exp When the object should expire
     * @param create set to true if you want the object to be created if it
     *               doesn't exist.
     * @param initial The initial value of the object if we create it
     * @return Status of the operation.
     */
    LIBCOUCHBASE_API
    libcouchbase_error_t libcouchbase_arithmetic_by_key(libcouchbase_t instance,
                                                        const void *hashkey,
                                                        size_t nhashkey,
                                                        const void *key,
                                                        size_t nkey,
                                                        int64_t delta,
                                                        time_t exp,
                                                        bool create,
                                                        uint64_t initial);

    /**
     * Spool a remove operation to the cluster. The operation <b>may</b> be
     * sent immediately, but you won't be sure (or get the result) until you
     * run the event loop (or call libcouchbase_execute).
     *
     * @param instance the handle to libcouchbase
     * @param key the key to delete
     * @param nkey the number of bytes in the key
     * @param cas the cas value for the object (or 0 if you don't care)
     * @return Status of the operation.
     */
    LIBCOUCHBASE_API
    libcouchbase_error_t libcouchbase_remove(libcouchbase_t instance,
                                             const void *key, size_t nkey,
                                             uint64_t cas);

    /**
     * Spool a remove operation to the cluster. The operation <b>may</b> be
     * sent immediately, but you won't be sure (or get the result) until you
     * run the event loop (or call libcouchbase_execute).
     *
     * @param instance the handle to libcouchbase
     * @param hashkey the key to use for hashing
     * @param nhashkey the number of bytes in hashkey
     * @param key the key to delete
     * @param nkey the number of bytes in the key
     * @param cas the cas value for the object (or 0 if you don't care)
     * @return Status of the operation.
     */
    LIBCOUCHBASE_API
    libcouchbase_error_t libcouchbase_remove_by_key(libcouchbase_t instance,
                                                    const void *hashkey,
                                                    size_t nhashkey,
                                                    const void *key,
                                                    size_t nkey,
                                                    uint64_t cas);

#ifdef __cplusplus
}
#endif

#endif
