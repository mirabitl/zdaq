#!/bin/bash
docker build -t zdaq_pi:1 -f- https://github.com/mirabitl/zdaq.git <<EOF
FROM debian:buster
RUN set -eux; \
    apt update; \
    apt -y install wget;\
    wget https://raw.githubusercontent.com/mirabitl/zdaq/master/docker_zdaq_install.sh;\
    chmod +x docker_zdaq_install.sh;\
    ./docker_zdaq_install.sh;
EOF
