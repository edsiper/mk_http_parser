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

#include "mk_http_parser.h"

#define mark_end()    req->end   = i; req->chars = 0; eval_field(req, buffer)
#define parse_next()  req->start = i + 1; req->chars++; continue
#define field_len()   (req->end - req->start)

/*
 * Parse the protocol and point relevant fields, don't take logic decisions
 * based on this, just parse to locate things.
 */
int mk_http_parser(mk_http_request_t *req, char *buffer, int len)
{
    int i;

    for (i = 0; i < len; i++) {
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
                /* Check if is there is no more headers */
                if (req->chars == 0) {
                    if (buffer[i+1] == '\r') {
                        req->level  = REQ_LEVEL_END;
                        parse_next();
                    }
                }
                /* Found key/value separator */
                else if (buffer[i] == ':') {
                    mark_end();
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
                    parse_next();
                }
                continue;
            }
            else if (req->status == MK_ST_HEADER_END) {
                if (buffer[i] == '\n') {
                    req->status = MK_ST_HEADER_KEY;
                    req->chars = -1;
                    parse_next();
                }
            }
        }
        else if (req->level == REQ_LEVEL_END) {
            if (buffer[i] == '\r') {
                printf("here2!\n");
                req->status = MK_ST_BLOCK_END;
                req->chars = -1;
                parse_next();
            }
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

    return MK_HTTP_PENDING;
}

mk_http_request_t *mk_http_request_new()
{
    mk_http_request_t *req;

    req = malloc(sizeof(mk_http_request_t));
    req->level  = REQ_LEVEL_FIRST;
    req->status = MK_ST_REQ_METHOD;
    req->length = 0;
    req->start  = 0;
    req->end    = 0;

    return req;
}
