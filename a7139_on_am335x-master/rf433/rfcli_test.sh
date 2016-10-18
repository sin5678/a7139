#!/bin/sh

counter=0
while [ $counter -lt 10000 ]; do
    echo counter=$counter
    rfcli -l
    let counter=counter+1
    sleep 3
done

