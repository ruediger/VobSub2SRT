# syntax = docker/dockerfile:experimental
#
# Requires Docker v18.06 or later and BuildKit mode to use cache mount
# Docker v18.06 also requires the daemon to be running in experimental mode.
#
# $ export DOCKER_BUILDKIT=1
#
# See https://github.com/moby/buildkit/blob/master/frontend/dockerfile/docs/experimental.md
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
RUN --mount=type=cache,id=apt,target=/var/cache/apt \
	apt update && apt install -y \
	build-essential cmake libtesseract-dev libtiff5-dev pkg-config tesseract-ocr-eng

FROM registry.gitlab.com/pld-linux/pld AS build-pld
RUN --mount=type=cache,id=poldek,target=/var/cache/poldek \
	poldek -O 'keep downloads=yes' --up -u \
	cmake gzip libtiff-devel make tesseract-devel

FROM build-$TARGET AS build
WORKDIR /build
COPY --from=source /source .
RUN ./configure
RUN make
RUN make install
