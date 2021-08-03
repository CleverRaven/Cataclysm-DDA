FROM ghcr.io/nelhage/llama as llama
FROM ubuntu:focal
RUN apt-get update && \
    DEBIAN_FRONTEND=noninteractive \
    apt-get -y install \
        gcc g++ gcc-9 g++-9 ca-certificates \
        libncursesw5-dev libsdl2-dev libsdl2-ttf-dev libsdl2-image-dev \
        libsdl2-mixer-dev libpulse-dev gettext parallel && \
    apt-get clean
COPY --from=llama /llama_runtime /llama_runtime
WORKDIR /
ENTRYPOINT ["/llama_runtime"]
