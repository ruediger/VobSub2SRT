#
# To build for arm, see https://www.docker.com/blog/getting-started-with-docker-for-arm-on-linux/
#
# Build targets supported:
# docker build --build-arg=TARGET=pld packaging
# docker build --build-arg=TARGET=debian packaging

ARG VERSION=v1.0pre7
ARG TARGET=debian

FROM alpine AS source
RUN apk add git
ARG VERSION
RUN git clone --depth=1 -b $VERSION https://github.com/ruediger/VobSub2SRT /source

FROM debian:buster AS build-debian
RUN apt update && apt install -y libtiff5-dev libtesseract-dev tesseract-ocr-eng build-essential cmake pkg-config

FROM registry.gitlab.com/pld-linux/pld AS build-pld
RUN poldek -u cmake gzip libtiff-devel make tesseract-devel

FROM build-$TARGET AS build
WORKDIR /build
COPY --from=source /source .
RUN ./configure
RUN make
RUN make install
