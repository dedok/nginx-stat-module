
/**
 * (c) BSD-2-Cause, link: https://opensource.org/licenses/BSD-2-Clause
 * (c) V. Soshnikov, mailto: dedok.mad@gmail.com
 */

#include "ngx_http_stat_module.h"


static ngx_int_t ngx_http_influx_net_send_udp(
        ngx_http_stat_main_conf_t *smcf, ngx_log_t *log);
static ngx_int_t ngx_http_influx_net_connect_udp(
        ngx_http_stat_main_conf_t *smcf, ngx_log_t *log);
static u_char *ngx_http_influx_s11n_metric(ngx_http_stat_main_conf_t *smcf,
        ngx_http_stat_storage_t *storage, ngx_uint_t m,
        ngx_http_stat_interval_t *interval, time_t ts,
        u_char *buffer, ngx_uint_t buffer_size);
static u_char *ngx_http_influx_s11n_statistic(ngx_http_stat_main_conf_t *smcf,
        ngx_http_stat_storage_t *storage, ngx_uint_t s, time_t ts,
        u_char *buffer, ngx_uint_t buffer_size);


void
ngx_http_influx_udp_timer_handler(ngx_event_t *ev)
{
    time_t                       ts;
    ngx_http_stat_main_conf_t   *smcf;
    ngx_buf_t                   *buffer;
    u_char                      *b;
    ngx_slab_pool_t             *shpool;
    ngx_http_stat_storage_t     *storage;
    ngx_uint_t                   m, i, s;
    ngx_http_stat_metric_t      *metric;
    ngx_http_stat_param_t       *param;
    ngx_http_stat_interval_t    *interval;
    ngx_http_stat_statistic_t   *statistic;

    smcf = ev->data;

    buffer = &smcf->buffer;
    b = buffer->start;

    shpool = (ngx_slab_pool_t *) smcf->shared->shm.addr;
    storage = (ngx_http_stat_storage_t *) shpool->data;

    ts = ngx_time();

    /** Lock {{{ */
    ngx_shmtx_lock(&shpool->mutex);

    if ((ngx_uint_t) (ts - storage->event_time) * 1000 < smcf->frequency) {
        ngx_shmtx_unlock(&shpool->mutex);
        goto yeild;
    }

    if (smcf->connection) {
        ngx_log_error(NGX_LOG_NOTICE, ev->log, 0, "re-init connection");
        ngx_close_connection(smcf->connection);
        smcf->connection = NULL;
    }

    if (storage->allocator->nomemory) {
        ngx_log_error(NGX_LOG_ALERT, ev->log, 0,
                "shared memory is full");
    }

    storage->event_time = ts;

    ngx_http_stat_gc(smcf, storage->event_time);

    for (m = 0; m < storage->metrics->nelts; m++) {

         metric = &((ngx_http_stat_metric_t *) storage->metrics->elts)[m];
         param = &((ngx_http_stat_param_t *)
                 storage->params->elts)[metric->param];

        if (metric->split != SPLIT_INTERNAL) {

            for (i = 0; i < smcf->intervals->nelts; i++) {

                interval = &((ngx_http_stat_interval_t *)
                        smcf->intervals->elts)[i];

                b = ngx_http_influx_s11n_metric(smcf, storage, m,
                        interval, storage->event_time, b,
                        smcf->buffer_size - (b - buffer->start));
            }

        } else {

            b = ngx_http_influx_s11n_metric(smcf, storage, m,
                    &param->interval, storage->event_time, b,
                    smcf->buffer_size - (b - buffer->start));

        }

    }

    for (s = 0; s < storage->statistics->nelts; s++) {

        statistic = &((ngx_http_stat_statistic_t *)
                storage->statistics->elts)[s];

        param = &((ngx_http_stat_param_t *)
                storage->params->elts)[statistic->param];

        b = ngx_http_influx_s11n_statistic(smcf, storage, s,
                storage->event_time, b,
                smcf->buffer_size - (b - buffer->start));

        ngx_http_stat_statistic_init(statistic->stt, param->percentile);
    }

	*b = '\0';

    ngx_shmtx_unlock(&shpool->mutex);

    /** Lock }}} */

    if (b == buffer->start + smcf->buffer_size) {
        ngx_log_error(NGX_LOG_ALERT, ev->log, 0,
                "stat buffer size is too small");
        goto yeild;
    }

    if (b != buffer->start) {

        buffer->pos = buffer->start;
        buffer->last = b;

        ngx_http_influx_net_send_udp(smcf, ev->log);
    }

yeild:
    if (ngx_quit || ngx_terminate || ngx_exiting) {
        return;
    }

    ngx_time_update();

    ngx_add_timer(ev, smcf->frequency);
}


static
ngx_int_t
ngx_http_influx_net_send_udp(ngx_http_stat_main_conf_t *smcf,
        ngx_log_t *log)
{
    ngx_buf_t *b;
    u_char    *part, *next, *nl;
    ssize_t    n;

    if (smcf->connection) {
        return NGX_ERROR;
    }

    if (ngx_http_influx_net_connect_udp(smcf, log) != NGX_OK) {
        ngx_log_error(NGX_LOG_ERR, log, 0,
                "ngx_http_influx_net_connect_udp: connect to \"%V\" failed",
                &smcf->server.name);
        goto failed;
    }

    smcf->connection->data = smcf;

    b = &smcf->buffer;

    part = b->start;
    next = NULL;
    nl = NULL;

    while (*part) {

        next = part;
        nl = part;

        while ((next = (u_char*) ngx_strchr(next, '\n')) &&
                ((size_t) (next - part) <= smcf->package_size))
        {
            nl = next;
            next++;
        }

        if (nl > part) {

            n = ngx_send(smcf->connection, part, nl - part + 1);

            if (n == -1) {
                ngx_log_error(NGX_LOG_ERR, log, n,
                        "ngx_http_influx_net_connect_udp: "
                        "udp send to \"%V\" error",
                        &smcf->server.name);
                goto failed;
            }

            if (n != nl - part + 1) {
                ngx_log_error(NGX_LOG_ERR, log, 0,
                        "ngx_http_influx_net_connect_udp: udp send to \"%V\" "
                        "incomplete", &smcf->server.name);
                goto failed;
            }
        }
        else {
            ngx_log_error(NGX_LOG_ERR, log, 0,
                    "ngx_http_influx_net_connect_udp: package size too small, "
                    "need send %z to \"%V\"",
                    (size_t) (next - part), &smcf->server.name);
        }

        part = nl + 1;
    }

    ngx_close_connection(smcf->connection);
    smcf->connection = NULL;

    return NGX_OK;

failed:

    ngx_close_connection(smcf->connection);
    smcf->connection = NULL;

    return NGX_ERROR;
}


static ngx_int_t
ngx_http_influx_net_connect_udp(ngx_http_stat_main_conf_t *smcf,
        ngx_log_t *log)
{
    ngx_int_t         rc, event;
    ngx_socket_t      s;
    ngx_connection_t *c;
    ngx_event_t      *rev, *wev;

    s = ngx_socket(smcf->server.sockaddr->sa_family, SOCK_DGRAM, 0);

    if (s == (ngx_socket_t) -1) {
        ngx_log_error(NGX_LOG_ERR, log, ngx_socket_errno,
                "ngx_http_influx_net_connect_udp: " ngx_socket_n " failed");
        return NGX_ERROR;
    }

    c = ngx_get_connection(s, log);

    if (c == NULL) {
        if (ngx_close_socket(s) == -1) {
            ngx_log_error(NGX_LOG_ERR, log, ngx_socket_errno,
                    "ngx_http_influx_net_connect_udp: " ngx_close_socket_n
                    " failed");
        }

        return NGX_ERROR;
    }

    rev = c->read;
    wev = c->write;

    rev->log = log;
    wev->log = log;

    smcf->connection = c;

    c->number = ngx_atomic_fetch_add(ngx_connection_counter, 1);

    rc = connect(s, smcf->server.sockaddr, smcf->server.socklen);

    if (rc == -1) {
        ngx_log_error(NGX_LOG_ERR, log, ngx_socket_errno,
                "ngx_http_influx_net_connect_udp: connect failed");
        goto failed;
    }

    wev->ready = 1;

    event = (ngx_event_flags & NGX_USE_CLEAR_EVENT) ?
            NGX_CLEAR_EVENT: NGX_LEVEL_EVENT;

    if (ngx_add_event(rev, NGX_READ_EVENT, event) != NGX_OK) {
        goto failed;
    }

    return NGX_OK;

failed:

    ngx_close_connection(c);

    return NGX_ERROR;
}
/** }}} */

/** Serializer part
 */
static u_char *
ngx_http_influx_s11n_metric(ngx_http_stat_main_conf_t *smcf,
        ngx_http_stat_storage_t *storage, ngx_uint_t m,
        ngx_http_stat_interval_t *interval, time_t ts,
        u_char *buffer, ngx_uint_t buffer_size)
{
    ngx_uint_t                  l, a;
    double                      value;
    ngx_http_stat_metric_t     *metric;
    ngx_http_stat_param_t      *param;
    ngx_http_stat_acc_t         aggregate, *acc;
    u_char                     *b;
    ngx_str_t                  *split;

    metric = &((ngx_http_stat_metric_t *) storage->metrics->elts)[m];
    param = &((ngx_http_stat_param_t *) storage->params->elts)[metric->param];

    if (metric->acc == NULL) {
        return buffer;
    }

    aggregate.value = 0;
    aggregate.count = 0;

    for (l = 0; l < interval->value; l++) {

        if ((time_t) (ts - l - 1) >= storage->start_time) {
            a = (ts - l - 1 - storage->start_time) %
                    (storage->max_interval + 1);
            acc = &metric->acc[a];
            aggregate.value += acc->value;
            aggregate.count += acc->count;
        }
    }

    value = param->aggregate(interval, &aggregate);

    b = buffer;

    if (metric->split != SPLIT_INTERNAL) {

        split = &((ngx_str_t *) smcf->splits->elts)[metric->split];

        if (!smcf->template->nelts) {

            b = ngx_snprintf(b, buffer_size - (b - buffer),
                    "%V,location=%V,parameter=%V,interval=%V",
                    &smcf->host, split, &param->name, &interval->name);

        } else {

            ngx_str_t *variables[] = TEMPLATE_VARIABLES(
                    &smcf->host, split, &param->name, &interval->name);

            b = ngx_http_stat_template_execute(b, buffer_size - (b - buffer),
                    smcf->template, variables);
        }

    } else {

        b = ngx_snprintf(b, buffer_size - (b - buffer), "%V,parameter=%V",
                &smcf->host, &param->name);

    }

    b = ngx_snprintf(b, buffer_size - (b - buffer), " value=%.3f %T\n",
            value, ts);

    return b;
}


static u_char *
ngx_http_influx_s11n_statistic(ngx_http_stat_main_conf_t *smcf,
        ngx_http_stat_storage_t *storage, ngx_uint_t s, time_t ts,
        u_char *buffer, ngx_uint_t buffer_size)
{
    ngx_http_stat_statistic_t *statistic;
    ngx_http_stat_param_t     *param;
    ngx_http_stat_stt_t       *stt;
    u_char                     p[4], *b;
    ngx_str_t                  percentile, *split;

    statistic = &((ngx_http_stat_statistic_t *) storage->statistics->elts)[s];
    param = &((ngx_http_stat_param_t *) storage->params->elts)[statistic->param];

    if (statistic->stt == NULL) {
        return buffer;
    }

    percentile.data = p;
    percentile.len = ngx_snprintf(p, sizeof(p), "p%ui",
            param->percentile) - p;

    stt = statistic->stt;

    b = buffer;

    if (statistic->split != SPLIT_INTERNAL) {

        split = &((ngx_str_t *) smcf->splits->elts)[statistic->split];

        if (smcf->template->nelts == 0) {

            b = ngx_snprintf(b, buffer_size - (b - buffer),
                    "%V,location=%V,parameter=%V,percentile=%V",
                    &smcf->host, split, &param->name, &percentile);
        } else {
            ngx_str_t *variables[] = TEMPLATE_VARIABLES(
                    &smcf->host, split, &param->name, &percentile);
            b = ngx_http_stat_template_execute(b, buffer_size - (b - buffer),
                    smcf->template, variables);
        }
    } else {
        b = ngx_snprintf(b, buffer_size - (b - buffer),
                "%V,parameter=%V,percentile=%V",
                &smcf->host, &param->name, &percentile);
    }

    b = ngx_snprintf(b, buffer_size - (b - buffer), " value=%.3f %T\n",
            stt->q[P2_METRIC_COUNT / 2], ts);

    return b;
}
/** }}} */

