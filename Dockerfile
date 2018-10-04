FROM alpine:3.7
MAINTAINER aosviridov@domclick.ru

RUN apk add --update --no-cache build-base pcre pcre-dev zlib zlib-dev

ADD . /stat

RUN wget http://nginx.org/download/nginx-1.9.11.tar.gz && \
  tar xvfz nginx-1.9.11.tar.gz && \
  cd /nginx-1.9.11 && \
  ./configure \
  --sbin-path=/usr/local/nginx/nginx \
  --conf-path=/stat/conf/nginx_dc.conf \
  --pid-path=/usr/local/nginx/nginx.pid \
  --add-dynamic-module=/stat/ && \
  make && \
  make install && \
  mkdir -p /usr/local/nginx/www/static/
