#
# To build for arm, see https://www.docker.com/blog/getting-started-with-docker-for-arm-on-linux/

ARG VERSION=v1.0pre7
ARG TARGET=debian

FROM alpine AS source
RUN apk add git
ARG VERSION
RUN git clone --depth=1 -b $VERSION https://github.com/ruediger/VobSub2SRT /source

FROM debian:buster AS build-debian
RUN apt update && apt install -y libtiff5-dev libtesseract-dev tesseract-ocr-eng build-essential cmake pkg-config

FROM build-$TARGET AS build
WORKDIR /build
COPY --from=source /source .
RUN ./configure
RUN make
RUN make install
