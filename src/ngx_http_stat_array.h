
/**
 * (c) BSD-2-Cause, link: https://opensource.org/licenses/BSD-2-Clause
 * (c) V. Soshnikov, mailto: dedok.mad@gmail.com
 */

#ifndef NGX_HTTP_STAT_ARRAY_H_INCLUDED
#define NGX_HTTP_STAT_ARRAY_H_INCLUDED 1

#include "ngx_http_stat_allocator.h"


typedef struct {
    void                      *elts;
    ngx_uint_t                 nelts;
    size_t                     size;
    ngx_uint_t                 nalloc;
    ngx_http_stat_allocator_t *allocator;
} ngx_http_stat_array_t;


ngx_http_stat_array_t *ngx_http_stat_array_create(
    ngx_http_stat_allocator_t *allocator, ngx_uint_t n, size_t size);
ngx_int_t ngx_http_stat_array_init(ngx_http_stat_array_t *array,
    ngx_http_stat_allocator_t *allocator, ngx_uint_t n, size_t size);
void ngx_http_stat_array_destroy(ngx_http_stat_array_t *array);
void *ngx_http_stat_array_push(ngx_http_stat_array_t *a);
void *ngx_http_stat_array_push_n(ngx_http_stat_array_t *a, ngx_uint_t n);
ngx_http_stat_array_t *ngx_http_stat_array_copy(
    ngx_http_stat_allocator_t *allocator, ngx_http_stat_array_t *array);

#endif /** NGX_HTTP_STAT_ARRAY_H_INCLUDED */

