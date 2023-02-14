#!/bin/bash


result=`./objdetector -c=0.53 -d=$1 -t=$2`

# if [ "$result" == "1" ]; then
# do something...
# fi



file=$1
file=$file$"/image*.jpg"
rm -f $file
echo $result


