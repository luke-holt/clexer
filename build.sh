#!/usr/bin/env sh

set -euox pipefail

gcc -Wall -Wpedantic main.c dynamic_array.c util.c -o clexer -g

