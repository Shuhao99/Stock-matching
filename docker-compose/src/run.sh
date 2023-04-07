#!/bin/bash

make clean


make


# taskset -c 0-1 ./exchange_server
taskset -c 0-3 ./exchange_server


while true
do
    sleep 1
done
