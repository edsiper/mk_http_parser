/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*  Monkey HTTP Server
 *  ==================
 *  Copyright 2001-2014 Monkey Software LLC <eduardo@monkey.io>
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mk_http_parser.h"

#define mark_end()    req->end   = i; req->chars = -1; eval_field(req, buffer)
#define parse_next()  req->start = i + 1; continue
#define field_len()   (req->end - req->start)
#define header_scope_eq(req, x) req->header_min = req->header_max = x

struct header_entry {
    int len;
    const char name[32];
};

struct header_entry headers_table[] = {
    {  6, "Accept"              },
    { 14, "Accept-Charset"      },
    { 15, "Accept-Encoding"     },
    { 15, "Accept-Language"     },
    { 13, "Authorization"       },
    {  6, "Cookie"              },
    { 10, "Connection"          },
    { 14, "Content-Length"      },
    { 13, "Content-Range"       },
    { 12, "Content-Type"        },
    { 17, "If-Modified-Since"   },
    {  4, "Host"                },
    { 13, "Last-Modified"       },
    { 19, "Last-Modified-Since" },
    {  7, "Referer"             },
    {  5, "Range"               },
    { 10, "User-Agent"          }
};

/* Macro just for testing the parser on specific locations */
#define remaining()                                                     \
    {                                                                   \
        printf("%s** Line: %i / Chars: %i%s / remaining:\n",            \
               ANSI_BOLD, __LINE__, req->chars, ANSI_RESET);            \
        int x = 0;                                                      \
        for (x = i; x < len; x++) {                                     \
            if (buffer[x] == '\n') {                                    \
                printf("\\ n\n");                                       \
            }                                                           \
            else if (buffer[x] == '\r') {                               \
        printf("\\ r\n");                                               \
            }                                                           \
            else {                                                      \
                printf(" %c\n", buffer[x]);                             \
            }                                                           \
                                                                        \
        }                                                               \
    }

static inline int header_lookup(struct mk_http_parser *req, char *buffer)
{
    int i;
    int len;
    struct mk_http_header *header;
    struct header_entry *h;

    len = (req->header_sep - req->header_key);

    for (i = req->header_min; i <= req->header_max; i++) {
        h = &headers_table[i];

        /* Check string length first */
        if (h->len != len) {
            continue;
        }

        if (strncmp(buffer + req->header_key + 1,
                    h->name + 1,
                    len - 1) == 0) {

            /* We got a header match, register the header index */
            header = &req->headers[i];
            header->type = i;
            header->key.data = buffer + req->header_key;
            header->key.len  = len;
            header->val.data = buffer + req->header_val;
            header->val.len  = req->end - req->header_val;

            if (i == MK_HEADER_CONTENT_LENGTH) {
                req->header_content_length = atol(header->val.data);
                if (req->header_content_length < 0) {
                    return -1;
                }
            }

            printf("                 ===> %sMATCH%s '%s' = '",
                   ANSI_YELLOW, ANSI_RESET, h->name);


            int z;
            for (z = req->header_val; z < req->end; z++) {
                printf("%c", buffer[z]);
            }
            printf("'\n");

            /* FIXME: register header value */
            return 0;
        }
    }

    printf("                 ===> %sUNKNOWN HEADER%s",
           ANSI_RED, ANSI_RESET);

    printf("\n");
    return 0;
}

/*
 * Parse the protocol and point relevant fields, don't take logic decisions
 * based on this, just parse to locate things.
 */
int mk_http_parser(struct mk_http_parser *req, char *buffer, int len)
{
    int i;
    int ret;

    for (i = 0; i < len; i++, req->chars++) {
        /* FIRST LINE LEVEL: Method, URI & Protocol */
        if (req->level == REQ_LEVEL_FIRST) {
            switch (req->status) {
            case MK_ST_REQ_METHOD:                      /* HTTP Method */
                if (buffer[i] == ' ') {
                    mark_end();
                    req->status = MK_ST_REQ_URI;
                    if (req->end < 2) {
                        return MK_HTTP_ERROR;
                    }
                    parse_next();
                }
                break;
            case MK_ST_REQ_URI:                         /* URI */
                if (buffer[i] == ' ') {
                    mark_end();
                    req->status = MK_ST_REQ_PROT_VERSION;
                    if (field_len() < 1) {
                        return MK_HTTP_ERROR;
                    }
                    parse_next();
                }
                else if (buffer[i] == '?') {
                    mark_end();
                    req->status = MK_ST_REQ_QUERY_STRING;
                    parse_next();
                }
                break;
            case MK_ST_REQ_QUERY_STRING:                /* Query string */
                if (buffer[i] == ' ') {
                    mark_end();
                    req->status = MK_ST_REQ_PROT_VERSION;
                    parse_next();
                }
                break;
            case MK_ST_REQ_PROT_VERSION:                /* Protocol Version */
                if (buffer[i] == '\r') {
                    mark_end();
                    if (field_len() != 8) {
                        return MK_HTTP_ERROR;
                    }
                    req->status = MK_ST_LF;
                    continue;
                }
                break;
            case MK_ST_LF:                              /* New Line */
                if (buffer[i] == '\n') {
                    req->status = MK_ST_FIRST_CONTINUE;
                    continue;
                }
                else {
                    return MK_HTTP_ERROR;
                }
                break;
            case MK_ST_FIRST_CONTINUE:                  /* First line finalizing */
                if (buffer[i] == '\r') {
                    req->status = MK_ST_FIRST_FINALIZE;
                    continue;
                }
                else {
                    req->level = REQ_LEVEL_HEADERS;
                    req->status = MK_ST_HEADER_KEY;
                    i--;
                    mark_end();
                    parse_next();
                }
                break;
            case MK_ST_FIRST_FINALIZE:                 /* First line END */
                if (buffer[i] == '\n') {
                    req->level  = REQ_LEVEL_HEADERS;
                    req->status = MK_ST_HEADER_KEY;
                    parse_next();
                }
                else {
                    return MK_HTTP_ERROR;
                }
            };
        }
        /* HEADERS: all headers stuff */
        else if (req->level == REQ_LEVEL_HEADERS) {
            /* Expect a Header key */
            if (req->status == MK_ST_HEADER_KEY) {
                if (buffer[i] == '\r') {
                    if (req->chars == 0) {
                        req->level = REQ_LEVEL_END;
                        parse_next();
                    }
                    else {
                        return MK_HTTP_ERROR;
                    }
                }

                if (req->chars == 0) {
                    /*
                     * We reach the start of a Header row, lets catch the most
                     * probable header. Note that we don't accept headers starting
                     * in lowercase.
                     *
                     * The goal of this 'first row character lookup', is to define a
                     * small range set of probable headers comparison once we catch
                     * a header end.
                     */
                    switch (buffer[i]) {
                    case 'A':
                        req->header_min = MK_HEADER_ACCEPT;
                        req->header_max = MK_HEADER_AUTHORIZATION;
                        break;
                    case 'C':
                        req->header_min = MK_HEADER_COOKIE;
                        req->header_max = MK_HEADER_CONTENT_TYPE;
                        break;
                    case 'I':
                        header_scope_eq(req, MK_HEADER_IF_MODIFIED_SINCE);
                        break;
                    case 'H':
                        header_scope_eq(req, MK_HEADER_HOST);
                        break;
                    case 'L':
                        req->header_min = MK_HEADER_LAST_MODIFIED;
                        req->header_max = MK_HEADER_LAST_MODIFIED_SINCE;
                        break;
                    case 'R':
                        req->header_min = MK_HEADER_REFERER;
                        req->header_max = MK_HEADER_RANGE;
                        break;
                    case 'U':
                        header_scope_eq(req, MK_HEADER_USER_AGENT);
                        break;
                    default:
                        req->header_key = -1;
                        req->header_sep = -1;
                        req->header_min = -1;
                        req->header_max = -1;
                    };
                }

                /* Found key/value separator */
                if (buffer[i] == ':') {
                    /* Set the key/value middle point */
                    req->header_key = req->start;
                    req->header_sep = i;

                    mark_end();

                    /* validate length */
                    if (field_len() < 1) {
                        return MK_HTTP_ERROR;
                    }

                    /* Wait for a value */
                    req->status = MK_ST_HEADER_VALUE;
                    parse_next();
                }
            }
            /* Parsing the header value */
            else if (req->status == MK_ST_HEADER_VALUE) {
                /* Trim left, set starts only when found something != ' ' */
                if (buffer[i] != ' ') {
                    req->status = MK_ST_HEADER_VAL_STARTS;
                    req->header_val = i;
                    i--;
                    parse_next();
                }
            }
            /* New header row starts */
            else if (req->status == MK_ST_HEADER_VAL_STARTS) {
                /* Maybe there is no more headers and we reach the end ? */
                if (buffer[i] == '\r') {
                    mark_end();
                    req->status = MK_ST_HEADER_END;
                    if (field_len() <= 0) {
                        return MK_HTTP_ERROR;
                    }

                    /*
                     * A header row has ended, lets lookup the header and populate
                     * our headers table index.
                     */
                    ret = header_lookup(req, buffer);
                    if (ret != 0) {
                        return MK_HTTP_ERROR;
                    }
                    parse_next();
                }
                else if (buffer[i] == '\n' && buffer[i - 1] != '\r') {
                    return MK_HTTP_ERROR;
                }
                continue;
            }
            else if (req->status == MK_ST_HEADER_END) {
                if (buffer[i] == '\n') {
                    req->status = MK_ST_HEADER_KEY;
                    req->chars = -1;
                    parse_next();
                }
                else {
                    return MK_HTTP_ERROR;
                }
            }
        }
        else if (req->level == REQ_LEVEL_END) {
            if (buffer[i] == '\n') {
                req->level = REQ_LEVEL_BODY;
                req->chars = -1;
                parse_next();
            }
            else {
                return MK_HTTP_ERROR;
            }
        }
        else if (req->level == REQ_LEVEL_BODY) {
            /*
             * Reaching this level can means two things:
             *
             * - A Pipeline Request
             * - A Body content (POST/PUT methods
             */
            if (req->header_content_length > 0) {
                req->body_received = len - i;
                if (req->body_received == req->header_content_length) {
                    return MK_HTTP_OK;
                }
                else {
                    return MK_HTTP_PENDING;
                }
            }
            return MK_HTTP_OK;
        }
    }

    if (req->level == REQ_LEVEL_FIRST) {
        if (req->status == MK_ST_REQ_METHOD) {
            if (field_len() == 0 || field_len() > 10) {
                return MK_HTTP_ERROR;
            }
        }

    }

    /* No headers */
    if (req->level == REQ_LEVEL_HEADERS) {
        if (req->status == MK_ST_HEADER_KEY) {
            return MK_HTTP_OK;
        }
        else {
        }
    }
    else if (req->level == REQ_LEVEL_BODY) {
        if (req->header_content_length > 0 &&
            req->body_received < req->header_content_length) {
            return MK_HTTP_PENDING;
        }
        else if (req->chars == 0) {
            return MK_HTTP_OK;
        }
        else {
        }

    }

    return MK_HTTP_PENDING;
}

struct mk_http_parser *mk_http_parser_new()
{
    struct mk_http_parser *req;

    req = malloc(sizeof(struct mk_http_parser));
    req->level  = REQ_LEVEL_FIRST;
    req->status = MK_ST_REQ_METHOD;
    req->length = 0;
    req->start  = 0;
    req->end    = 0;
    req->chars  = -1;

    /* init headers */
    req->header_min = -1;
    req->header_max = -1;
    req->header_sep = -1;
    req->body_received  = 0;
    req->header_content_length = 0;

    return req;
}
