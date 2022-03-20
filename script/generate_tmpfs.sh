#!/bin/bash

sudo mkdir -p /app/untrusted_memory && \
sudo mount -t tmpfs -o size=8G tmpfs /app/untrusted_memory

