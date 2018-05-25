FROM alpine:3.7

RUN apk add --update --no-cache build-base pcre pcre-dev zlib zlib-dev


ADD . /stat
WORKDIR /stat

RUN wget http://nginx.org/download/nginx-1.11.5.tar.gz && \
  tar xvfz nginx-1.11.5.tar.gz


WORKDIR nginx-1.11.5
RUN ./configure --add-module=/stat/

RUN make
RUN make install


