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

#ifndef MK_HTTP_H
#define MK_HTTP_H

/* General status */
#define MK_HTTP_PENDING -10  /* cannot complete until more data arrives */
#define MK_HTTP_ERROR    -1  /* found an error when parsing the string */
#define MK_HTTP_OK        0

/* Request levels
 * ==============
 *
 * 1. FIRST_LINE         : Method, URI (+ QS) + Protocol version + CRLF
 * 2. HEADERS (optional) : KEY, SEP, VALUE + CRLF
 * 3. BODY (option)      : data based on Content-Length or Chunked transfer encoding
 */

enum {
    REQ_LEVEL_FIRST    = 1,
    REQ_LEVEL_HEADERS  = 2,
    REQ_LEVEL_BODY     = 3
};

/* Statuses per levels */
enum {
    /* REQ_LEVEL_FIRST */
    MK_ST_REQ_METHOD        = 1,
    MK_ST_REQ_URI           ,
    MK_ST_REQ_QUERY_STRING  ,
    MK_ST_REQ_PROT_VERSION  ,
    MK_ST_FIRST_CONTINUE    ,
    MK_ST_FIRST_FINALIZE    ,    /* LEVEL_FIRST finalize the request */

    /* REQ_HEADERS */
    MK_ST_HEADER_KEY        ,
    MK_ST_HEADER_VAL_STARTS ,
    MK_ST_HEADER_VALUE      ,
    MK_ST_HEADER_END        ,
    MK_ST_BLOCK_END         ,

    MK_ST_LF
};

typedef struct {
    int level;   /* request level */
    int status;  /* level status */
    int next;    /* something next after status ? */
    int length;

    /* lookup fields */
    int start;
    int end;
} mk_http_request_t;

mk_http_request_t *mk_http_request_new();
int mk_http_parser(mk_http_request_t *req, char *buffer, int len);


#ifdef HTTP_STANDALONE

/* ANSI Colors */
#define ANSI_RESET "\033[0m"
#define ANSI_BOLD  "\033[1m"

#define ANSI_CYAN          "\033[36m"
#define ANSI_BOLD_CYAN     ANSI_BOLD ANSI_CYAN
#define ANSI_MAGENTA       "\033[35m"
#define ANSI_BOLD_MAGENTA  ANSI_BOLD ANSI_MAGENTA
#define ANSI_RED           "\033[31m"
#define ANSI_BOLD_RED      ANSI_BOLD ANSI_RED
#define ANSI_YELLOW        "\033[33m"
#define ANSI_BOLD_YELLOW   ANSI_BOLD ANSI_YELLOW
#define ANSI_BLUE          "\033[34m"
#define ANSI_BOLD_BLUE     ANSI_BOLD ANSI_BLUE
#define ANSI_GREEN         "\033[32m"
#define ANSI_BOLD_GREEN    ANSI_BOLD ANSI_GREEN
#define ANSI_WHITE         "\033[37m"
#define ANSI_BOLD_WHITE    ANSI_BOLD ANSI_WHITE

#define TEST_OK      0
#define TEST_FAIL    1


static inline void p_field(mk_http_request_t *req, char *buffer)
{
    int i;

    printf("'");
    for (i = req->start; i < req->end; i++) {
        printf("%c", buffer[i]);
    }
    printf("'");

}

static inline int eval_field(mk_http_request_t *req, char *buffer)
{
    if (req->level == REQ_LEVEL_FIRST) {
        printf("[ \033[35mfirst level\033[0m ] ");
    }
    else {
        printf("[   \033[36mheaders\033[0m   ] ");
    }

    printf(" ");
    switch (req->status) {
    case MK_ST_REQ_METHOD:
        printf("MK_ST_REQ_METHOD       : ");
        break;
    case MK_ST_REQ_URI:
        printf("MK_ST_REQ_URI          : ");
        break;
    case MK_ST_REQ_QUERY_STRING:
        printf("MK_ST_REQ_QUERY_STRING : ");
        break;
    case MK_ST_REQ_PROT_VERSION:
        printf("MK_ST_REQ_PROT_VERSION : ");
        break;
    case MK_ST_HEADER_KEY:
        printf("MK_ST_HEADER_KEY       : ");
        break;
    case MK_ST_HEADER_VAL_STARTS:
        printf("MK_ST_HEADER_VAL_STARTS: ");
        break;
    default:
        printf("\033[31mUNKNOWN UNKNOWN\033[0m       : ");
        break;
    };


    p_field(req, buffer);
    printf("\n");

    return 0;
}



#endif


#endif
