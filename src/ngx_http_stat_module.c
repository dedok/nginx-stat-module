
/**
 * (c) BSD-2-Cause, link: https://opensource.org/licenses/BSD-2-Clause
 * (c) V. Soshnikov, mailto: dedok.mad@gmail.com
 */

#include "ngx_http_stat_module.h"


/** FWD {{{ */
typedef char *(*ngx_http_stat_arg_handler_pt)(ngx_http_stat_ctx_t *,
        void *, ngx_str_t *);

typedef struct {
    ngx_str_t                    name;
    ngx_http_stat_arg_handler_pt handler;
    ngx_str_t                    deflt;
} ngx_http_stat_arg_t;

/** Sources (i.e. locations or...) {{{ */
struct ngx_http_stat_source_s;

typedef double (*ngx_http_stat_source_handler_pt)(
        struct ngx_http_stat_source_s *source, ngx_http_request_t*);

typedef struct ngx_http_stat_source_s {
    ngx_str_t                           name;
    ngx_flag_t                          re;
    ngx_http_stat_source_handler_pt     get;
    ngx_http_stat_aggregate_pt          aggregate;
    ngx_uint_t                          type;
} ngx_http_stat_source_t;

static double ngx_http_stat_source_request_time(
        ngx_http_stat_source_t *source, ngx_http_request_t *r);
static double ngx_http_stat_source_bytes_sent(
        ngx_http_stat_source_t *source, ngx_http_request_t *r);
static double ngx_http_stat_source_body_bytes_sent(
        ngx_http_stat_source_t *source, ngx_http_request_t *r);
static double ngx_http_stat_source_request_length(
        ngx_http_stat_source_t *source, ngx_http_request_t *r);
static double ngx_http_stat_source_rps(
        ngx_http_stat_source_t *source, ngx_http_request_t *r);
static double ngx_http_stat_source_keepalive_rps(
        ngx_http_stat_source_t *source, ngx_http_request_t *r);
static double ngx_http_stat_source_response_2xx_rps(
        ngx_http_stat_source_t *source, ngx_http_request_t *r);
static double ngx_http_stat_source_response_3xx_rps(
        ngx_http_stat_source_t *source, ngx_http_request_t *r);
static double ngx_http_stat_source_response_4xx_rps(
        ngx_http_stat_source_t *source, ngx_http_request_t *r);
static double ngx_http_stat_source_response_5xx_rps(
        ngx_http_stat_source_t *source, ngx_http_request_t *r);
static double ngx_http_stat_source_response_xxx_rps(
        ngx_http_stat_source_t *source, ngx_http_request_t *r);
static double ngx_http_stat_source_upstream_cache_status_rps(
        ngx_http_stat_source_t *source, ngx_http_request_t *r);
/** }}} */

typedef enum {
    TEMPLATE_VARIABLE_LOCATION,
} ngx_http_stat_default_data_variable_t;

static ngx_http_complex_value_t *ngx_http_stat_complex_compile(
        ngx_conf_t *cf, ngx_str_t *value);

static ngx_int_t ngx_http_stat_handler(ngx_http_request_t *r);
static ngx_int_t ngx_http_stat_shared_init(ngx_shm_zone_t *shm_zone,
        void *data);

static ngx_int_t ngx_http_stat_add_variables(ngx_conf_t *cf);
static ngx_int_t ngx_http_stat_init(ngx_conf_t *cf);
static ngx_int_t ngx_http_stat_process_init(ngx_cycle_t *cycle);

static void *ngx_http_stat_create_main_conf(ngx_conf_t *cf);
static void *ngx_http_stat_create_srv_conf(ngx_conf_t *cf);
static void *ngx_http_stat_create_loc_conf(ngx_conf_t *cf);
static char *ngx_http_stat_merge_loc_conf(ngx_conf_t* cf, void* parent,
        void* child);

static ngx_str_t *ngx_http_stat_location(ngx_pool_t *pool, ngx_str_t *uri);

static char *ngx_http_stat_config(ngx_conf_t *cf, ngx_command_t *cmd,
        void *conf);
static char *ngx_http_stat_default_data(ngx_conf_t *cf, ngx_command_t *cmd,
        void *conf);
static char *ngx_http_stat_data(ngx_conf_t *cf, ngx_command_t *cmd,
        void *conf);
static char *ngx_http_stat_param(ngx_conf_t *cf, ngx_command_t *cmd,
        void *conf);

static char *ngx_http_stat_add_default_data(ngx_conf_t *cf, ngx_array_t *datas,
        ngx_str_t *location, ngx_array_t *template,  ngx_array_t *params,
        ngx_http_complex_value_t *filter);
static char *ngx_http_stat_add_data(ngx_conf_t *cf, ngx_array_t *datas,
        ngx_str_t *split,  ngx_array_t *params, ngx_http_complex_value_t *filter);

static char *ngx_http_stat_config_arg_host(ngx_http_stat_ctx_t *ctx,
        void *data, ngx_str_t *value);
static char *ngx_http_stat_config_arg_server(ngx_http_stat_ctx_t *ctx,
        void *data, ngx_str_t *value);
static char *ngx_http_stat_config_arg_port(ngx_http_stat_ctx_t *ctx,
        void *data, ngx_str_t *value);
static char *ngx_http_stat_config_arg_frequency(ngx_http_stat_ctx_t *ctx,
        void *data, ngx_str_t *value);
static char *ngx_http_stat_config_arg_intervals(ngx_http_stat_ctx_t *ctx,
        void *data, ngx_str_t *value);
static char *ngx_http_stat_config_arg_params(ngx_http_stat_ctx_t *ctx,
        void *data, ngx_str_t *value);
static char *ngx_http_stat_config_arg_shared(ngx_http_stat_ctx_t *ctx,
        void *data, ngx_str_t *value);
static char *ngx_http_stat_config_arg_buffer(ngx_http_stat_ctx_t *ctx,
        void *data, ngx_str_t *value);
static char *ngx_http_stat_config_arg_package(ngx_http_stat_ctx_t *ctx,
        void *data, ngx_str_t *value);
static char *ngx_http_stat_config_arg_template(ngx_http_stat_ctx_t *ctx,
        void *data, ngx_str_t *value);
static char *ngx_http_stat_config_arg_protocol(ngx_http_stat_ctx_t *ctx,
        void *data, ngx_str_t *value);
static char *ngx_http_stat_config_arg_timeout(ngx_http_stat_ctx_t *ctx,
        void *data, ngx_str_t *value);

static char *ngx_http_stat_param_arg_name(ngx_http_stat_ctx_t *ctx,
        void *data, ngx_str_t *value);
static char *ngx_http_stat_param_arg_aggregate(ngx_http_stat_ctx_t *ctx,
        void *data, ngx_str_t *value);
static char *ngx_http_stat_param_arg_interval(ngx_http_stat_ctx_t *ctx,
        void *data, ngx_str_t *value);
static char *ngx_http_stat_param_arg_percentile(ngx_http_stat_ctx_t *ctx,
        void *data, ngx_str_t *value);

static char *ngx_http_stat_parse_params(ngx_http_stat_ctx_t *ctx,
        ngx_str_t *value, ngx_array_t *params);
static char *ngx_http_stat_parse_string(ngx_http_stat_ctx_t *ctx,
        ngx_str_t *value, ngx_str_t *result);
static char *ngx_http_stat_parse_size(ngx_http_stat_ctx_t *ctx,
        ngx_str_t *value, ngx_uint_t *result);
static char *ngx_http_stat_parse_time(ngx_http_stat_ctx_t *ctx,
        ngx_str_t *value, ngx_uint_t *result);
/** }}} */

/** Vars {{{ */
static ngx_command_t ngx_http_stat_commands[] = {

    { ngx_string("stat_config"),
      NGX_HTTP_MAIN_CONF|NGX_CONF_ANY,
      ngx_http_stat_config,
      NGX_HTTP_MAIN_CONF_OFFSET,
      0,
      NULL },

    { ngx_string("stat_param"),
      NGX_HTTP_LOC_CONF|NGX_HTTP_LIF_CONF|NGX_CONF_TAKE2|NGX_CONF_TAKE3|
          NGX_CONF_TAKE4,
      ngx_http_stat_param,
      NGX_HTTP_LOC_CONF_OFFSET,
      0,
      NULL },

    { ngx_string("stat_default"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_CONF_TAKE12,
      ngx_http_stat_default_data,
      0,
      0,
      NULL },

    { ngx_string("stat"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_HTTP_LIF_CONF|
          NGX_CONF_TAKE123,
      ngx_http_stat_data,
      0,
      0,
      NULL },

      ngx_null_command
};


static ngx_http_module_t ngx_http_stat_module_ctx = {
    ngx_http_stat_add_variables,       /* preconfiguration */
    ngx_http_stat_init,                /* postconfiguration */

    ngx_http_stat_create_main_conf,    /* create main configuration */
    NULL,                              /* init main configuration */

    ngx_http_stat_create_srv_conf,     /* create server configuration */
    NULL,                              /* merge server configuration */

    ngx_http_stat_create_loc_conf,     /* create location configuration */
    ngx_http_stat_merge_loc_conf,      /* merge location configuration */
};


ngx_module_t ngx_http_stat_module = {
    NGX_MODULE_V1,
    &ngx_http_stat_module_ctx,         /* module ctx */
    ngx_http_stat_commands,            /* module directives */
    NGX_HTTP_MODULE,                   /* module type */
    NULL,                              /* init master */
    NULL,                              /* init module */
    ngx_http_stat_process_init,        /* init process */
    NULL,                              /* init thread */
    NULL,                              /* exit thread */
    NULL,                              /* exit process */
    NULL,                              /* exit master */
    NGX_MODULE_V1_PADDING
};

static ngx_str_t stat_shared_name = ngx_string("_ngx_stat_module");

static ngx_http_variable_t ngx_http_stat_vars[] = {
    { ngx_null_string, NULL, NULL, 0, 0, 0 }
};

static  ngx_http_stat_arg_t ngx_http_stat_config_args[] = {
    { ngx_string("host"),
      ngx_http_stat_config_arg_host,
      ngx_null_string },
    { ngx_string("server"),
      ngx_http_stat_config_arg_server,
      ngx_null_string },
    { ngx_string("port"),
      ngx_http_stat_config_arg_port,
      ngx_string("8089") },
    { ngx_string("frequency"),
      ngx_http_stat_config_arg_frequency,
      ngx_string("60") },
    { ngx_string("intervals"),
      ngx_http_stat_config_arg_intervals,
      ngx_string("1m") },
    { ngx_string("params"),
      ngx_http_stat_config_arg_params,
      ngx_string(DEFAULT_PARAMS)},
    { ngx_string("shared"),
      ngx_http_stat_config_arg_shared,
      ngx_string("1m") },
    { ngx_string("buffer"),
      ngx_http_stat_config_arg_buffer,
      ngx_string("64k") },
    { ngx_string("package"),
      ngx_http_stat_config_arg_package,
      ngx_string("1400") },
    { ngx_string("template"),
      ngx_http_stat_config_arg_template,
      ngx_null_string },
    { ngx_string("protocol"),
      ngx_http_stat_config_arg_protocol,
      ngx_null_string },
    { ngx_string("timeout"),
      ngx_http_stat_config_arg_timeout,
      ngx_string("100") }
};


static  ngx_http_stat_arg_t ngx_http_stat_param_args[] = {
    { ngx_string("name"),
      ngx_http_stat_param_arg_name,
      ngx_null_string },
    { ngx_string("aggregate"),
      ngx_http_stat_param_arg_aggregate,
      ngx_null_string },
    { ngx_string("interval"),
      ngx_http_stat_param_arg_interval,
      ngx_null_string },
    { ngx_string("percentile"),
      ngx_http_stat_param_arg_percentile,
      ngx_null_string },
};

static ngx_event_t timer;

/** Metrics & acc functions & statistics {{{ */
static ngx_http_stat_aggregate_t ngx_http_stat_aggregates[] = {

    {   ngx_string("avg"),
        ngx_http_stat_aggregate_avg },
    {   ngx_string("persec"),
        ngx_http_stat_aggregate_persec },
    {   ngx_string("sum"),
        ngx_http_stat_aggregate_sum },
};
/** }}} */

static  ngx_http_stat_source_t ngx_http_stat_sources[] = {

    {   .name = ngx_string("request_time"),
        .get = ngx_http_stat_source_request_time,
        .aggregate = ngx_http_stat_aggregate_avg },

    {   .name = ngx_string("bytes_sent"),
        .get = ngx_http_stat_source_bytes_sent,
        .aggregate = ngx_http_stat_aggregate_avg },

    {   .name = ngx_string("body_bytes_sent"),
        .get = ngx_http_stat_source_body_bytes_sent,
        .aggregate = ngx_http_stat_aggregate_avg },

    {   .name = ngx_string("request_length"),
        .get = ngx_http_stat_source_request_length,
        .aggregate = ngx_http_stat_aggregate_avg },

    {   .name = ngx_string("rps"),
        .get = ngx_http_stat_source_rps,
        .aggregate = ngx_http_stat_aggregate_persec },

    {   .name = ngx_string("keepalive_rps"),
        .get = ngx_http_stat_source_keepalive_rps,
        .aggregate = ngx_http_stat_aggregate_persec },

    {   .name = ngx_string("response_2xx_rps"),
        .get = ngx_http_stat_source_response_2xx_rps,
        .aggregate = ngx_http_stat_aggregate_persec },

    {   .name = ngx_string("response_3xx_rps"),
        .get = ngx_http_stat_source_response_3xx_rps,
        .aggregate = ngx_http_stat_aggregate_persec },

    {   .name = ngx_string("response_4xx_rps"),
        .get = ngx_http_stat_source_response_4xx_rps,
        .aggregate = ngx_http_stat_aggregate_persec },

    {   .name = ngx_string("response_5xx_rps"),
        .get = ngx_http_stat_source_response_5xx_rps,
        .aggregate = ngx_http_stat_aggregate_persec },

    {   .name = ngx_string("response_\\d\\d\\d_rps"),
        .re = 1,
        .get = ngx_http_stat_source_response_xxx_rps,
        .aggregate = ngx_http_stat_aggregate_persec },

    {   .name = ngx_string("upstream_cache_(miss|bypass|expired|stale|updating|revalidated|hit)_rps"),
        .re = 1,
        .get = ngx_http_stat_source_upstream_cache_status_rps,
        .aggregate = ngx_http_stat_aggregate_persec },

    {   .name = ngx_string("upstream_time"),
        .get = ngx_http_stat_source_upstream_time,
        .aggregate = ngx_http_stat_aggregate_avg },

    {   .name = ngx_string("upstream_header_time"),
        .get = ngx_http_stat_source_upstream_header_time,
        .aggregate = ngx_http_stat_aggregate_avg },
};

static  ngx_http_stat_template_arg_t ngx_http_stat_template_args[] = {
    { ngx_string("host"), TEMPLATE_VARIABLE_HOST },
    { ngx_string("split"), TEMPLATE_VARIABLE_SPLIT },
    { ngx_string("param"), TEMPLATE_VARIABLE_PARAM },
    { ngx_string("interval"), TEMPLATE_VARIABLE_INTERVAL },
};

static  ngx_http_stat_template_arg_t ngx_http_stat_default_data_args[] = {
    { ngx_string("location"), TEMPLATE_VARIABLE_LOCATION },
};
/** }}} */

/** Implementation {{{ */
static
ngx_int_t
ngx_http_stat_add_variables(ngx_conf_t *cf)
{
    ngx_http_variable_t *var, *v;

    for (v = ngx_http_stat_vars; v->name.len; v++) {
        var = ngx_http_add_variable(cf, &v->name, v->flags);
        if (var == NULL) {
            return NGX_ERROR;
        }

        var->get_handler = v->get_handler;
        var->data = v->data;
    }

    return NGX_OK;
}


static
ngx_int_t
ngx_http_stat_init(ngx_conf_t *cf)
{
    ngx_http_handler_pt       *h;
    ngx_http_core_main_conf_t *cmcf;

    cmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module);

    h = ngx_array_push(&cmcf->phases[NGX_HTTP_LOG_PHASE].handlers);
    if (h == NULL) {
        return NGX_ERROR;
    }

    *h = ngx_http_stat_handler;

    return NGX_OK;
}


static
ngx_int_t
ngx_http_stat_process_init(ngx_cycle_t *cycle)
{
    ngx_http_stat_main_conf_t *smcf;

    smcf = ngx_http_cycle_get_module_main_conf(cycle, ngx_http_stat_module);

    if (!smcf->enable) {
        return NGX_OK;
    }

    ngx_memzero(&timer, sizeof(timer));

    if (smcf->protocol.len > sizeof("influx/") - 1 &&
            ngx_strncmp(smcf->protocol.data, "influx/udp",
                sizeof("influx/udp") - 1) == 0)
    {
        timer.handler = ngx_http_influx_udp_timer_handler;
    } else {
        ngx_log_error(NGX_LOG_CRIT, cycle->log, 0,
            "a protocol does not supported \"%V\", for server \"%V\"",
            &smcf->protocol, &smcf->server.name);
        return NGX_ERROR;
    }

    timer.data = smcf;
    timer.log = cycle->log;

    ngx_add_timer(&timer, smcf->frequency);

    return NGX_OK;
}


static
void *
ngx_http_stat_create_main_conf(ngx_conf_t *cf)
{
    ngx_http_stat_main_conf_t     *smcf;
    ngx_http_stat_allocator_t     *allocator;

    smcf = ngx_pcalloc(cf->pool, sizeof(ngx_http_stat_main_conf_t));
    if (smcf == NULL) {
        return NULL;
    }

    smcf->sources = ngx_array_create(cf->pool, 1, sizeof(ngx_http_stat_source_t));
    smcf->splits = ngx_array_create(cf->pool, 1, sizeof(ngx_str_t));
    smcf->intervals = ngx_array_create(cf->pool, 1, sizeof(ngx_http_stat_interval_t));
    smcf->default_params = ngx_array_create(cf->pool, 1, sizeof(ngx_uint_t));
    smcf->template = ngx_array_create(cf->pool, 1, sizeof(ngx_http_stat_template_t));
    smcf->default_data_template = ngx_array_create(cf->pool, 1, sizeof(ngx_http_stat_template_t));
    smcf->default_data_params = ngx_array_create(cf->pool, 1, sizeof(ngx_uint_t));
    smcf->datas = ngx_array_create(cf->pool, 1, sizeof(ngx_http_stat_data_t));

    if (smcf->sources == NULL ||
        smcf->splits == NULL ||
        smcf->intervals == NULL ||
        smcf->default_params == NULL ||
        smcf->template == NULL ||
        smcf->default_data_template == NULL ||
        smcf->default_data_params == NULL ||
        smcf->datas == NULL)
    {
        return NULL;
    }

    smcf->storage = ngx_pcalloc(cf->pool, sizeof(ngx_http_stat_storage_t));
    if (smcf->storage == NULL) {
        return NULL;
    }

    allocator = ngx_palloc(cf->pool, sizeof(ngx_http_stat_allocator_t));
    if (allocator == NULL) {
        return NULL;
    }

    ngx_http_stat_allocator_init(allocator, cf->pool,
            ngx_http_stat_allocator_pool_alloc,
            ngx_http_stat_allocator_pool_free);

    smcf->storage->allocator = allocator;
    smcf->storage->params = ngx_http_stat_array_create(allocator,
            1, sizeof(ngx_http_stat_param_t));
    smcf->storage->internals = ngx_http_stat_array_create(allocator,
            1, sizeof(ngx_http_stat_internal_t));
    smcf->storage->metrics = ngx_http_stat_array_create(allocator,
            1, sizeof(ngx_http_stat_metric_t));
    smcf->storage->statistics = ngx_http_stat_array_create(allocator,
            1, sizeof(ngx_http_stat_statistic_t));

    if (smcf->storage->params == NULL ||
        smcf->storage->internals == NULL ||
        smcf->storage->metrics == NULL ||
        smcf->storage->statistics == NULL)
    {
        return NULL;
    }

    return smcf;
}


static
void *
ngx_http_stat_create_srv_conf(ngx_conf_t *cf)
{
    ngx_http_stat_srv_conf_t *sscf;

    sscf = ngx_pcalloc(cf->pool, sizeof(ngx_http_stat_srv_conf_t));
    if (sscf == NULL) {
        return NULL;
    }

    sscf->default_data_template = ngx_array_create(cf->pool,
            1, sizeof(ngx_http_stat_template_t));
    sscf->default_data_params = ngx_array_create(cf->pool,
            1, sizeof(ngx_uint_t));
    sscf->datas = ngx_array_create(cf->pool,
            1, sizeof(ngx_http_stat_data_t));

    if (sscf->default_data_template == NULL ||
        sscf->default_data_params == NULL ||
        sscf->datas == NULL)
    {
        return NULL;
    }

    return sscf;
}


static void *
ngx_http_stat_create_loc_conf(ngx_conf_t *cf)
{
    ngx_http_stat_main_conf_t *smcf;
    ngx_http_stat_loc_conf_t  *slcf;
    ngx_http_stat_srv_conf_t  *sscf;
    ngx_str_t                  loc, *uri, *location, *directive;

    slcf = ngx_pcalloc(cf->pool, sizeof(ngx_http_stat_loc_conf_t));
    if (slcf == NULL) {
        return NULL;
    }

    slcf->datas = ngx_array_create(cf->pool, 1,
            sizeof(ngx_http_stat_data_t));
    if (slcf->datas == NULL) {
        return NULL;
    }

    if (!cf->args) {
        return slcf;
    }

    directive = &((ngx_str_t*) cf->args->elts)[0];

    ngx_str_set(&loc, "location");

    smcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_stat_module);

    if (cf->args->nelts >= 2 &&
        directive->len == loc.len &&
        ngx_strncmp(directive->data, loc.data, loc.len) == 0)
    {
        sscf = ngx_http_conf_get_module_srv_conf(cf, ngx_http_stat_module);

        uri = &((ngx_str_t*)cf->args->elts)[cf->args->nelts - 1];
        location = ngx_http_stat_location(cf->pool, uri);
        if (location == NULL) {
            return NULL;
        }

        if (location->len == 0) {
            return slcf;
        }

        if (ngx_http_stat_add_default_data(cf, slcf->datas, location,
                    smcf->default_data_template, smcf->default_data_params,
                    smcf->default_data_filter) != NGX_CONF_OK)
        {
            return NULL;
        }

        if (ngx_http_stat_add_default_data(cf, slcf->datas, location,
                    sscf->default_data_template, sscf->default_data_params,
                    sscf->default_data_filter) != NGX_CONF_OK) {
            return NULL;
        }
    }

    return slcf;
}



static
char *
ngx_http_stat_merge_loc_conf(ngx_conf_t* cf, void* parent, void* child)
{
    ngx_http_stat_loc_conf_t *prev = parent;
    ngx_http_stat_loc_conf_t *conf = child;

    ngx_uint_t                  i;
    ngx_http_stat_data_t *prev_data;
    ngx_http_stat_data_t *data;

    for (i = 0; i < prev->datas->nelts; i++) {
        prev_data = &((ngx_http_stat_data_t*) prev->datas->elts)[i];
        data = ngx_array_push(conf->datas);
        if (data == NULL) {
            return NGX_CONF_ERROR;
        }

        *data = *prev_data;
    }

    return NGX_CONF_OK;
}


static
ngx_str_t *
ngx_http_stat_location(ngx_pool_t *pool,  ngx_str_t *uri)
{
    ngx_str_t      *split;
    ngx_uint_t      i;

    split = ngx_palloc(pool, sizeof(ngx_str_t));
    if (split == NULL) {
        return NULL;
    }

    split->data = ngx_palloc(pool, uri->len);
    split->len = 0;
    if (split->data == NULL) {
        return NULL;
    }

    for (i = 0; i < uri->len; i++) {

        if (isalnum(uri->data[i])) {
            split->data[split->len++] = uri->data[i];
        } else {
            split->data[split->len++] = '_';
        }
    }

    while (split->len > 0 && split->data[0] == '_') {
        split->data++;
        split->len--;
    }

    while (split->len > 0 && split->data[split->len - 1] == '_') {
        split->len--;
    }

    return split;
}


static
ngx_http_stat_ctx_t
ngx_http_stat_ctx_from_config(ngx_conf_t *cf)
{
    ngx_http_stat_main_conf_t *smcf;
    ngx_http_stat_ctx_t        ctx;

    smcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_stat_module);

    ctx.phase = PHASE_CONFIG;
    ctx.storage = smcf->storage;
    ctx.pool = cf->pool;
    ctx.log = cf->log;
    ctx.smcf = smcf;

    return ctx;
}


static
ngx_http_stat_ctx_t
ngx_http_stat_ctx_from_request(ngx_http_request_t *r)
{
    ngx_http_stat_main_conf_t *smcf;
    ngx_http_stat_storage_t   *storage;
    ngx_slab_pool_t           *shpool;

    storage = NULL;

    smcf = ngx_http_get_module_main_conf(r, ngx_http_stat_module);

    if (smcf->enable) {
        shpool = (ngx_slab_pool_t*)smcf->shared->shm.addr;
        storage = (ngx_http_stat_storage_t*)shpool->data;
    }

    ngx_http_stat_ctx_t ctx;
    ctx.phase = PHASE_REQUEST;
    ctx.storage = storage;
    ctx.pool = NULL;
    ctx.log = r->connection->log;
    ctx.smcf = smcf;

    return ctx;
}


static
char *
ngx_http_stat_parse_args(ngx_http_stat_ctx_t *ctx,
        ngx_array_t *vars, void *conf, ngx_http_stat_arg_t *args,
        ngx_uint_t args_count)
{
    ngx_uint_t             i, j, find;
    ngx_str_t             *var, value;
    ngx_uint_t             isset[args_count];
    ngx_http_stat_arg_t   *arg;

    ngx_memzero(isset, args_count * sizeof(ngx_uint_t));

    for (i = 1; i < vars->nelts; i++) {

        find = 0;
        var = &((ngx_str_t*) vars->elts)[i];

        for (j = 0; j < args_count; j++) {

             arg = &args[j];

            if (!ngx_strncmp(arg->name.data, var->data, arg->name.len) &&
                    var->data[arg->name.len] == '=')
            {
                isset[j] = 1;
                find = 1;
                value.data = var->data + arg->name.len + 1;
                value.len = var->len - (arg->name.len + 1);

                if (arg->handler(ctx, conf, &value) == NGX_CONF_ERROR) {
                    return NGX_CONF_ERROR;
                }

                break;
            }
        }

        if (!find) {
            ngx_log_error(NGX_LOG_ERR, ctx->log, 0,
                    "stat unknown option \"%V\"", var);
            return NGX_CONF_ERROR;
        }
    }

    for (i = 0; i < args_count; i++) {

        if (isset[i]) {
            continue;
        }

        arg = &args[i];
        if (arg->deflt.len == 0) {
            continue;
        }

        if (arg->handler(ctx, conf, (ngx_str_t*)&arg->deflt)
                == NGX_CONF_ERROR)
        {
            ngx_log_error(NGX_LOG_CRIT, ctx->log, 0,
                    "stat invalid option default value \"%V\"", &arg->deflt);
            return NGX_CONF_ERROR;
        }
    }

    return NGX_CONF_OK;
}


static
ngx_int_t
ngx_http_stat_init_data(ngx_http_stat_ctx_t *ctx,
        ngx_http_stat_data_t *data)
{
    ngx_http_stat_allocator_t *allocator;

    allocator = ctx->storage->allocator;

    data->metrics = ngx_http_stat_array_create(allocator, 1,
            sizeof(ngx_uint_t));
    data->statistics = ngx_http_stat_array_create(allocator, 1,
            sizeof(ngx_uint_t));

    if (data->metrics == NULL || data->statistics == NULL) {
        return NGX_ERROR;
    }

    data->filter = NULL;

    return NGX_OK;
}


static
ngx_int_t
ngx_http_stat_add_param_to_config(ngx_http_stat_ctx_t *ctx,
        ngx_http_stat_param_t *param)
{
    ngx_uint_t               p;
    ngx_http_stat_array_t   *params;
    ngx_http_stat_param_t   *old_param, *new_param;

    params = ctx->storage->params;

    for (p = 0; p < params->nelts; p++) {

        old_param = &((ngx_http_stat_param_t*) params->elts)[p];

        if (param->name.len == old_param->name.len &&
                ngx_strncmp(param->name.data, old_param->name.data,
                    param->name.len) == 0)
        {
            if (!param->percentile && !old_param->percentile) {

                if (param->aggregate == old_param->aggregate &&
                        param->interval.value == old_param->interval.value)
                {
                    return p;
                } else {
                    ngx_log_error(NGX_LOG_ERR, ctx->log, 0,
                            "stat param with different aggregate or interval");
                    return NGX_ERROR;
                }
            }
            else if (param->percentile == old_param->percentile) {
                return p;
            }
        }
    }

    new_param = ngx_http_stat_array_push(params);
    if (!new_param) {
        return NGX_ERROR;
    }

    *new_param = *param;

    return params->nelts - 1;
}


static
char *
ngx_http_stat_add_param_to_data(ngx_http_stat_ctx_t *ctx,
        ngx_uint_t split, ngx_uint_t param, ngx_http_stat_data_t *data)
{
    ngx_uint_t                   i, *m, *s;
    ngx_http_stat_storage_t     *storage;
    ngx_http_stat_param_t       *p;
    ngx_http_stat_metric_t      *metric;
    ngx_http_stat_statistic_t   *statistic;

    storage = ctx->storage;

    p = &((ngx_http_stat_param_t*) storage->params->elts)[param];

    if (p->percentile == 0) {

        for (i = 0; i < storage->metrics->nelts; i++) {

            metric = &((ngx_http_stat_metric_t*) storage->metrics->elts)[i];

            if (metric->split == split && metric->param == param) {
                break;
            }
        }

        if (i == storage->metrics->nelts) {

            metric = ngx_http_stat_array_push(storage->metrics);
            if (metric == NULL) {
                return NGX_CONF_ERROR;
            }

            metric->split = split;
            metric->param = param;
            metric->acc = NULL;

            if (ctx->phase == PHASE_REQUEST) {
                metric->acc = ngx_http_stat_allocator_alloc(
                        storage->allocator,
                        sizeof(ngx_http_stat_acc_t) * (storage->max_interval + 1));
                if (metric->acc == NULL) {
                    return NGX_CONF_ERROR;
                }

                ngx_memzero(metric->acc,
                        sizeof(ngx_http_stat_acc_t) * (storage->max_interval + 1));
            }
        }

        m = ngx_http_stat_array_push(data->metrics);
        if (m == NULL) {
            return NGX_CONF_ERROR;
        }

        *m = i;

    } else {
        for (i = 0; i < storage->statistics->nelts; i++) {

            statistic =
                &((ngx_http_stat_statistic_t*) storage->statistics->elts)[i];

            if (statistic->split == split && statistic->param == param) {
                break;
            }
        }

        if (i == storage->statistics->nelts) {

            statistic = ngx_http_stat_array_push(storage->statistics);
            if (statistic == NULL) {
                return NGX_CONF_ERROR;
            }

            statistic->split = split;
            statistic->param = param;
            statistic->stt = NULL;

            if (ctx->phase == PHASE_REQUEST) {

                statistic->stt = ngx_http_stat_allocator_alloc(
                        storage->allocator, sizeof(ngx_http_stat_stt_t));
                if (statistic->stt == NULL) {
                    return NGX_CONF_ERROR;
                }

                ngx_memzero(statistic->stt, sizeof(ngx_http_stat_stt_t));
            }
        }

        s = ngx_http_stat_array_push(data->statistics);
        if (s == NULL) {
            return NGX_CONF_ERROR;
        }

        *s = i;
    }

    return NGX_CONF_OK;
}


static
char *
ngx_http_stat_add_param_to_params(ngx_http_stat_ctx_t *ctx,
        ngx_uint_t param, ngx_array_t *params)
{
    ngx_http_stat_main_conf_t   *smcf;
    ngx_uint_t                   i, p, *new_param;
    ngx_http_stat_param_t       *old_param;

    smcf = ctx->smcf;

    for (i = 0; i < params->nelts; i++) {

        p = ((ngx_uint_t*) params->elts)[i];

        if (p != param) {
            continue;
        }

        old_param = &((ngx_http_stat_param_t*)
                smcf->storage->params->elts)[p];

        if (old_param->percentile == 0) {
            ngx_log_error(NGX_LOG_ERR, ctx->log, 0,
                    "stat duplicate param %V", &old_param->name);
        } else {
            ngx_log_error(NGX_LOG_ERR, ctx->log, 0,
                        "stat duplicate param %V/%ui",
                        &old_param->name, old_param->percentile);
            return NGX_CONF_ERROR;
        }
    }

    new_param = ngx_array_push(params);
    if (new_param == NULL) {
        return NGX_CONF_ERROR;
    }

    *new_param = param;

    return NGX_CONF_OK;
}


static
char *
ngx_http_stat_config(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    char                         *rc, host[HOST_LEN], *dot;
    ngx_uint_t                    host_size;
    ngx_http_stat_ctx_t           ctx;
    ngx_url_t                     u;
    ngx_str_t                     stat_shared_id;
    ngx_http_stat_main_conf_t    *smcf;

    ctx = ngx_http_stat_ctx_from_config(cf);

    rc = ngx_http_stat_parse_args(&ctx, cf->args, conf,
            ngx_http_stat_config_args,
            ARR_SIZE(ngx_http_stat_config_args));

    if (rc == NGX_CONF_ERROR) {
        return NGX_CONF_ERROR;
    }

    smcf = conf;

    if (smcf->host.len == 0) {

        gethostname(host, HOST_LEN);
        host[HOST_LEN - 1] = '\0';
        dot = strchr(host, '.');
        if (dot) {
            *dot = '\0';
        }

        host_size = strlen(host);

        smcf->host.data = ngx_palloc(cf->pool, host_size);
        if (smcf->host.data == NULL) {
            return NGX_CONF_ERROR;
        }

        ngx_memcpy(smcf->host.data, host, host_size);
        smcf->host.len = host_size;
    }

    if (smcf->server.name.len == 0) {
        ngx_conf_log_error(NGX_LOG_ERR, cf, 0,
                "stat config server not set");
        return NGX_CONF_ERROR;
    }

    if (smcf->port < 1 || smcf->port > 65535) {
        ngx_conf_log_error(NGX_LOG_ERR, cf, 0,
                "stat config port must be in range form 1 to 65535");
        return NGX_CONF_ERROR;
    }

    if (smcf->frequency < 1 || smcf->frequency > 65535) {
        ngx_conf_log_error(NGX_LOG_ERR, cf, 0,
                "stat config frequency must be in range form 1 to 65535");
        return NGX_CONF_ERROR;
    }

    if (smcf->intervals->nelts == 0) {
        ngx_conf_log_error(NGX_LOG_ERR, cf, 0,
                "stat config intervals not set");
        return NGX_CONF_ERROR;
    }

    if (smcf->default_params->nelts == 0) {
        ngx_conf_log_error(NGX_LOG_ERR, cf, 0,
                "stat config params not set");
        return NGX_CONF_ERROR;
    }

    if (smcf->shared_size == 0 || smcf->buffer_size == 0) {
        ngx_conf_log_error(NGX_LOG_ERR, cf, 0,
                "stat config shared must be positive value");
        return NGX_CONF_ERROR;
    }

    if (smcf->buffer_size == 0) {
        ngx_conf_log_error(NGX_LOG_ERR, cf, 0,
                "stat config buffer must be positive value");
        return NGX_CONF_ERROR;
    }

    if (smcf->package_size == 0) {
        ngx_conf_log_error(NGX_LOG_ERR, cf, 0,
                "stat config package must be positive value");
        return NGX_CONF_ERROR;
    }

    if (smcf->shared_size < sizeof(ngx_slab_pool_t)) {
        ngx_conf_log_error(NGX_LOG_ERR, cf, 0,
                "stat too small shared memory");
        return NGX_CONF_ERROR;
    }

    ngx_memzero(&u, sizeof(ngx_url_t));

    u.url = smcf->server.name;
    u.default_port = smcf->port;

    if (ngx_parse_url(cf->pool, &u) != NGX_OK) {
        ngx_conf_log_error(NGX_LOG_ERR, cf, 0,
                "\"%s\" in resolver \"%V\"", u.err, &u.url);
        return NGX_CONF_ERROR;
    }

    smcf->server.sockaddr = u.addrs[0].sockaddr;
    smcf->server.socklen = u.addrs[0].socklen;

    stat_shared_id.len = stat_shared_name.len + 32;
    stat_shared_id.data = ngx_palloc(cf->pool, stat_shared_id.len);
    if (stat_shared_id.data == NULL) {
        return NGX_CONF_ERROR;
    }

    ngx_snprintf(stat_shared_id.data, stat_shared_id.len,
            "%V.%T", &stat_shared_name, ngx_time());

    smcf->shared = ngx_shared_memory_add(cf, &stat_shared_id,
            smcf->shared_size, &ngx_http_stat_module);
    if (smcf->shared == NULL) {
        return NGX_CONF_ERROR;
    }
    if (smcf->shared->data) {
        ngx_conf_log_error(NGX_LOG_ERR, cf, 0,
                "stat shared memory is used");
        return NGX_CONF_ERROR;
    }
    smcf->shared->init = ngx_http_stat_shared_init;
    smcf->shared->data = smcf;

    smcf->buffer.start = ngx_palloc(cf->pool, smcf->buffer_size);
    if (smcf->buffer.start == NULL) {
        return NGX_CONF_ERROR;
    }
    smcf->buffer.end = smcf->buffer.start + smcf->buffer_size;

    smcf->enable = 1;

    return NGX_CONF_OK;
}


static
char *
ngx_http_stat_default_data(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_stat_ctx_t              ctx;
    ngx_http_stat_main_conf_t       *smcf;
    ngx_http_stat_srv_conf_t        *sscf;
    ngx_str_t                       *args, v, value;
    ngx_array_t                     *template, *params;
    ngx_http_complex_value_t        **filter;
    ngx_uint_t                       i;

    ctx = ngx_http_stat_ctx_from_config(cf);

    smcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_stat_module);
    sscf = ngx_http_conf_get_module_srv_conf(cf, ngx_http_stat_module);

    if (!smcf->enable) {
        ngx_conf_log_error(NGX_LOG_ERR, cf, 0, "stat config not set");
        return NGX_CONF_ERROR;
    }

    args = cf->args->elts;

    template = NULL;
    params = NULL;

    if (cf->cmd_type == NGX_HTTP_MAIN_CONF) {
        template = smcf->default_data_template;
        params = smcf->default_data_params;
        filter = &smcf->default_data_filter;
    }
    else if (cf->cmd_type == NGX_HTTP_SRV_CONF) {
        template = sscf->default_data_template;
        params = sscf->default_data_params;
        filter = &sscf->default_data_filter;
    }
    else {
        return NGX_CONF_ERROR;
    }

    if (ngx_http_stat_template_compile(&ctx, template,
                ngx_http_stat_default_data_args,
                ARR_SIZE(ngx_http_stat_default_data_args),
                &args[1]) != NGX_CONF_OK)
    {
        return NGX_CONF_ERROR;
    }

    if (cf->args->nelts >= 3) {

        for (i = 2; i < cf->args->nelts; i++) {

            v = args[i];

            if (v.len >= sizeof("params=") - 1 &&
                    ngx_strncmp(v.data, "params=", sizeof("params") - 1) == 0)
            {
                value.len = args[i].len - 7;
                value.data = args[i].data + 7;

                if (ngx_http_stat_parse_params(&ctx, &value, params)
                        != NGX_CONF_OK)
                {
                    return NGX_CONF_ERROR;
                }

            } else if (v.len >= sizeof("if") - 1 &&
                    ngx_strncmp(args[i].data, "if=", sizeof("if") - 1) == 0)
            {
                value.len = args[i].len - 3;
                value.data = args[i].data + 3;

                *filter = ngx_http_stat_complex_compile(cf, &value);
                if (!*filter) {
                    return NGX_CONF_ERROR;
                }

            } else {
                ngx_conf_log_error(NGX_LOG_ERR, cf, 0,
                        "stat unknown option \"%V\"", &v);
                return NGX_CONF_ERROR;
            }
        }
    }

    return NGX_CONF_OK;
}


static
char *
ngx_http_stat_data(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_stat_ctx_t              ctx;
    ngx_http_stat_main_conf_t       *smcf;
    ngx_http_stat_srv_conf_t        *sscf;
    ngx_http_stat_loc_conf_t        *slcf;
    ngx_str_t                       *args, *split, v, value;
    ngx_array_t                     *params, *datas;
    ngx_http_complex_value_t        *filter;
    ngx_uint_t                       i;

    ctx = ngx_http_stat_ctx_from_config(cf);

    smcf = ctx.smcf;
    sscf = ngx_http_conf_get_module_srv_conf(cf, ngx_http_stat_module);
    slcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_stat_module);

    if (!smcf->enable) {
        ngx_conf_log_error(NGX_LOG_ERR, cf, 0, "stat config not set");
        return NGX_CONF_ERROR;
    }

    args = cf->args->elts;

    split = &args[1];

    params = ngx_array_create(cf->pool, 1, sizeof(ngx_uint_t));
    if (params == NULL) {
        return NULL;
    }

    filter = NULL;

    if (cf->args->nelts >= 3) {

        for (i = 2; i < cf->args->nelts; i++) {

            v = args[i];

            if (v.len >= sizeof("params") - 1 &&
                    ngx_strncmp(v.data, "params=", sizeof("params=") - 1) == 0)
            {
                value.len = args[i].len - 7;
                value.data = args[i].data + 7;

                if (ngx_http_stat_parse_params(&ctx, &value, params)
                        != NGX_CONF_OK)
                {
                    return NGX_CONF_ERROR;
                }
            }
            else if (v.len >= sizeof("if") - 1 &&
                    ngx_strncmp(args[i].data, "if=", sizeof("if=") - 1) == 0)
            {
                value.len = args[i].len - 3;
                value.data = args[i].data + 3;

                filter = ngx_http_stat_complex_compile(cf, &value);
                if (filter == NULL) {
                    return NGX_CONF_ERROR;
                }
            }
            else {
                ngx_conf_log_error(NGX_LOG_ERR, cf, 0,
                        "stat unknown option \"%V\"", &v);
                return NGX_CONF_ERROR;
            }
        }
    }

    datas = slcf->datas;

    if (cf->cmd_type & NGX_HTTP_MAIN_CONF) {
        datas = smcf->datas;
    } else if (cf->cmd_type & NGX_HTTP_SRV_CONF) {
        datas = sscf->datas;
    }

    if (ngx_http_stat_add_data(cf, datas, split, params, filter)
            != NGX_CONF_OK)
    {
        return NGX_CONF_ERROR;
    }

    return NGX_CONF_OK;
}


static
ngx_int_t
ngx_http_stat_search_param(ngx_http_stat_array_t *internals,
        ngx_str_t *name, ngx_int_t *found)
{
    ngx_int_t                    rc;
    ngx_uint_t                   i, min_len;
    ngx_http_stat_internal_t    *internal;

    *found = 0;

    for (i = 0; i < internals->nelts; i++) {

        internal = &((ngx_http_stat_internal_t*) internals->elts)[i];

        min_len = internal->name.len < name->len ?
            internal->name.len : name->len;

        rc = ngx_strncmp(internal->name.data, name->data, min_len);

        if (rc == 0 && internal->name.len == name->len) {
            *found = 1;
            break;
        }

        if (rc > 0) {
            break;
        }
    }

    return i;
}


static
ngx_array_t *
ngx_http_stat_create_param_args(ngx_pool_t *pool, ngx_str_t *name,
        char *config)
{
    ngx_array_t     *args;
    ngx_str_t       *n, *arg;
    char            *p, *end, *begin;

    args = ngx_array_create(pool, 4, sizeof(ngx_str_t));
    if (args == NULL) {
        return NULL;
    }

    ngx_array_push(args);

    n = ngx_array_push(args);
    if (n == NULL) {
        ngx_array_destroy(args);
        return NULL;
    }

    n->data = ngx_palloc(pool, sizeof("name=") - 1 + name->len);
    if (n->data == NULL) {
        ngx_array_destroy(args);
        return NULL;
    }

    n->len = ngx_sprintf(n->data, "name=%V", name) - n->data;

    p = config;

    while (*p) {

        while (isspace(*p)) {
            p++;
        }

        begin = p;

        while (*p && !isspace(*p)) {
            p++;
        }

        end = p;

        if (end - begin != 0) {

            arg = ngx_array_push(args);
            if (arg == NULL) {
                ngx_array_destroy(args);
                return NULL;
            }

            arg->data = (u_char*)begin;
            arg->len = end - begin;
        }
        else {
            break;
        }
    }

    return args;
}


static
ngx_int_t
ngx_http_stat_parse_param_args(ngx_http_stat_ctx_t *ctx,
        ngx_array_t *args, ngx_http_stat_param_t *param)
{
    ngx_http_stat_allocator_t *allocator;

    allocator = ctx->storage->allocator;

    ngx_memzero(param, sizeof(ngx_http_stat_param_t));

    param->percentiles = ngx_http_stat_array_create(allocator, 1,
            sizeof(ngx_uint_t));
    if (param->percentiles == NULL) {
        return NGX_ERROR;
    }

    if (ngx_http_stat_parse_args(ctx, args, param,
                ngx_http_stat_param_args,
                ARR_SIZE(ngx_http_stat_param_args)) == NGX_CONF_ERROR)
    {
        return NGX_ERROR;
    }

    param->source = SOURCE_INTERNAL;

    if (param->name.data == NULL) {
        ngx_log_error(NGX_LOG_ERR, ctx->log, 0,
                "stat param name not set");
        goto error_exit;
    }

    if ((param->aggregate && !param->interval.value) ||
            (!param->aggregate && param->interval.value))
    {
        ngx_log_error(NGX_LOG_ERR, ctx->log, 0,
                "stat param must contain aggregate with interval");
        goto error_exit;
    }

    if (!param->aggregate && !param->interval.value
            && param->percentiles->nelts == 0)
    {
        ngx_log_error(NGX_LOG_ERR, ctx->log, 0,
                "stat param must contain aggregate and interval or percentile");
        goto error_exit;
    }

    if (param->interval.value > ctx->storage->max_interval) {
        goto error_exit;
    }

    return NGX_OK;

error_exit:
    ngx_http_stat_allocator_free(allocator, param->name.data);
    ngx_http_stat_allocator_free(allocator, param->interval.name.data);
    ngx_http_stat_array_destroy(param->percentiles);

    return NGX_ERROR;
}


static
ngx_int_t
ngx_http_stat_add_internal(ngx_http_stat_ctx_t *ctx,
        ngx_http_stat_param_t *param)
{

    ngx_http_stat_storage_t     *storage;
    ngx_int_t                    found;
    ngx_uint_t                   i, n, p;
    ngx_http_stat_data_t         data;
    ngx_http_stat_param_t        new_param;
    ngx_http_stat_internal_t    *internal;

    storage = ctx->storage;

    i = ngx_http_stat_search_param(storage->internals, &param->name, &found);

    if (!found) {
        if (ngx_http_stat_init_data(ctx, &data) == NGX_ERROR) {
            return NGX_ERROR;
        }
    } else {
        data = ((ngx_http_stat_internal_t*)
                storage->internals->elts)[i].data;
    }

    new_param = *param;

    for (n = 0; n < param->percentiles->nelts + 1; n++) {

        if (n == 0) {
            if (!new_param.interval.value || !new_param.aggregate) {
                continue;
            }
        } else {
            new_param.percentile = ((ngx_uint_t*)
                    new_param.percentiles->elts)[n - 1];
        }

        p = ngx_http_stat_add_param_to_config(ctx, &new_param);
        if ((ngx_int_t) p == NGX_ERROR) {
            return NGX_ERROR;
        }

        if (ngx_http_stat_add_param_to_data(ctx, SPLIT_INTERNAL, p,
                    &data) != NGX_CONF_OK)
        {
            return NGX_ERROR;
        }
    }

    if (!found) {

        internal = ngx_http_stat_array_push(storage->internals);
        if (internal == NULL) {
            return NGX_ERROR;
        }

        ngx_memmove(
            &((ngx_http_stat_internal_t*) storage->internals->elts)[i + 1],
            &((ngx_http_stat_internal_t*)storage->internals->elts)[i],
            (storage->internals->nelts - i - 1) * sizeof(ngx_http_stat_internal_t));

        internal = &((ngx_http_stat_internal_t*)storage->internals->elts)[i];
        internal->name = param->name;
        internal->data = data;
    }

    return i;
}


static
char *
ngx_http_stat_param(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_stat_ctx_t            ctx;
    ngx_http_stat_main_conf_t     *smcf;
    ngx_http_stat_param_t          param;

    ctx = ngx_http_stat_ctx_from_config(cf);
    smcf = ctx.smcf;

    if (!smcf->enable) {
        ngx_conf_log_error(NGX_LOG_ERR, cf, 0, "stat config not set");
        return NGX_CONF_ERROR;
    }

    if (ngx_http_stat_parse_param_args(&ctx, cf->args, &param) != NGX_OK) {
        return NGX_CONF_ERROR;
    }

    if (ngx_http_stat_add_internal(&ctx, &param) == NGX_ERROR) {
        return NGX_CONF_ERROR;
    }

    return NGX_CONF_OK;
}


static
char *
ngx_http_stat_config_arg_host(ngx_http_stat_ctx_t *ctx,
        void *data, ngx_str_t *value)
{
    ngx_http_stat_main_conf_t *smcf = data;
    return ngx_http_stat_parse_string(ctx, value, &smcf->host);
}

static
char *
ngx_http_stat_config_arg_protocol(ngx_http_stat_ctx_t *ctx,
        void *data, ngx_str_t *value)
{
    ngx_http_stat_main_conf_t *smcf = data;
    return ngx_http_stat_parse_string(ctx, value, &smcf->protocol);
}


static
char *
ngx_http_stat_config_arg_timeout(ngx_http_stat_ctx_t *ctx,
        void *data, ngx_str_t *value)
{
    ngx_http_stat_main_conf_t *smcf = data;
    smcf->timeout = ngx_atoi(value->data, value->len) * 1000;
    return NGX_CONF_OK;
}


static
char *
ngx_http_stat_config_arg_server(ngx_http_stat_ctx_t *ctx,
        void *data, ngx_str_t *value)
{
    ngx_http_stat_main_conf_t *smcf = data;
    return ngx_http_stat_parse_string(ctx, value, &smcf->server.name);
}


static
char *
ngx_http_stat_config_arg_port(ngx_http_stat_ctx_t *ctx,
        void *data, ngx_str_t *value)
{
    ngx_http_stat_main_conf_t *smcf = data;
    smcf->port = ngx_atoi(value->data, value->len);

    return NGX_CONF_OK;
}


static
char *
ngx_http_stat_config_arg_frequency(ngx_http_stat_ctx_t *ctx,
        void *data, ngx_str_t *value)
{
    ngx_http_stat_main_conf_t *smcf = data;
    smcf->frequency = ngx_atoi(value->data, value->len) * 1000;

    return NGX_CONF_OK;
}


static
char *
ngx_http_stat_config_arg_intervals(ngx_http_stat_ctx_t *ctx,
        void *data, ngx_str_t *value)
{
    ngx_http_stat_main_conf_t   *smcf;
    ngx_http_stat_allocator_t   *allocator;
    ngx_uint_t                   i, s;
    ngx_http_stat_interval_t    *interval;

    smcf = data;
    allocator = ctx->storage->allocator;
    s = 0;

    for (i = 0; i <= value->len; i++) {

        if (i == value->len || value->data[i] == '|') {

            if (i == s) {
                ngx_log_error(NGX_LOG_ERR, ctx->log, 0,
                        "stat config intervals is empty");
                return NGX_CONF_ERROR;
            }

            interval = ngx_array_push(smcf->intervals);
            if (interval == NULL) {
                return NGX_CONF_ERROR;
            }

            interval->name.data = ngx_http_stat_allocator_alloc(allocator, i - s);
            if (interval->name.data == NULL) {
                return NGX_CONF_ERROR;
            }

            ngx_memcpy(interval->name.data, &value->data[s], i - s);
            interval->name.len = i - s;

            if (ngx_http_stat_parse_time(ctx, &interval->name,
                        &interval->value) == NGX_CONF_ERROR)
            {
                ngx_log_error(NGX_LOG_ERR, ctx->log, 0,
                        "stat config interval is invalid");
                return NGX_CONF_ERROR;
            }

            if (interval->value > smcf->storage->max_interval) {
                smcf->storage->max_interval = interval->value;
            }

            s = i + 1;
        }
    }

    return NGX_CONF_OK;
}


static
char *
ngx_http_stat_config_arg_params(ngx_http_stat_ctx_t *ctx, void *conf,
        ngx_str_t *value)
{
    ngx_http_stat_main_conf_t *smcf = conf;
    return ngx_http_stat_parse_params(ctx, value, smcf->default_params);
}


static
char *
ngx_http_stat_config_arg_shared(ngx_http_stat_ctx_t *ctx, void *conf,
        ngx_str_t *value)
{
    ngx_http_stat_main_conf_t *smcf = conf;
    return ngx_http_stat_parse_size(ctx, value, &smcf->shared_size);
}

static
char *
ngx_http_stat_config_arg_buffer(ngx_http_stat_ctx_t *ctx, void *conf,
        ngx_str_t *value)
{
    ngx_http_stat_main_conf_t *smcf = conf;
    return ngx_http_stat_parse_size(ctx, value, &smcf->buffer_size);
}

static
char *
ngx_http_stat_config_arg_package(ngx_http_stat_ctx_t *ctx, void *conf,
        ngx_str_t *value)
{
    ngx_http_stat_main_conf_t *smcf = conf;
    return ngx_http_stat_parse_size(ctx, value, &smcf->package_size);
}

static
char *
ngx_http_stat_config_arg_template(ngx_http_stat_ctx_t *ctx, void *conf,
        ngx_str_t *value)
{
    ngx_http_stat_main_conf_t *smcf = conf;

    return ngx_http_stat_template_compile(ctx, smcf->template,
            ngx_http_stat_template_args,
            ARR_SIZE(ngx_http_stat_template_args),
            value);
}


static
char *
ngx_http_stat_param_arg_name(ngx_http_stat_ctx_t *ctx, void *data,
        ngx_str_t *value)
{
    ngx_http_stat_allocator_t     *allocator;
    ngx_http_stat_param_t         *param;

    param = (ngx_http_stat_param_t *) data;
    allocator = ctx->storage->allocator;

    param->name.data = ngx_http_stat_allocator_alloc(allocator, value->len);
    if (param->name.data == NULL) {
        return NGX_CONF_ERROR;
    }

    ngx_memcpy(param->name.data, value->data, value->len);
    param->name.len = value->len;

    return NGX_CONF_OK;
}

static
char *
ngx_http_stat_param_arg_aggregate(ngx_http_stat_ctx_t *ctx, void *data,
        ngx_str_t *value)
{
    ngx_uint_t                   found, a;
    ngx_http_stat_param_t       *param;

    param = (ngx_http_stat_param_t *) data;
    found = 0;

    for (a = 0; a < ARR_SIZE(ngx_http_stat_aggregates); a++) {

        if (ngx_http_stat_aggregates[a].name.len == value->len &&
                ngx_strncmp(ngx_http_stat_aggregates[a].name.data,
                    value->data, value->len) == 0)
        {
            found = 1;
            param->aggregate = ngx_http_stat_aggregates[a].get;
            break;
        }
    }

    if (!found) {
        ngx_log_error(NGX_LOG_ERR, ctx->log, 0,
                "stat param unknow aggregate \"%V\"", value);
        return NGX_CONF_ERROR;
    }

    return NGX_CONF_OK;
}


static
char *
ngx_http_stat_param_arg_interval(ngx_http_stat_ctx_t *ctx, void *data,
        ngx_str_t *value)
{
    ngx_http_stat_allocator_t *allocator;
    ngx_http_stat_param_t     *param;
    ngx_http_stat_interval_t  *interval;

   allocator = ctx->storage->allocator;
   param = (ngx_http_stat_param_t*)data;
   interval = &param->interval;

    interval->name.data = ngx_http_stat_allocator_alloc(allocator, value->len);
    if (interval->name.data == NULL) {
        return NGX_CONF_ERROR;
    }

    ngx_memcpy(interval->name.data, value->data, value->len);
    interval->name.len = value->len;

    if (ngx_http_stat_parse_time(ctx, value, &interval->value)
            == NGX_CONF_ERROR)
    {
        return NGX_CONF_ERROR;
    }

    return NGX_CONF_OK;
}


static
char *
ngx_http_stat_param_arg_percentile(ngx_http_stat_ctx_t *ctx, void *data,
        ngx_str_t *value)
{
    ngx_http_stat_param_t   *param;
    ngx_uint_t               i, s, *percentile;
    ngx_int_t                p;

    param = (ngx_http_stat_param_t*)data;
    s = 0;

    for (i = 0; i <= value->len; i++) {

        if (i == value->len || value->data[i] == '|') {

            if (i == s) {
                ngx_log_error(NGX_LOG_ERR, ctx->log, 0,
                        "stat param percentile is empty");
                return NGX_CONF_ERROR;
            }

            p = ngx_atoi(&value->data[s], i - s);
            if (p == NGX_ERROR || p <= 0 || p > 100) {
                ngx_log_error(NGX_LOG_ERR, ctx->log, 0,
                        "stat param percentile is invalid");
                return NGX_CONF_ERROR;
            }

            percentile = ngx_http_stat_array_push(param->percentiles);
            if (percentile == NULL) {
                return NGX_CONF_ERROR;
            }

            *percentile = p;

            s = i + 1;
        }
    }

    return NGX_CONF_OK;
}


static
ngx_uint_t
ngx_http_stat_add_split(ngx_conf_t *cf, ngx_str_t *name)
{
    ngx_http_stat_main_conf_t       *smcf;
    ngx_array_t                     *splits;
    ngx_uint_t                       s, i;
    ngx_str_t                       *split;

    smcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_stat_module);
    splits = smcf->splits;
    s = SPLIT_EMPTY;

    for (i = 0; i < splits->nelts; i++) {
        split = &((ngx_str_t*)splits->elts)[i];
        if (split->len == name->len &&
                ngx_strncmp(split->data, name->data, name->len) == 0)
        {
            s = i;
            break;
        }
    }

    if (s == SPLIT_EMPTY) {

        s = splits->nelts;
        split = ngx_array_push(splits);
        if (split == NULL) {
            return SPLIT_EMPTY;
        }

        split->data = ngx_pstrdup(cf->pool, (ngx_str_t*)name);
        if (split->data == NULL) {
            return SPLIT_EMPTY;
        }

        split->len = name->len;
    }

    return s;
}


static
char *
ngx_http_stat_add_default_data(ngx_conf_t *cf, ngx_array_t *datas,
        ngx_str_t *location, ngx_array_t *template,
        ngx_array_t *params, ngx_http_complex_value_t *filter)
{
    ngx_str_t   *variables[] = {location};
    ngx_str_t    split;

    if (template->nelts == 0) {
        return NGX_CONF_OK;
    }

    split.len = ngx_http_stat_template_len(template, variables);
    if (split.len == 0) {
        return NGX_CONF_ERROR;
    }

    split.data = ngx_palloc(cf->pool, split.len);
    if (split.data == NULL) {
        return NGX_CONF_ERROR;
    }

    ngx_http_stat_template_execute(split.data, split.len,
            template, variables);

    return ngx_http_stat_add_data(cf, datas, &split, params, filter);
}


static
char *
ngx_http_stat_add_data(ngx_conf_t *cf, ngx_array_t *datas,
        ngx_str_t *split, ngx_array_t *params,
        ngx_http_complex_value_t *filter)
{
    ngx_http_stat_ctx_t            ctx;
    ngx_http_stat_main_conf_t     *smcf;
    ngx_uint_t                     s, i, p;
    ngx_http_stat_data_t          *data;
    ngx_http_stat_param_t         *param;
    ngx_http_stat_source_t        *source;

    ctx = ngx_http_stat_ctx_from_config(cf);
    smcf = ctx.smcf;

    s = ngx_http_stat_add_split(cf, split);
    if (s == SPLIT_EMPTY) {
        return NGX_CONF_ERROR;
    }

    data = ngx_array_push(datas);
    if (data == NULL) {
        return NGX_CONF_ERROR;
    }

    if (ngx_http_stat_init_data(&ctx, data) == NGX_ERROR) {
        return NGX_CONF_ERROR;
    }

    if (params->nelts == 0) {
        params = smcf->default_params;
    }

    for (i = 0; i < params->nelts; i++) {

        p = ((ngx_uint_t*)params->elts)[i];

        param = &((ngx_http_stat_param_t*)smcf->storage->params->elts)[p];

        if (param->source != SOURCE_INTERNAL) {

            source = &((ngx_http_stat_source_t*) smcf->sources->elts)[param->source];

            if (source->type != 0) {

                if ((cf->cmd_type & NGX_HTTP_MAIN_CONF) &&
                        !(source->type & NGX_HTTP_MAIN_CONF))
                {
                    continue;
                }

                if ((cf->cmd_type & NGX_HTTP_SRV_CONF) &&
                        !(source->type & NGX_HTTP_SRV_CONF))
                {
                    continue;
                }

                if ((cf->cmd_type & NGX_HTTP_LOC_CONF) &&
                        !(source->type & NGX_HTTP_LOC_CONF))
                {
                    continue;
                }
            }
        }

        ngx_http_stat_add_param_to_data(&ctx, s, p, data);
    }

    data->filter = (ngx_http_complex_value_t*) filter;

    return NGX_CONF_OK;
}


static
ngx_uint_t
ngx_http_stat_get_source(ngx_http_stat_ctx_t *ctx,  ngx_str_t *name)
{
    ngx_http_stat_main_conf_t     *smcf;
    ngx_http_stat_source_t        *source;
    ngx_uint_t                     c;
    ngx_regex_compile_t            rc;
    ngx_http_stat_source_t        *new_source;

    if (ctx->phase != PHASE_CONFIG) {
        return NGX_ERROR;
    }

    smcf = ctx->smcf;

    for (c = 0; c < smcf->sources->nelts; c++) {

        source = &((ngx_http_stat_source_t*) smcf->sources->elts)[c];

        if (source->name.len == name->len &&
                ngx_strncmp(source->name.data, name->data, name->len) == 0)
        {
            return c;
        }
    }

    for (c = 0; c < ARR_SIZE(ngx_http_stat_sources); c++) {

        source = &ngx_http_stat_sources[c];

        if (!source->re) {
            if (source->name.len == name->len &&
                    ngx_strncmp(source->name.data, name->data, name->len) == 0)
            {
                break;
            }
        } else {

            ngx_memzero(&rc, sizeof(ngx_regex_compile_t));
            rc.pool = ctx->pool;
            rc.pattern = source->name;

            if (ngx_regex_compile(&rc) != NGX_OK) {
                ngx_log_error(NGX_LOG_ERR, ctx->log, 0,
                        "stat can't compile regex");
                return NGX_ERROR;
            }

            if (ngx_regex_exec(rc.regex, name, NULL, 0) >= 0) {
                break;
            }
        }
    }

    if (c == ARR_SIZE(ngx_http_stat_sources)) {
        ngx_log_error(NGX_LOG_ERR, ctx->log, 0,
                "stat unknow param \"%V\"", name);
        return NGX_ERROR;
    }

    new_source = ngx_array_push(smcf->sources);
    if (new_source == NULL) {
        return NGX_ERROR;
    }
    *new_source = ngx_http_stat_sources[c];
    new_source->name = *name;

    return smcf->sources->nelts - 1;
}


static
ngx_http_stat_param_t
ngx_http_stat_create_param(ngx_http_stat_ctx_t *ctx, ngx_uint_t c)
{
    ngx_http_stat_main_conf_t *smcf;
    ngx_http_stat_source_t    *source;
    ngx_http_stat_param_t      param;

    smcf = ctx->smcf;
    source = &((ngx_http_stat_source_t*) smcf->sources->elts)[c];

    ngx_memzero(&param, sizeof(ngx_http_stat_param_t));

    param.name = source->name;
    param.source = c;
    param.aggregate = source->aggregate;

    return param;
}

static
char *
ngx_http_stat_parse_params(ngx_http_stat_ctx_t *ctx, ngx_str_t *value,
        ngx_array_t *params)
{
    ngx_http_stat_main_conf_t   *smcf;
    ngx_uint_t                   i, s, q, p, c;
    ngx_http_stat_param_t        param;
    ngx_str_t                    default_params, name;
    ngx_int_t                    percentile;

    smcf = ctx->smcf;

    s = 0;
    q = NGX_CONF_UNSET_UINT;

    for (i = 0; i <= value->len; i++) {

        if (i == value->len || value->data[i] == '|') {

            if (i == s) {
                ngx_log_error(NGX_LOG_ERR, ctx->log, 0,
                        "stat params is empty");
                return NGX_CONF_ERROR;
            }

            if (i - s == 1 && value->data[s] == '*') {

                if (params == smcf->default_params) {

                    ngx_str_set(&default_params, DEFAULT_PARAMS);

                    if (ngx_http_stat_parse_params(ctx,
                                &default_params, params) == NGX_CONF_ERROR)
                    {
                        return NGX_CONF_ERROR;
                    }

                } else {

                    for (p = 0; p < smcf->default_params->nelts; p++) {

                        c = ((ngx_uint_t*)smcf->default_params->elts)[p];
                        if (ngx_http_stat_add_param_to_params(ctx, c,
                                    params) != NGX_CONF_OK)
                        {
                            return NGX_CONF_ERROR;
                        }
                    }
                }

            } else {

                if (q == NGX_CONF_UNSET_UINT) {
                    q = i;
                }

                name.data = &value->data[s];
                name.len = q - s;

                c = ngx_http_stat_get_source(ctx, &name);
                if ((ngx_int_t) c == NGX_ERROR) {
                    return NGX_CONF_ERROR;
                }

                param = ngx_http_stat_create_param(ctx, c);

                if (q != i) {

                    percentile = ngx_atoi(&value->data[q + 1], i - q - 1);
                    if (percentile == NGX_ERROR ||
                            percentile <= 0 ||
                            percentile > 100)
                    {
                        ngx_log_error(NGX_LOG_ERR, ctx->log, 0,
                                "stat bad param \"%*s\"", i - s,
                                &value->data[s]);
                        return NGX_CONF_ERROR;
                    }

                    param.percentile = percentile;
                }

                p = ngx_http_stat_add_param_to_config(ctx, &param);
                if ((ngx_int_t) p == NGX_ERROR) {
                    return NGX_CONF_ERROR;
                }

                if (ngx_http_stat_add_param_to_params(ctx, p, params)
                        != NGX_CONF_OK)
                {
                    return NGX_CONF_ERROR;
                }
            }

            s = i + 1;
            q = NGX_CONF_UNSET_UINT;
        }

        if (value->data[i] == '/') {
            q = i;
        }
    }

    return NGX_CONF_OK;
}


static
char *
ngx_http_stat_parse_string(ngx_http_stat_ctx_t *ctx, ngx_str_t *value,
        ngx_str_t *result)
{
    ngx_http_stat_allocator_t *allocator;

    allocator = ctx->storage->allocator;

    result->data = ngx_http_stat_allocator_alloc(allocator, value->len);
    if (result->data == NULL) {
        return NGX_CONF_ERROR;
    }

    ngx_memcpy(result->data, value->data, value->len);
    result->len = value->len;

    return NGX_CONF_OK;
}


static
char *
ngx_http_stat_parse_size(ngx_http_stat_ctx_t *ctx, ngx_str_t *value,
        ngx_uint_t *result)
{
    ngx_uint_t len;

    len = 0;

    while (len < value->len && value->data[len] >= '0' &&
            value->data[len] <= '9')
    {
        len++;
    }

    *result = (ngx_uint_t) ngx_atoi(value->data, len);
    if (*result == (ngx_uint_t) NGX_ERROR) {
        return NGX_CONF_ERROR;
    }

    if (len + 1 == value->len) {

        switch (value->data[len]) {
        case 'b':
            *result *= 1;
            break;
        case 'k':
            *result *= 1024;
            break;
        case 'm':
            *result *= 1024 * 1024;
            break;
        default:
            return NGX_CONF_ERROR;
        }

        len++;
    }

    if (len != value->len) {
        return NGX_CONF_ERROR;
    }

    return NGX_CONF_OK;
}


static
char *
ngx_http_stat_parse_time(ngx_http_stat_ctx_t *ctx, ngx_str_t *value,
        ngx_uint_t *result)
{
    ngx_uint_t len;

    len = 0;
    while (len < value->len && value->data[len] >= '0' &&
            value->data[len] <= '9')
    {
        len++;
    }

    *result = (ngx_uint_t) ngx_atoi(value->data, len);
    if (*result == (ngx_uint_t) NGX_ERROR) {
        return NGX_CONF_ERROR;
    }

    if (len + 1 == value->len) {

        switch (value->data[len]) {
        case 's':
            *result *= 1;
            break;
        case 'm':
            *result *= 60;
            break;
        default:
            return NGX_CONF_ERROR;
        }

        len++;
    }

    if (len != value->len) {
        return NGX_CONF_ERROR;
    }

    return NGX_CONF_OK;
}

/** Variables, Stats and metric API {{{ */
char *
ngx_http_stat_template_compile(ngx_http_stat_ctx_t *ctx,
        ngx_array_t *template, ngx_http_stat_template_arg_t *args,
        ngx_uint_t nargs, ngx_str_t *value)
{
    typedef enum {
        TEMPLATE_STATE_ERROR = -1,
        TEMPLATE_STATE_NONE,
        TEMPLATE_STATE_VAR,
        TEMPLATE_STATE_BRACKET_VAR,
        TEMPLATE_STATE_VAR_START,
        TEMPLATE_STATE_BRACKET_VAR_START,
        TEMPLATE_STATE_BRACKET_VAR_END,
        TEMPLATE_STATE_NOP,
    } ngx_http_stat_template_parser_state_t;

    typedef enum {
        TEMPLATE_LEXEM_ERROR = -1,
        TEMPLATE_LEXEM_VAR,
        TEMPLATE_LEXEM_BRACKET_OPEN,
        TEMPLATE_LEXEM_BRACKET_CLOSE,
        TEMPLATE_LEXEM_ALNUM,
        TEMPLATE_LEXEM_OTHER,
        TEMPLATE_LEXEM_END,
    } ngx_http_stat_template_parser_lexem_t;

    ngx_http_stat_template_parser_state_t parser[
        TEMPLATE_STATE_BRACKET_VAR_END + 1][TEMPLATE_LEXEM_END + 1] = {
        { 1,  6,  6,  6,  6,  0},
        { 0,  2, -1,  3, -1, -1},
        {-1, -1, -1,  4, -1, -1},
        { 0,  0,  0,  6,  0,  0},
        {-1, -1,  5,  6, -1, -1},
        { 0,  0,  0,  0,  0,  0},
    };

    ngx_http_stat_allocator_t                 *allocator;
    ngx_uint_t                                 i, s, find, a;
    ngx_int_t                                  new_state;
    ngx_http_stat_template_parser_state_t      state;
    ngx_http_stat_template_parser_lexem_t      lexem;
    ngx_http_stat_template_t                  *arg;

    allocator = ctx->storage->allocator;
    s = 0;
    state = TEMPLATE_STATE_NONE;

    for (i = 0; i <= value->len; i++) {

        if (i == value->len) {
            lexem = TEMPLATE_LEXEM_END;
        } else if (value->data[i] == '$') {
            lexem = TEMPLATE_LEXEM_VAR;
        } else if (value->data[i] == '(') {
            lexem = TEMPLATE_LEXEM_BRACKET_OPEN;
        } else if (value->data[i] == ')') {
            lexem = TEMPLATE_LEXEM_BRACKET_CLOSE;
        } else if (isalnum(value->data[i])) {
            lexem = TEMPLATE_LEXEM_ALNUM;
        } else {
            lexem = TEMPLATE_LEXEM_OTHER;
        }

        new_state = parser[state][lexem];

        if (new_state == TEMPLATE_LEXEM_ERROR) {
            return NGX_CONF_ERROR;
        }

        if (new_state != TEMPLATE_STATE_NOP) {

            if (i != s && (state == TEMPLATE_STATE_NONE ||
                        state == TEMPLATE_STATE_VAR_START ||
                        state == TEMPLATE_STATE_BRACKET_VAR_START))
            {
                arg = ngx_array_push(template);
                if (arg == NULL) {
                    return NGX_CONF_ERROR;
                }

                if (state == TEMPLATE_STATE_NONE) {

                    arg->data.data = ngx_http_stat_allocator_alloc(
                            allocator, i - s);
                    if (arg->data.data == NULL) {
                        return NGX_CONF_ERROR;
                    }

                    ngx_memcpy(arg->data.data, value->data + s, i - s);
                    arg->data.len = i - s;
                    arg->variable = 0;

                } else if (state == TEMPLATE_STATE_VAR_START ||
                        state == TEMPLATE_STATE_BRACKET_VAR_START)
                {
                    find = 0;

                    for (a = 0; a < nargs; a++) {

                        if (args[a].name.len == (i - s) &&
                                ngx_strncmp(args[a].name.data,
                                    &value->data[s], i - s) == 0)
                        {
                            find = 1;
                            arg->variable = args[a].variable;
                            arg->data.data = NULL;
                            arg->data.len = 0;
                            break;
                        }
                    }

                    if (!find) {
                        ngx_log_error(NGX_LOG_ERR, ctx->log, 0,
                                "stat unknow template arg \"%*s\"",
                                i - s, &value->data[s]);
                        return NGX_CONF_ERROR;
                    }
                }
            }

            s = i;
            state = new_state;
        }
    }

    return NGX_CONF_OK;
}


u_char *
ngx_http_stat_template_execute(u_char* buffer, ngx_uint_t buffer_size,
        ngx_array_t *template, ngx_str_t *variables[])
{
    u_char                      *b;
    ngx_uint_t                   i;
    ngx_http_stat_template_t    *arg;
    ngx_str_t                   *data;

    b = buffer;

    for (i = 0; i < template->nelts; i++) {

        arg = &((ngx_http_stat_template_t*) template->elts)[i];

        if (arg->data.len) {
            data = &arg->data;
        } else {
            data = variables[arg->variable];
        }

        b = ngx_snprintf(b, buffer_size - (b - buffer), "%V", data);
    }

    return b;
}


ngx_uint_t
ngx_http_stat_template_len(ngx_array_t *template, ngx_str_t *variables[])
{
    ngx_uint_t                   len, i;
    ngx_http_stat_template_t    *arg;

    len = 0;

    for (i = 0; i < template->nelts; i++) {

        arg = &((ngx_http_stat_template_t*) template->elts)[i];

        if (arg->data.len) {
            len += arg->data.len;
        } else {
            len += variables[arg->variable]->len;
        }
    }

    return len;
}
/** }}} */

static
ngx_http_complex_value_t *
ngx_http_stat_complex_compile(ngx_conf_t *cf, ngx_str_t *value)
{
    ngx_http_compile_complex_value_t ccv;

    ngx_memzero(&ccv, sizeof(ngx_http_compile_complex_value_t));

    ccv.cf = cf;
    ccv.value = value;
    ccv.complex_value = ngx_palloc(cf->pool, sizeof(ngx_http_complex_value_t));
    if (ccv.complex_value == NULL) {
        return NULL;
    }

    if (ngx_http_compile_complex_value(&ccv) != NGX_OK) {
        return NULL;
    }

    return ccv.complex_value;
}


static
ngx_int_t
ngx_http_stat_shared_init(ngx_shm_zone_t *shm_zone, void *data)
{
    ngx_http_stat_main_conf_t *smcf = shm_zone->data;

    ngx_uint_t                   shared_required_size, buffer_required_size, m,
                                 s;
    ngx_slab_pool_t             *shpool;
    ngx_http_stat_storage_t     *storage;
    ngx_http_stat_allocator_t   *allocator;
    u_char                      *accs, *stts;
    ngx_http_stat_metric_t      *metric;
    ngx_http_stat_statistic_t   *statistic;
    ngx_http_stat_param_t       *param;

    if (data) {
        ngx_log_error(NGX_LOG_ERR, shm_zone->shm.log, 0,
                "stat shared memory data set");
        return NGX_ERROR;
    }

    shared_required_size =
        2 *
        (sizeof(ngx_slab_pool_t) +
        sizeof(ngx_http_stat_storage_t) +
        sizeof(ngx_array_t) * 4 +
        sizeof(ngx_http_stat_metric_t) * (smcf->storage->metrics->nelts) +
        sizeof(ngx_http_stat_statistic_t) * (smcf->storage->statistics->nelts) +
        sizeof(ngx_http_stat_param_t) * (smcf->storage->params->nelts) +
        sizeof(ngx_http_stat_internal_t) * (smcf->storage->internals->nelts) +
        sizeof(ngx_http_stat_acc_t) * (smcf->storage->max_interval + 1) *
        smcf->storage->metrics->nelts +
        sizeof(ngx_http_stat_stt_t) * smcf->storage->statistics->nelts);

    if (shared_required_size > shm_zone->shm.size) {
        ngx_log_error(NGX_LOG_ERR, shm_zone->shm.log, 0,
                "stat too small shared memory (minimum size is %uzb)",
                shared_required_size);
        return NGX_ERROR;
    }

    /*
     * 128 is the approximate size of the one record
     */
    buffer_required_size = (smcf->intervals->nelts *
            smcf->storage->metrics->nelts + smcf->storage->statistics->nelts) *
            128;
    if (buffer_required_size > smcf->buffer_size) {
        ngx_log_error(NGX_LOG_ERR, shm_zone->shm.log, 0,
                "stat too small buffer size (minimum size is %uzb)",
                buffer_required_size);
        return NGX_ERROR;
    }

    shpool = (ngx_slab_pool_t *) shm_zone->shm.addr;

    if (shm_zone->shm.exists) {
        ngx_log_error(NGX_LOG_ERR, shm_zone->shm.log, 0,
                "stat shared memory exists");
        return NGX_ERROR;
    }

    storage = ngx_slab_alloc(shpool, sizeof(ngx_http_stat_storage_t));
    if (storage == NULL) {
        return NGX_ERROR;
    }

    shpool->data = storage;

    ngx_memzero(storage, sizeof(ngx_http_stat_storage_t));

    storage->max_interval = smcf->storage->max_interval;

    storage->start_time = ngx_time();
    storage->last_time = storage->start_time;
    storage->event_time = storage->start_time;

    allocator = ngx_slab_alloc(shpool, sizeof(ngx_http_stat_allocator_t));
    if (allocator == NULL) {
        return NGX_ERROR;
    }

    ngx_http_stat_allocator_init(allocator,
            shpool, ngx_http_stat_allocator_slab_alloc,
            ngx_http_stat_allocator_slab_free);

    storage->allocator = allocator;
    storage->metrics = ngx_http_stat_array_copy(storage->allocator,
            smcf->storage->metrics);
    storage->statistics = ngx_http_stat_array_copy(storage->allocator,
            smcf->storage->statistics);
    storage->params = ngx_http_stat_array_copy(storage->allocator,
            smcf->storage->params);
    storage->internals = ngx_http_stat_array_copy(storage->allocator,
            smcf->storage->internals);

    if (!storage->metrics || !storage->statistics || !storage->params ||
            !storage->internals)
    {
        return NGX_ERROR;
    }

    accs = ngx_slab_calloc(shpool, sizeof(ngx_http_stat_acc_t) *
            (smcf->storage->max_interval + 1) * smcf->storage->metrics->nelts);
    if (accs == NULL) {
        return NGX_ERROR;
    }

    stts = ngx_slab_calloc(shpool,
            sizeof(ngx_http_stat_stt_t) * smcf->storage->statistics->nelts);
    if (stts == NULL) {
        return NGX_ERROR;
    }

    for (m = 0; m < storage->metrics->nelts; m++) {

        metric = &((ngx_http_stat_metric_t *) storage->metrics->elts)[m];
        metric->acc = (ngx_http_stat_acc_t *)(accs +
            sizeof(ngx_http_stat_acc_t) * (smcf->storage->max_interval + 1) *
            m);
    }

    for (s = 0; s < storage->statistics->nelts; s++) {

        statistic = &(((ngx_http_stat_statistic_t*)
                    storage->statistics->elts)[s]);

        param = &((ngx_http_stat_param_t*)
                smcf->storage->params)[statistic->param];

        statistic->stt = (ngx_http_stat_stt_t*)(
                stts + sizeof(ngx_http_stat_stt_t) * s);

        ngx_http_stat_statistic_init(statistic->stt, param->percentile);
    }

    return NGX_OK;
}


static
double *
ngx_http_stat_get_sources_values(ngx_http_stat_main_conf_t *smcf,
        ngx_http_request_t *r)
{
    double                   *values;
    ngx_uint_t                c;
    ngx_http_stat_source_t   *source;

    values = ngx_palloc(r->pool, sizeof(double) * smcf->sources->nelts);
    if (values == NULL) {
        return NULL;
    }

    for (c = 0; c < smcf->sources->nelts; c++) {

        source = &((ngx_http_stat_source_t*)smcf->sources->elts)[c];
        if (source->get) {
            values[c] = source->get(source, r);
        }
    }

    return values;
}


static
void
ngx_http_stat_add_metric(ngx_http_request_t *r,
        ngx_http_stat_storage_t *storage,
        ngx_http_stat_metric_t *metric, time_t ts, double value)
{
    ngx_uint_t a = ((ts - storage->start_time) % (storage->max_interval + 1));
    ngx_http_stat_acc_t *acc = &metric->acc[a];

    acc->count++;
    acc->value += value;
}


static
void
ngx_http_stat_add_statistic(ngx_http_request_t *r,
        ngx_http_stat_storage_t *storage,
        ngx_http_stat_statistic_t *statistic, time_t ts, double value,
        ngx_uint_t percentile)
{
    ngx_http_stat_stt_t     *stt;
    ngx_uint_t               k, i;
    double                   d, a;
    ngx_int_t                s;

    stt = statistic->stt;

    if (stt->count >= P2_METRIC_COUNT) {

        for (k = 0; k < P2_METRIC_COUNT; k++) {
            if (value < stt->q[k])
                break;
        }

        if (k == 0) {
            k = 1;
            stt->q[0] = value;
        } else if (k == P2_METRIC_COUNT) {
            k = 4;
            stt->q[P2_METRIC_COUNT - 1] = value;
        }

        for (i = 0; i < P2_METRIC_COUNT; i++) {

            if (i >= k) {
                stt->n[i]++;
            }
            stt->np[i] += stt->dn[i];
        }

        for (i = 1; i < P2_METRIC_COUNT - 1; i++) {

            d = stt->np[i] - stt->n[i];
            s = (d >= 0.0) ? 1 : -1;

            if ((d >= 1.0 && stt->n[i + 1] - stt->n[i] > 1) ||
                    (d <= -1.0 && stt->n[i - 1] - stt->n[i] < -1))
            {
                a = stt->q[i] + (double) s * ((stt->n[i] - stt->n[i - 1] + s) *
                        (stt->q[i + 1] - stt->q[i]) / (stt->n[i + 1] - stt->n[i]) +
                        (stt->n[i + 1] - stt->n[i] - s) * (stt->q[i] - stt->q[i - 1]) /
                        (stt->n[i] - stt->n[i - 1])) / (stt->n[i + 1] - stt->n[i - 1]);

                if (a <= stt->q[i - 1] || stt->q[i + 1] <= a) {
                    a = stt->q[i] + (double) s * (stt->q[i + s] - stt->q[i]) /
                        (stt->n[i + s] - stt->n[i]);
                }

                stt->q[i] = a;
                stt->n[i] += s;
            }
        }

    } else {

        for (i = 0; i < stt->count; i++) {
            if (value < stt->q[i]) {
                break;
            }
        }

        if (stt->count != 0 && i < P2_METRIC_COUNT - 1) {
            memmove(&stt->q[i + 1], &stt->q[i],
                    (stt->count - i) * sizeof(*stt->q));
        }

        stt->q[i] = value;
        stt->count++;
    }
}


static void
ngx_http_stat_add_data_values(ngx_http_request_t *r,
        ngx_http_stat_storage_t *storage, time_t ts,
        ngx_http_stat_data_t *data,  double *values)
{
    ngx_uint_t                   i, m, s;
    ngx_str_t                    result;
    ngx_http_stat_metric_t      *metric;
    ngx_http_stat_param_t       *param;
    ngx_http_stat_statistic_t   *statistic;
    double                       value;

    if (data->filter) {

        if (ngx_http_complex_value(r, data->filter, &result) != NGX_OK) {
            return;
        }

        if (result.len == 0 || (result.len == 1 && result.data[0] == '0')) {
            return;
        }
    }

    for (i = 0; i < data->metrics->nelts; i++) {

        m = ((ngx_uint_t*) data->metrics->elts)[i];
        metric = &((ngx_http_stat_metric_t*)storage->metrics->elts)[m];
        param = &((ngx_http_stat_param_t*)storage->params->elts)[metric->param];
        value = (param->source != SOURCE_INTERNAL) ? values[param->source] :
            values[0];
        ngx_http_stat_add_metric(r, storage, metric, ts, value);
    }

    for (i = 0; i < data->statistics->nelts; i++) {

        s = ((ngx_uint_t*)data->statistics->elts)[i];
        statistic = &((ngx_http_stat_statistic_t*)storage->statistics->elts)[s];
        param = &((ngx_http_stat_param_t*)storage->params->elts)[statistic->param];
        value = (param->source != SOURCE_INTERNAL) ? values[param->source] :
            values[0];
        ngx_http_stat_add_statistic(r, storage, statistic, ts, value, param->percentile);
    }
}


static
void
ngx_http_stat_add_datas_values(ngx_http_request_t *r,
        ngx_http_stat_storage_t *storage, time_t ts,
        ngx_array_t *datas,  double *values)
{
    ngx_uint_t              i;
    ngx_http_stat_data_t *data;

    for (i = 0; i < datas->nelts; i++) {
        data = &((ngx_http_stat_data_t*)datas->elts)[i];
        ngx_http_stat_add_data_values(r, storage, ts, data, values);
    }
}


static
ngx_int_t
ngx_http_stat_handler(ngx_http_request_t *r)
{

    ngx_http_stat_main_conf_t     *smcf;
    ngx_http_stat_srv_conf_t      *sscf;
    ngx_http_stat_loc_conf_t      *slcf;
    ngx_slab_pool_t               *shpool;
    ngx_http_stat_storage_t       *storage;
    double                        *values;
    time_t                         ts;

    smcf = ngx_http_get_module_main_conf(r, ngx_http_stat_module);
    sscf = ngx_http_get_module_srv_conf(r, ngx_http_stat_module);
    slcf = ngx_http_get_module_loc_conf(r, ngx_http_stat_module);

    if (!smcf->enable) {
        return NGX_OK;
    }

    if (smcf->datas->nelts == 0 &&
            sscf->datas->nelts == 0 &&
            slcf->datas->nelts == 0)
    {
        return NGX_OK;
    }

    shpool = (ngx_slab_pool_t*) smcf->shared->shm.addr;
    storage = (ngx_http_stat_storage_t*) shpool->data;

    ts = ngx_time();

    values = ngx_http_stat_get_sources_values(smcf, r);
    if (values == NULL) {
        return NGX_OK;
    }

    ngx_shmtx_lock(&shpool->mutex);
    ngx_http_stat_gc(smcf, ts);

    if (r == r->main) {
        ngx_http_stat_add_datas_values(r, storage, ts, smcf->datas, values);
        ngx_http_stat_add_datas_values(r, storage, ts, sscf->datas, values);
    }

    ngx_http_stat_add_datas_values(r, storage, ts, slcf->datas, values);

    ngx_shmtx_unlock(&shpool->mutex);

    return NGX_OK;
}


ngx_int_t
ngx_http_stat(ngx_http_request_t *r, ngx_str_t *name, double value,
        char *config)
{
    ngx_http_stat_ctx_t            ctx;
    ngx_http_stat_main_conf_t     *smcf;
    ngx_slab_pool_t               *shpool;
    ngx_http_stat_storage_t       *storage;
    time_t                         ts;
    ngx_int_t                      found, i;
    ngx_array_t                   *args;
    ngx_http_stat_param_t          param;
    ngx_http_stat_internal_t      *internal;

    ctx = ngx_http_stat_ctx_from_request(r);
    smcf = ctx.smcf;

    if (!smcf->enable) {
        return NGX_OK;
    }

    shpool = (ngx_slab_pool_t*) smcf->shared->shm.addr;
    storage = (ngx_http_stat_storage_t*) shpool->data;

    ts = ngx_time();

    ngx_shmtx_lock(&shpool->mutex);

    ngx_http_stat_gc(smcf, ts);

    found = 0;
    i = ngx_http_stat_search_param(storage->internals, name, &found);

    if (!found) {

        if (!config || storage->allocator->nomemory) {
            ngx_shmtx_unlock(&shpool->mutex);
            return NGX_OK;
        }

        args = ngx_http_stat_create_param_args(r->pool, name, config);
        if (args == NULL) {
            ngx_shmtx_unlock(&shpool->mutex);
            return NGX_ERROR;
        }

        if (ngx_http_stat_parse_param_args(&ctx, args, &param) != NGX_OK) {
            ngx_shmtx_unlock(&shpool->mutex);
            return NGX_ERROR;
        }

        i = ngx_http_stat_add_internal(&ctx, &param);

        if (i == NGX_ERROR) {
            ngx_shmtx_unlock(&shpool->mutex);
            return NGX_ERROR;
        }
    }

    internal = &((ngx_http_stat_internal_t*) storage->internals->elts)[i];
    ngx_http_stat_add_data_values(r, storage, ts, &internal->data, &value);

    ngx_shmtx_unlock(&shpool->mutex);

    return NGX_OK;
}


void
ngx_http_stat_gc(ngx_http_stat_main_conf_t *smcf, time_t ts)
{
    ngx_slab_pool_t             *shpool;
    ngx_http_stat_storage_t     *storage;
    ngx_uint_t                   m, a;
    ngx_http_stat_metric_t      *metric;
    ngx_http_stat_acc_t         *acc;

    shpool = (ngx_slab_pool_t *) smcf->shared->shm.addr;
    storage = (ngx_http_stat_storage_t *) shpool->data;

    while ((ngx_uint_t) storage->last_time + storage->max_interval <
            (ngx_uint_t)ts)
    {
        for (m = 0; m < storage->metrics->nelts; m++) {

            metric = &(((ngx_http_stat_metric_t*)
                        storage->metrics->elts)[m]);

            a = ((storage->last_time - storage->start_time) % (
                        storage->max_interval + 1));

            acc = &metric->acc[a];

            acc->value = 0;
            acc->count = 0;
        }

        storage->last_time++;
    }
}


static
double
ngx_http_stat_source_request_time( ngx_http_stat_source_t *source,
        ngx_http_request_t *r)
{
    ngx_msec_int_t   ms;
    struct timeval   tp;

    ngx_gettimeofday(&tp);

    ms = (ngx_msec_int_t) ((tp.tv_sec - r->start_sec) * 1000 +
            (tp.tv_usec / 1000 - r->start_msec));

    return (double) ms;
}


static
double
ngx_http_stat_source_bytes_sent( ngx_http_stat_source_t *source,
        ngx_http_request_t *r)
{
    return (double) r->connection->sent;
}


static
double
ngx_http_stat_source_body_bytes_sent( ngx_http_stat_source_t *source,
        ngx_http_request_t *r)
{
    off_t  length;

    length = r->connection->sent - r->header_size;
    length = ngx_max(length, 0);

    return (double) length;
}

static
double
ngx_http_stat_source_request_length( ngx_http_stat_source_t *source,
        ngx_http_request_t *r)
{
    return (double) r->request_length;
}


static
double
ngx_http_stat_source_rps(ngx_http_stat_source_t *source,
        ngx_http_request_t *r)
{
    return 1;
}


static
double
ngx_http_stat_source_keepalive_rps(ngx_http_stat_source_t *source,
        ngx_http_request_t *r)
{
    if (r->connection->requests == 1) {
        return 0;
    }
    return 1;
}


static
double
ngx_http_stat_source_response_2xx_rps( ngx_http_stat_source_t *source,
        ngx_http_request_t *r)
{
    if (r->headers_out.status / 100 == 2) {
        return 1;
    }
    return 0;
}


static
double
ngx_http_stat_source_response_3xx_rps( ngx_http_stat_source_t *source,
        ngx_http_request_t *r)
{
    if (r->headers_out.status / 100 == 3) {
        return 1;
    }
    return 0;
}


static
double
ngx_http_stat_source_response_4xx_rps(ngx_http_stat_source_t *source,
        ngx_http_request_t *r)
{
    if (r->headers_out.status / 100 == 4) {
        return 1;
    }
    return 0;
}


static
double
ngx_http_stat_source_response_5xx_rps( ngx_http_stat_source_t *source,
        ngx_http_request_t *r)
{
    if (r->headers_out.status / 100 == 5) {
        return 1;
    }
    return 0;
}


static
double
ngx_http_stat_source_response_xxx_rps(ngx_http_stat_source_t *source,
        ngx_http_request_t *r)
{
    ngx_uint_t status;

    status = ngx_atoi(source->name.data + sizeof("response_") - 1, 3);
    if (r->headers_out.status == status) {
        return 1;
    }
    return 0;
}


static
double
ngx_http_stat_source_upstream_cache_status_rps(
        ngx_http_stat_source_t *source, ngx_http_request_t *r)
{
#if NGX_HTTP_CACHE
    ngx_str_t param_status, cache_status;

    if (r->upstream == NULL || r->upstream->cache_status == 0) {
        return 0;
    }

    param_status.data = source->name.data + sizeof("upstream_cache_") - 1;
    param_status.len = source->name.len - (sizeof("upstream_cache_") - 1) - 4;

    cache_status = ngx_http_cache_status[r->upstream->cache_status - 1];
    if (cache_status.len == param_status.len &&
            ngx_strncasecmp(cache_status.data, param_status.data,
                param_status.len) == 0)
    {
        return 1;
    }
#endif /** NGX_HTTP_CACHE */
    return 0;
}

/** Acc && Inver funcs API {{{ */
double
ngx_http_stat_aggregate_avg( ngx_http_stat_interval_t *interval,
         ngx_http_stat_acc_t *acc)
{
    return (acc->count != 0) ? acc->value / acc->count : 0;
}


double
ngx_http_stat_aggregate_persec( ngx_http_stat_interval_t *interval,
         ngx_http_stat_acc_t *acc)
{
    return acc->value / interval->value;
}


double
ngx_http_stat_aggregate_sum( ngx_http_stat_interval_t *interval,
         ngx_http_stat_acc_t *acc)
{
    return acc->value;
}


void
ngx_http_stat_statistic_init(ngx_http_stat_stt_t *stt, ngx_uint_t percentile)
{
    double      p;
    ngx_uint_t  i;

    ngx_memzero(stt, sizeof(ngx_http_stat_stt_t));

    p = (double) percentile / 100;

    stt->dn[P2_METRIC_COUNT - 1] = 1;
    stt->dn[P2_METRIC_COUNT / 2] = p;

    for (i = 1; i < P2_METRIC_COUNT / 2; i++) {
        stt->dn[i] = (p / (P2_METRIC_COUNT / 2)) * i;
    }

    for (i = P2_METRIC_COUNT / 2 + 1; i < P2_METRIC_COUNT - 1; i++) {
        stt->dn[i] = p + ((1 - p) / (P2_METRIC_COUNT - 1 - P2_METRIC_COUNT / 2))
            * (i - P2_METRIC_COUNT / 2);
    }

    for (i = 0; i < P2_METRIC_COUNT; i++) {
        stt->np[i] = (P2_METRIC_COUNT - 1) * stt->dn[i] + 1;
        stt->n[i] = i + 1;
    }
}

/** }}} */

/** }}} */

