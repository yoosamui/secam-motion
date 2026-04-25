#!/bin/bash

result=`./objectdetector $1 $2`

if [ "$result" == "detections=0" ]; then

    file=$1

    rm -f $file
    
    file=$2
    
    rm -f $file
    
    
fi

echo $result


