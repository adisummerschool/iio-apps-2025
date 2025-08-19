#!/bin/bash

while true; do
	iio_attr -u ip:10.76.84.248 -c xadc voltage0 raw
	sleep 1
done
