#!/bin/sh

#set -x
set  -eu

#sudo apt install libmosquitto-dev mosquitto mosquitto-clients

echo 'Compiling...'

gcc -Wall -Wno-unused-but-set-variable -Wno-unused-variable \
  -pedantic \
  -DBME680_FLOAT_POINT_COMPENSATION \
  -iquote. \
  ./bme680.c \
  ./argEval.c \
  ./main.c \
  -lm -lrt -lmosquitto \
  -o bme680

echo 'Compiled.'

