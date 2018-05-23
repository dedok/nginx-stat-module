
/**
 * (c) BSD-2-Cause, link: https://opensource.org/licenses/BSD-2-Clause
 * (c) V. Soshnikov, mailto: dedok.mad@gmail.com
 */

#include "ngx_http_stat_allocator.h"


void
ngx_http_stat_allocator_init(ngx_http_stat_allocator_t *allocator,
        void *pool, void* (*alloc)(void *, size_t),
        void (*free)(void *, void *))
{
    allocator->pool = pool;
    allocator->alloc = alloc;
    allocator->free = free;
    allocator->nomemory = 0;
}


void *
ngx_http_stat_allocator_pool_alloc(void *pool, size_t size)
{
    return ngx_palloc((ngx_pool_t*) pool, size);
}


void
ngx_http_stat_allocator_pool_free(void *pool, void *p)
{
    if (p) {
        ngx_pfree((ngx_pool_t*) pool, p);
    }
}

void *
ngx_http_stat_allocator_slab_alloc(void *pool, size_t size)
{
    return ngx_slab_alloc_locked((ngx_slab_pool_t*) pool, size);
}


void
ngx_http_stat_allocator_slab_free(void *pool, void *p)
{
    if (p) {
        ngx_slab_free_locked((ngx_slab_pool_t*)pool, p);
    }
}


void *
ngx_http_stat_allocator_alloc(ngx_http_stat_allocator_t *allocator,
        size_t size)
{
    void *p;
    p = allocator->alloc(allocator->pool, size);
    if (p == NULL) {
        allocator->nomemory = 1;
    }
    return p;
}


void
ngx_http_stat_allocator_free(ngx_http_stat_allocator_t *allocator,
        void *p)
{
    if (p) {
        allocator->free(allocator->pool, p);
    }
}

