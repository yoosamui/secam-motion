#!/bin/sh

./secam-motion -c=gate -h=500 -w=960 -p=0 &
./secam-motion -c=center -h=500 -w=960 -p=0 & 
./secam-motion -c=left -h=500 -w=960 -p=0  &
./secam-motion -c=garage -h=500 -w=960 -p=0 &
./secam-motion -c=entrance -h=500 -w=960 -p=0 &
./secam-motion -c=behind -h=500 -w=960 -p=0 &




