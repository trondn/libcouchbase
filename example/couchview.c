/* -*- Mode: C; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2011 Couchbase, Inc.
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
 * Example program using libcouchbase_view_execute.
 *
 * Fetches any view in two modes depending on -c parameter: chunked (display
 * result as soon as possible), and default (using internal buffer to
 * accumulate response).
 *
 * @author Sergey Avseyev
 */
#include "config.h"
#include <getopt.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <inttypes.h>
#include <assert.h>

#include <libcouchbase/couchbase.h>

static void usage(char cmd, const void *arg, void *cookie);
static void set_flag(char cmd, const void *arg, void *cookie)
{
    (void)cmd;
    (void)arg;
    *((int *)cookie) = 1;
}

static void set_char_ptr(char cmd, const void *arg, void *cookie)
{
    const char **myptr = cookie;
    *myptr = arg;
    (void)cmd;
}

const char *host = "localhost:8091";
const char *username = NULL;
const char *passwd = NULL;
const char *bucket = NULL;
const char *filename = "-";
const char *post_data = NULL;
int chunked = 0;

struct cookie_st {
    struct libcouchbase_io_opt_st *io;
};

static void set_auth_data(char cmd, const void *arg, void *cookie)
{
    (void)cmd;
    (void)cookie;
    username = arg;
    if (isatty(fileno(stdin))) {
        char prompt[80];
        snprintf(prompt, sizeof(prompt), "Please enter password for %s: ", username);
        passwd = getpass(prompt);
        if (passwd == NULL) {
            exit(EXIT_FAILURE);
        }
    } else {
        char buffer[80];
        size_t len;
        if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
            exit(EXIT_FAILURE);
        }
        len = strlen(buffer) - 1;
        while (len > 0 && isspace(buffer[len])) {
            buffer[len] = '\0';
            --len;
        }
        if (len == 0) {
            exit(EXIT_FAILURE);
        }
        passwd = strdup(buffer);
    }
}

typedef void (*OPTION_HANDLER)(char cmd, const void *arg, void *cookie);
static struct {
    const char *name;
    const char *description;
    int argument;
    char letter;
    OPTION_HANDLER handler;
    void *cookie;
} my_options[256] = {
    ['?'] = {
        .name = "help",
        .description = "\t-?\t\tPrint program usage information",
        .argument = 0,
        .letter = '?',
        .handler = usage
    },
    ['u'] = {
        .name = "username",
        .description = "\t-u name\t\tSpecify username",
        .argument = 1,
        .letter = 'u',
        .handler = set_auth_data
    },
    ['h'] = {
        .name = "host",
        .description = "\t-h host\t\tHost to read configuration from",
        .argument = 1,
        .letter = 'h',
        .handler = set_char_ptr,
        .cookie = &host
    },
    ['b'] = {
        .name = "bucket",
        .description = "\t-b bucket\tThe bucket to connect to",
        .argument = 1,
        .letter = 'b',
        .handler = set_char_ptr,
        .cookie = &bucket
    },
    ['o'] = {
        .name = "file",
        .description = "\t-o filename\tSend the output to this file",
        .argument = 1,
        .letter = 'o',
        .handler = set_char_ptr,
        .cookie = &filename
    },
    ['c'] = {
        .name = "chunked",
        .description = "\t-c\t\tUse chunked callback to stream the data",
        .argument = 0,
        .letter = 'c',
        .handler = set_flag,
        .cookie = &chunked
    },
    ['d'] = {
        .name = "data",
        .description = "\t-d\t\tPOST data, e.g. {\"keys\": [\"key1\", \"key2\", ...]}",
        .argument = 1,
        .letter = 'd',
        .handler = set_char_ptr,
        .cookie = &post_data
    },
};

/**
 * Handle all of the command line options the user passed on the command line.
 * Please note that this function will set optind to point to the first option
 *
 * @param argc Argument count
 * @param argv Argument vector
 */
static void handle_options(int argc, char **argv)
{
    struct option opts[256] =  { [0] = { .name = NULL } };
    int ii = 0;
    char shortopts[128] = { 0 };
    int jj = 0;
    int kk = 0;
    int c;
    for (ii = 0; ii < 256; ++ii) {
        if (my_options[ii].name != NULL) {
            opts[jj].name = (char *)my_options[ii].name;
            opts[jj].has_arg = my_options[ii].argument ? required_argument : no_argument;
            opts[jj].val = my_options[ii].letter;

            shortopts[kk++] = (char)opts[jj].val;
            if (my_options[ii].argument) {
                shortopts[kk++] = ':';
            }
        }
    }

    while ((c = getopt_long(argc, argv, shortopts, opts, NULL)) != EOF) {
        if (my_options[c].handler != NULL) {
            my_options[c].handler((char)c, optarg, my_options[c].cookie);
        } else {
            usage((char)c, NULL, NULL);
        }
    }
}

FILE *output;


static void data_callback(libcouchbase_couch_request_t request,
                          libcouchbase_t instance,
                          const void *cookie,
                          libcouchbase_error_t error,
                          libcouchbase_http_status_t status,
                          const char *path, size_t npath,
                          const void *bytes, size_t nbytes)
{
    struct cookie_st *c = (struct cookie_st *)cookie;

    fwrite(bytes, nbytes, sizeof(char), output);
    if (bytes == NULL) { /* end of response */
        c->io->stop_event_loop(c->io);
    }
    (void)request;
    (void)instance;
    (void)path;
    (void)npath;
    (void)error;
    (void)status;
}

static void complete_callback(libcouchbase_couch_request_t request,
                              libcouchbase_t instance,
                              const void *cookie,
                              libcouchbase_error_t error,
                              libcouchbase_http_status_t status,
                              const char *path, size_t npath,
                              const void *bytes, size_t nbytes)
{
    struct cookie_st *c = (struct cookie_st *)cookie;

    fprintf(stderr, "\"");
    fwrite(path, npath, sizeof(char), stderr);
    fprintf(stderr, "\": ");
    if (error == LIBCOUCHBASE_SUCCESS) {
        fprintf(stderr, "OK\n");
        fwrite(bytes, nbytes, sizeof(char), output);
    } else {
        fprintf(stderr, "FAIL(%d): %s, HTTP code: %d\n",
                error, libcouchbase_strerror(instance, error), status);
        fwrite(bytes, nbytes, sizeof(char), output);
    }
    c->io->stop_event_loop(c->io);
    (void)request;
}

static void error_callback(libcouchbase_t instance,
                           libcouchbase_error_t error,
                           const char *errinfo)
{
    (void)instance;
    fprintf(stderr, "Error %d", error);
    if (errinfo) {
        fprintf(stderr, ": %s", errinfo);
    }
    fprintf(stderr, "\n");
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
    char *uri;
    const char *bytes;
    size_t nbytes = 0;
    struct cookie_st cookie;
    libcouchbase_error_t rc;
    libcouchbase_t instance;

    handle_options(argc, argv);

    if (strcmp(filename, "-") == 0) {
        output = stdout;
    } else {
        output = fopen(filename, "w");
        if (output == NULL) {
            fprintf(stderr, "Failed to open %s: %s\n", filename,
                    strerror(errno));
            return 1;
        }
    }

    uri = argv[optind];
    if (!uri) {
        usage(0, NULL, NULL);
    }

    cookie.io = libcouchbase_create_io_ops(LIBCOUCHBASE_IO_OPS_DEFAULT, NULL, NULL);
    if (cookie.io == NULL) {
        fprintf(stderr, "Failed to create IO instance\n");
        return 1;
    }
    instance = libcouchbase_create(host, username,
                                   passwd, bucket, cookie.io);
    if (instance == NULL) {
        fprintf(stderr, "Failed to create libcouchbase instance\n");
        return 1;
    }

    (void)libcouchbase_set_error_callback(instance, error_callback);
    (void)libcouchbase_set_couch_data_callback(instance, data_callback);
    (void)libcouchbase_set_couch_complete_callback(instance, complete_callback);

    if (libcouchbase_connect(instance) != LIBCOUCHBASE_SUCCESS) {
        fprintf(stderr, "Failed to connect libcouchbase instance to server\n");
        return 1;
    }

    /* Wait for the connect to compelete */
    libcouchbase_wait(instance);

    bytes = post_data;
    if (bytes) {
        nbytes = strlen(bytes);
    }

    rc = libcouchbase_make_couch_request(instance, (void *)&cookie,
                                         uri, strlen(uri), bytes, nbytes,
                                         bytes ? LIBCOUCHBASE_HTTP_METHOD_POST : LIBCOUCHBASE_HTTP_METHOD_GET,
                                         chunked);

    if (rc != LIBCOUCHBASE_SUCCESS) {
        fprintf(stderr, "Failed to execute view: %s\n", libcouchbase_strerror(instance, rc));
        return 1;
    }

    /* Start the event loop and let it run until request will be completed
     * with success or failure (see view callbacks)  */
    cookie.io->run_event_loop(cookie.io);

    return 0;
}

static void usage(char cmd, const void *arg, void *cookie)
{
    int ii;

    (void)cmd;
    (void)arg;
    (void)cookie;

    fprintf(stderr, "Usage: ./couchview [options] viewid\n");
    for (ii = 0; ii < 256; ++ii) {
        if (my_options[ii].name != NULL) {
            fprintf(stderr, "%s\n", my_options[ii].description);
        }
    }
    exit(EXIT_FAILURE);
}
