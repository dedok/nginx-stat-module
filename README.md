# nginx-stat-module
-------------------

A nginx module for collecting location stats into a specified engine.

This module use shared memory segment to collect aggregated stats from all workers
and send calculated values for last minute to `a specefied engine` every 60s (default)
over UDP or TCP in non-blocking way.

Stats aggegation made on the fly in fixed size buffer allocated on server
start and does't affect server perfomance.

You can build this module as a dynamic one.

Some parts are based on [graphite-nginx-module](https://github.com/mailru/graphite-nginx-module)

*Note. This module is not distributed with the Nginx source.*

## Requirements
---------------
* nginx: 1.2.0 - 1.x.x

## Features
-----------
* Aggregation of location, server or http metrics
* Calculation of percentiles
* Sending data to:
* Influx over UDP or TCP in non-blocking way

## Versions
-----------
* v0.0.1 - beta.

## Contents
-----------
* [Directives](#Directives)
* [stat_config](#stat_config)
* [stat](#stat)
* [stat_default](#stat_default)
* [stat_param](#stat_param)
* [Aggregate functions](aggregate-functions)
* [Params](#params)
* [Percentiles](percentiles)
* [How to install](#how-to-install)

## Directives
-------------

## stat_config
--------------
**syntax:** *stat_config [key1=&lt;value1&gt; key2=&lt;value2&gt; ... keyN=&lt;valueN&gt;]*

**context:** *http*

Specify global settings for a whole server instance.

Param     | Required | Default       | Description
--------- | -------- | ------------- | -----------
host      |          | gethostname() | host name for all tag
server    | Yes      |               | an engine server IP address
protocol  | Yes      |               | an engine type
port      |          | 2003          | an engine server port
frequency |          | 60            | how often send values to the engine
intervals |          | 1m            | aggregation intervals, time interval list, vertical bar separator (`m` - minutes)
params    |          | *             | limit metrics list to track, vertical bar separator
shared    |          | 2m            | shared memory size, increase in case of `too small shared memory` error
buffer    |          | 64k           | network buffer size, increase in case of `too small buffer size` error
package   |          | 1400          | maximum UDP packet size
template  |          |               | template for graph name (default is $prefix.$host.$split.$param_$interval) 

Example:
```nginx
http {
  stat_config
     protocol=influx/udp
     server=127.0.0.1:8089
     frequency=1
     ;
}
```

[Back to contents](#contents)

## stat_default
---------------

**syntax:** *stat_default &lt;path prefix&gt; [params=&lt;params&gt;] [if=&lt;condition&gt;]*

**context:** *http, server*

Create measurement point in all nested locations.
You can use "$location" variable which represents the name of the current location with all non-alphanumeric characters replaced with "\_." Leading and trailing "\_" are deleted.

Example:
```nginx
   stat_default nginx.$location;

   location /foo/ {
   }

   location /bar/ {
   }
```

Data for `/foo/` will be sent to `nginx.foo`, data for `/bar/` - to `nginx.bar`.
The `<params>` parameter (1.3.0) specifies list of params to be collected for all nested locations. To add all default params, use \*.
The `<if>` parameter (1.1.0) enables conditional logging. A request will not be logged if the condition evaluates to "0" or an empty string.

[Back to contents](#contents)

## stat
-------
**syntax:** *stat &lt;path prefix&gt; [params=&lt;params&gt;] [if=&lt;condition&gt;]*

**context:** *http, server, location, if*

Create measurement point in specific location.

Example:
```nginx
    location /foo/ {
        graphite_data nginx.foo;
    }
```

The `<params>` parameter (1.3.0) specifies list of params to be collected for this location. To add all default params, use \*.
The `<if>` parameter (1.1.0) enables conditional logging. A request will not be logged if the condition evaluates to "0" or an empty string.

Example:
```nginx
    map $scheme $is_http { http 1; }
    map $scheme $is_https { https 1; }
    # ...
    location /bar/ {
        stat nginx.all.bar;
        stat nginx.http.bar if=$is_http;
        stat nginx.https.bar if=$is_https;
        stat nginx.arg params=rps|request_time;
        stat nginx.ext params=*|rps|request_time;
    }
```

[Back to contents](#contents)

## stat_param
-------------
**syntax:** *stat_param name=&lt;path&gt; interval=&lt;time value&gt; aggregate=&lt;func&gt;*

**context:** *location*

Param      | Required | Description
---------- | -------- | -----------
name       | Yes      | path prefix for all graphs
interval   | Yes\*    | aggregation interval, time intrval value format (`m` - minutes)
aggregate  | Yes\*    | aggregation function on values
percentile | Yes\*    | percentile level

## Aggregate functions
----------------------
func   | Description
------ | -----------
sum    | sum of values per interval
persec | sum of values per second  (`sum` devided on seconds in `interval`)
avg    | average value on interval

## Params
---------
Param                   | Units | Func | Description
----------------------- | ----- | ---- | ------------------------------------------
request\_time           | ms    | avg  | total time spent on serving request
bytes\_sent             | bytes | avg  | http response length
body\_bytes\_sent       | bytes | avg  | http response body length
request\_length         | bytes | avg  | http request length
content\_time           | ms    | avg  | time spent generating content inside nginx
upstream\_time          | ms    | avg  | time spent tailking with upstream
upstream\_connect\_time | ms    | avg  | time spent on upstream connect (nginx >= 1.9.1)
upstream\_header\_time  | ms    | avg  | time spent on upstream header (nginx >= 1.9.1)
rps                     | rps   | sum  | total requests number per aggregation interval
keepalive\_rps          | rps   | sum  | requests number sent over previously opened keepalive connection
response\_2xx\_rps      | rps   | sum  | total responses number with 2xx code
response\_3xx\_rps      | rps   | sum  | total responses number with 3xx code
response\_4xx\_rps      | rps   | sum  | total responses number with 4xx code
response\_5xx\_rps      | rps   | sum  | total responses number with 5xx code
response\_[0-9]{3}\_rps | rps   | sum  | total responses number with given code
upstream\_cache\_(miss\|bypass\|expired\|stale\|updating\|revalidated\|hit)\_rps | rps   | sum  | totar responses with a given upstream cache status

[Back to contents](#contents)

## Percentiles
--------------
To calculate percentile value for any parameter, set percentile level via `/`. E.g. `request_time/50|request_time/90|request_time/99`.

[Back to contents](#contents)

## How to install
-----------------

### Dynamic module:
-------------------
```bash
$> wget 'http://nginx.org/download/nginx-1.9.2.tar.gz'
$> tar -xzf nginx-1.9.2.tar.gz
$> cd nginx-1.9.2/
$> ./configure --add-dynamic-module=/path/to/nginx-stat-module
$> make
$> make install
```
### Static module:
------------------
```bash
$> wget 'http://nginx.org/download/nginx-1.9.2.tar.gz'
$> tar -xzf nginx-1.9.2.tar.gz
$> cd nginx-1.9.2/
$> ./configure --add-module=/path/to/nginx-stat-module
$> make
$> make install
```

[Back to contents](#contents)

