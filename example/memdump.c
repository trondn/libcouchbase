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
 * Example program using libcouchbase_tap_cluster.
 *
 * @author Trond Norbye
 * @todo add documentation
 */
#include <unistd.h>
#include <getopt.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <event.h>

#include <libcouchbase/couchbase.h>

#include <libdrizzle/drizzle_client.h>

static void usage(char cmd, const void *arg, void *cookie);
static void set_char_ptr(char cmd, const void *arg, void *cookie) {
    (void)cmd;
    const char **myptr = cookie;
    *myptr = arg;
}

const char *host = "localhost:8091";
const char *username = NULL;
const char *passwd = NULL;
const char *bucket = NULL;

static drizzle_st *drizzle;
static drizzle_con_st *drizzle_con;

static const char *drizzle_host = "localhost";
static const in_port_t drizzle_port = 4427;
static const char *drizzle_user = NULL;
static const char *drizzle_passwd = NULL;
static const char *drizzle_dbname = NULL;

static void set_auth_data(char cmd, const void *arg, void *cookie) {
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
        if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
            exit(EXIT_FAILURE);
        }
        size_t len = strlen(buffer) - 1;
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
    bool argument;
    char letter;
    OPTION_HANDLER handler;
    void *cookie;
} my_options[256] = {
    ['?'] = {
        .name = "help",
        .description = "\t-?\tPrint program usage information",
        .argument = false,
        .letter = '?',
        .handler = usage
    },
    ['u'] = {
        .name = "username",
        .description = "\t-u nm\tSpecify username",
        .argument = true,
        .letter = 'u',
        .handler = set_auth_data
    },
    ['h'] = {
        .name = "host",
        .description = "\t-h host\tHost to read configuration from",
        .argument = true,
        .letter = 'h',
        .handler = set_char_ptr,
        .cookie = &host
    },
    ['H'] = {
        .name = "drizzle-host",
        .description = "\t-H host\tWhere drizzle is running",
        .argument = true,
        .letter = 'H',
        .handler = set_char_ptr,
        .cookie = &drizzle_host
    },
    ['U'] = {
        .name = "drizzle-user",
        .description = "\t-U nm\tSpecify drizzle username",
        .argument = true,
        .letter = 'U',
        .handler = set_char_ptr,
        .cookie = &drizzle_user
    },
    ['P'] = {
        .name = "drizzle-passwd",
        .description = "\t-P nm\tSpecify drizzle password",
        .argument = true,
        .letter = 'P',
        .handler = set_char_ptr,
        .cookie = &drizzle_passwd
    },
    ['S'] = {
        .name = "drizzle-dbnm",
        .description = "\t-S nm\tSpecify drizzle database name",
        .argument = true,
        .letter = 'S',
        .handler = set_char_ptr,
        .cookie = &drizzle_dbname
    },
    ['b'] = {
        .name = "bucket",
        .description = "\t-b bucket\tThe bucket to connect to",
        .argument = true,
        .letter = 'b',
        .handler = set_char_ptr,
        .cookie = &bucket
    },
};

/**
 * Handle all of the command line options the user passed on the command line.
 * Please note that this function will set optind to point to the first option
 *
 * @param argc Argument count
 * @param argv Argument vector
 */
static void handle_options(int argc, char **argv) {
    struct option opts[256] =  { [0] = { .name = NULL } };
    int ii = 0;
    char shortopts[128] = { 0 };
    int jj = 0;
    int kk = 0;
    for (ii = 0; ii < 256; ++ii) {
        if (my_options[ii].name != NULL) {
            opts[jj].name = (char*)my_options[ii].name;
            opts[jj].has_arg = my_options[ii].argument ? required_argument : no_argument;
            opts[jj].val = my_options[ii].letter;

            shortopts[kk++] = (char)opts[jj].val;
            if (my_options[ii].argument) {
                shortopts[kk++] = ':';
            }
        }
    }

    int c;
    while ((c = getopt_long(argc, argv, shortopts, opts, NULL)) != EOF) {
        if (my_options[c].handler != NULL) {
            my_options[c].handler((char)c, optarg, my_options[c].cookie);
        } else {
            usage((char)c, NULL, NULL);
        }
    }
}

static void drizzle_init(void) {
    if ((drizzle = drizzle_create(NULL)) == NULL) {
        fprintf(stderr, "Failed to create drizzle instance\n");
        exit(EXIT_FAILURE);
    }

    if ((drizzle_con = drizzle_con_create(drizzle, NULL)) == NULL) {
        fprintf(stderr, "Failed to create drizzle connection instance\n");
        exit(EXIT_FAILURE);
    }

    drizzle_con_set_tcp(drizzle_con, drizzle_host, drizzle_port);
    drizzle_con_set_auth(drizzle_con, drizzle_user, drizzle_passwd);
    drizzle_con_set_db(drizzle_con, drizzle_dbname);

    // @todo I should probably run connect to verify that the stuff works?
}

static char *sql_buffer[8192 * 1024];

static void tap_mutation(libcouchbase_t instance,
                         const void *key,
                         size_t nkey,
                         const void *data,
                         size_t nbytes,
                         uint32_t flags,
                         uint32_t exp,
                         const void *es,
                         size_t nes)
{
    (void)instance;(void)flags;(void)exp;(void)es;(void)nes;
    // @todo decode jSON
    size_t len = (size_t)snprintf(sql_buffer, sizeof(sql_buffer),
                                  "REPLACE INTO dumptable(k, v) VALUES('");
    len += drizzle_escape_string(sql_buffer + len, key, nkey);
    len += (size_t)snprintf(sql_buffer + len, sizeof(sql_buffer), "','");
    len += drizzle_escape_string(sql_buffer + len, data, nbytes);
    len += (size_t)snprintf(sql_buffer + len, sizeof(sql_buffer), "');");

    drizzle_result_st *result = drizzle_result_create(drizzle_con, NULL);
    if (result == NULL) {
        fprintf(stderr, "Failed to create result structure\n");
        exit(EXIT_FAILURE);
    }

    drizzle_return_t ret;
    drizzle_query_str(drizzle_con, result, sql_buffer, &ret);

    if (ret != DRIZZLE_RETURN_OK) {
        fprintf(stderr, "Failed to add entry to the database: %s\n",
                drizzle_con_error(drizzle_con));
    }

    drizzle_result_free(result);
}

static void tap_deletion(libcouchbase_t instance,
                         const void *key,
                         size_t nkey,
                         const void *es,
                         size_t nes)
{

    (void)instance; (void)es; (void)nes; (void)key; (void)nkey;
    int len = snprintf(sql_buffer, sizeof(sql_buffer),
                       "delete from dumptable where k='");
    len += (int)drizzle_escape_string(sql_buffer + len, key, nkey);
    strcpy(sql_buffer + len, "';");

    drizzle_result_st *result = drizzle_result_create(drizzle_con, NULL);
    if (result == NULL) {
        fprintf(stderr, "Failed to create result structure\n");
        exit(EXIT_FAILURE);
    }

    drizzle_return_t ret;
    drizzle_query_str(drizzle_con, result, sql_buffer, &ret);

    if (ret != DRIZZLE_RETURN_OK) {
        fprintf(stderr, "Failed to nuke table: %s\n",
                drizzle_con_error(drizzle_con));
    }

    drizzle_result_free(result);
}

static void tap_flush(libcouchbase_t instance,
                      const void *es,
                      size_t nes)
{
    (void)instance; (void)es; (void)nes;

    drizzle_result_st *result = drizzle_result_create(drizzle_con, NULL);
    if (result == NULL) {
        fprintf(stderr, "Failed to create result structure\n");
        exit(EXIT_FAILURE);
    }

    drizzle_return_t ret;
    drizzle_query_str(drizzle_con, result, "delete from dumptable;", &ret);

    if (ret != DRIZZLE_RETURN_OK) {
        fprintf(stderr, "Failed to nuke table: %s\n",
                drizzle_con_error(drizzle_con));
    }

    drizzle_result_free(result);
}

int main(int argc, char **argv)
{
    handle_options(argc, argv);

    if (!drizzle_user || !drizzle_passwd || !drizzle_dbname) {
        fprintf(stderr, "You need to specify:\n");
        if (!drizzle_user) {
            fprintf(stderr, "\t-U drizzle-user\n");
        }
        if (!drizzle_passwd) {
            fprintf(stderr, "\t-P drizzle-password\n");
        }
        if (!drizzle_dbname) {
            fprintf(stderr, "\t-S drizzle-database\n");
        }
        exit(1);
    }

    drizzle_init();
    struct event_base *evbase = event_init();

    libcouchbase_t instance = libcouchbase_create(host, username,
                                                  passwd, bucket, evbase);
    if (instance == NULL) {
        fprintf(stderr, "Failed to create libcouchbase instance\n");
        return 1;
    }

    if (libcouchbase_connect(instance) != LIBCOUCHBASE_SUCCESS) {
        fprintf(stderr, "Failed to connect libcouchbase instance to server\n");
        return 1;
    }

    libcouchbase_callback_t callbacks = {
        .tap_mutation = tap_mutation,
        .tap_deletion = tap_deletion,
        .tap_flush = tap_flush
    };
    libcouchbase_set_callbacks(instance, &callbacks);
    libcouchbase_tap_cluster(instance, NULL, true);

    return 0;
}

static void usage(char cmd, const void *arg, void *cookie)
{
    (void)cmd;
    (void)arg;
    (void)cookie;

    fprintf(stderr, "Usage: ./memdump [options]\n");
    for (int ii = 0; ii < 256; ++ii) {
        if (my_options[ii].name != NULL) {
            fprintf(stderr, "%s\n", my_options[ii++].description);
        }
    }
    exit(EXIT_FAILURE);
}
