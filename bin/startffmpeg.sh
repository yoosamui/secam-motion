#!/bin/bash



# standard
ffmpeg -i $1 -metadata title="secam by Juan R. Gonzalez"  -vcodec copy -acodec copy $2 -y


# FULL-HD 1080p/25fps
#ffmpeg -i $1 -c:v libx264 -profile:v high -level:v 4.1 -preset veryfast -b:v 3000k -maxrate 3000k -bufsize 6000k -pix_fmt yuv420p -g 50 -keyint_min 50 -sc_threshold 0 -c:a aac -b:a 128k -ac 2 -ar 44100 $2 -y
exit;



