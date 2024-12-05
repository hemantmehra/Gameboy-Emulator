#!/bin/sh

set -xe

cc main.c -o main -I/usr/local/include -L/usr/local/lib -lraylib -lm
