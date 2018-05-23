
/**
 * (c) BSD-2-Cause, link: https://opensource.org/licenses/BSD-2-Clause
 * (c) V. Soshnikov, mailto: dedok.mad@gmail.com
 */

#include "ngx_http_stat_array.h"


ngx_http_stat_array_t *
ngx_http_stat_array_create(ngx_http_stat_allocator_t *allocator,
        ngx_uint_t n, size_t size)
{
    ngx_http_stat_array_t *array;

    array = ngx_http_stat_allocator_alloc(allocator,
            sizeof(ngx_http_stat_array_t));
    if (array == NULL) {
        return NULL;
    }

    if (ngx_http_stat_array_init(array, allocator, n, size) != NGX_OK) {
        return NULL;
    }

    return array;
}


ngx_int_t
ngx_http_stat_array_init(ngx_http_stat_array_t *array,
        ngx_http_stat_allocator_t *allocator, ngx_uint_t n,
        size_t size)
{
    array->nelts = 0;
    array->size = size;
    array->nalloc = n;
    array->allocator = allocator;

    array->elts = ngx_http_stat_allocator_alloc(allocator, n * size);
    if (array->elts == NULL) {
        return NGX_ERROR;
    }

    return NGX_OK;
}


void
ngx_http_stat_array_destroy(ngx_http_stat_array_t *array)
{
    ngx_http_stat_allocator_t *allocator;

    allocator = array->allocator;

    ngx_http_stat_allocator_free(allocator, array->elts);
    ngx_http_stat_allocator_free(allocator, array);
}


void *
ngx_http_stat_array_push(ngx_http_stat_array_t *a)
{
    return ngx_http_stat_array_push_n(a, 1);
}

void *
ngx_http_stat_array_push_n(ngx_http_stat_array_t *array, ngx_uint_t n)
{
    ngx_http_stat_allocator_t *allocator;
    ngx_uint_t                    nalloc;
    void                         *new, *elt;

    allocator = array->allocator;

    if (array->nelts == array->nalloc) {

        nalloc = 2 * ((n >= array->nalloc) ? n : array->nalloc);

        new = ngx_http_stat_allocator_alloc(allocator, nalloc * array->size);
        if (new == NULL) {
            return NULL;
        }

        ngx_memcpy(new, array->elts, array->nelts * array->size);
        ngx_http_stat_allocator_free(allocator, array->elts);
        array->elts = new;
        array->nalloc = nalloc;
    }

    elt = (u_char* )array->elts + array->size * array->nelts;
    array->nelts += n;

    return elt;
}

ngx_http_stat_array_t *
ngx_http_stat_array_copy(ngx_http_stat_allocator_t *allocator,
        ngx_http_stat_array_t *array)
{
    ngx_http_stat_array_t *copy;

    copy = ngx_http_stat_array_create(allocator, array->nalloc, array->size);

    if (copy == NULL) {
        return NULL;
    }

    if (ngx_http_stat_array_push_n(copy, array->nelts) == NULL) {
        return NULL;
    }

    ngx_memcpy(copy->elts, array->elts, array->nelts * array->size);

    return copy;
}

