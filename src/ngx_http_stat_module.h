
/**
 * (c) BSD-2-Cause, link: https://opensource.org/licenses/BSD-2-Clause
 * (c) V. Soshnikov, mailto: dedok.mad@gmail.com
 */

#ifndef NGX_HTTP_STAT_MODULE_H_INCLUDED
#define NGX_HTTP_STAT_MODULE_H_INCLUDED 1

#include "ngx_http_stat_array.h"
#include "ngx_http_stat_allocator.h"


#define HOST_LEN 256

#define PHASE_CONFIG 0
#define PHASE_REQUEST 1

#define SPLIT_EMPTY ((ngx_uint_t) -1)
#define SPLIT_INTERNAL ((ngx_uint_t) - 2)
#define SOURCE_INTERNAL ((ngx_uint_t) - 1)

#define ARR_SIZE(struct_) \
    (sizeof((struct_)) / sizeof(struct_[0]))

static const u_char* DEFAULT_PARAMS =
    "request_time|"
    "bytes_sent|"
    "body_bytes_sent|"
    "request_length|"
    "rps|"
    "keepalive_rps|"
    "response_2xx_rps|"
    "response_3xx_rps|"
    "response_4xx_rps|"
    "response_5xx_rps|"
    "upstream_time|"
    "upstream_header_time"
#if defined (NGX_STAT_STUB)
    "|"
    "accepted|"
    "handled|"
    "requests|"
    "active|"
    "reading|"
    "writing|"
    "waiting|"
#endif /** NGX_STAT_STUB */
    ""

#define TEMPLATE_VARIABLES(host, split, param, interval) \
    {host, split, param, interval}


/** Shm mem struct */
typedef struct {
    time_t                      start_time, last_time, event_time;

    ngx_uint_t                  max_interval;

    ngx_http_stat_allocator_t  *allocator;

    ngx_http_stat_array_t      *metrics;
    ngx_http_stat_array_t      *statistics;

    ngx_http_stat_array_t      *params;
    ngx_http_stat_array_t      *internals;
} ngx_http_stat_storage_t;


/** Backend */
typedef struct {
    struct sockaddr   *sockaddr;
    socklen_t          socklen;
    ngx_str_t          name;
} ngx_http_stat_server_t;


/** Main conf */
typedef struct {

    ngx_uint_t                 enable;

    ngx_str_t                  protocol;
    ngx_str_t                  host;
    int                        port;
    ngx_http_stat_server_t     server;

    ngx_shm_zone_t            *shared;
    ngx_buf_t                  buffer;

    ngx_uint_t                 frequency;

    ngx_array_t               *sources;
    ngx_array_t               *intervals;
    ngx_array_t               *splits;

    ngx_uint_t timeout;

    ngx_array_t               *default_params;

    size_t                     shared_size;
    size_t                     buffer_size;
    size_t                     package_size;

    ngx_array_t               *template;

    ngx_array_t               *default_data_template;
    ngx_array_t               *default_data_params;
    ngx_http_complex_value_t  *default_data_filter;

    ngx_array_t               *datas;

    ngx_http_stat_storage_t   *storage;

    ngx_connection_t          *connection;

} ngx_http_stat_main_conf_t;

/** Srv conf */
typedef struct {
    ngx_array_t              *default_data_template;
    ngx_array_t              *default_data_params;
    ngx_http_complex_value_t *default_data_filter;
    ngx_array_t              *datas;
} ngx_http_stat_srv_conf_t;

/** Loc conf */
typedef struct {
    ngx_array_t *datas;
} ngx_http_stat_loc_conf_t;

/** Context */
typedef struct {
    ngx_int_t                  phase;
    ngx_http_stat_storage_t   *storage;
    ngx_pool_t                *pool;
    ngx_log_t                 *log;
    ngx_http_stat_main_conf_t *smcf;
} ngx_http_stat_ctx_t;

/** Acc && Inver funcs API {{{ */
typedef struct {
    ngx_str_t       name;
    ngx_uint_t      value;
} ngx_http_stat_interval_t;

typedef struct {
    double          value;
    ngx_uint_t      count;
} ngx_http_stat_acc_t;

#define P2_METRIC_COUNT 5

typedef struct {
    double      q[P2_METRIC_COUNT];
    double      dn[P2_METRIC_COUNT];
    double      np[P2_METRIC_COUNT];
    ngx_int_t   n[P2_METRIC_COUNT];
    ngx_uint_t  count;
} ngx_http_stat_stt_t;

typedef double (*ngx_http_stat_aggregate_pt)(ngx_http_stat_interval_t*,
        ngx_http_stat_acc_t*);

typedef struct {
    ngx_str_t                  name;
    ngx_http_stat_aggregate_pt get;
} ngx_http_stat_aggregate_t;


double ngx_http_stat_aggregate_avg(ngx_http_stat_interval_t *interval,
    ngx_http_stat_acc_t *acc);
double ngx_http_stat_aggregate_persec(ngx_http_stat_interval_t *interval,
    ngx_http_stat_acc_t *acc);
double ngx_http_stat_aggregate_sum(ngx_http_stat_interval_t *interval,
    ngx_http_stat_acc_t *acc);
void ngx_http_stat_statistic_init(ngx_http_stat_stt_t *stt,
    ngx_uint_t percentile);
/** }}} */

/** Variables, Stats and metric API {{{ */
typedef enum {
    TEMPLATE_VARIABLE_HOST,
    TEMPLATE_VARIABLE_SPLIT,
    TEMPLATE_VARIABLE_PARAM,
    TEMPLATE_VARIABLE_INTERVAL,
} ngx_http_stat_template_variable_t;

typedef struct {
    ngx_str_t                   name;
    ngx_uint_t                  source;
    ngx_http_stat_interval_t    interval;
    ngx_http_stat_aggregate_pt  aggregate;
    ngx_uint_t                  percentile;
    ngx_http_stat_array_t      *percentiles;
} ngx_http_stat_param_t;


typedef struct ngx_http_stat_metric_s {
    ngx_uint_t                split;
    ngx_uint_t                param;
    ngx_http_stat_acc_t       *acc;
} ngx_http_stat_metric_t;


typedef struct {
    ngx_uint_t                split;
    ngx_uint_t                param;
    ngx_http_stat_stt_t       *stt;
} ngx_http_stat_statistic_t;


typedef struct {
    ngx_http_stat_array_t     *metrics;
    ngx_http_stat_array_t     *statistics;
    ngx_http_complex_value_t  *filter;
} ngx_http_stat_data_t;


typedef struct {
    ngx_str_t             name;
    ngx_http_stat_data_t  data;
} ngx_http_stat_internal_t;

typedef struct {
    ngx_str_t   name;
    int         variable;
} ngx_http_stat_template_arg_t;

typedef struct {
    ngx_str_t   data;
    int         variable;
} ngx_http_stat_template_t;

char *ngx_http_stat_template_compile(ngx_http_stat_ctx_t *ctx,
    ngx_array_t *template, ngx_http_stat_template_arg_t *args,
    ngx_uint_t nargs, ngx_str_t *value);
u_char *ngx_http_stat_template_execute(u_char *buffer, ngx_uint_t buffer_size,
    ngx_array_t *template, ngx_str_t *variables[]);
ngx_uint_t ngx_http_stat_template_len( ngx_array_t *template,
    ngx_str_t *variables[]);
/** }}} */

/** Backend timed handlers {{{ */
void ngx_http_influx_udp_timer_handler(ngx_event_t *ev);
/** }}} */

ngx_int_t ngx_http_stat(ngx_http_request_t *r, ngx_str_t *name,
    double value, char *config);
void ngx_http_stat_gc(ngx_http_stat_main_conf_t *smcf, time_t ts);

#endif /** NGX_HTTP_STAT_MODULE_H_INCLUDED */

