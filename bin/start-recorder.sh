#!/bin/bash

ffmpeg -i "$1" -metadata title="secam-motion"  -vcodec copy -acodec copy -framerate 30  "$2" -y 



