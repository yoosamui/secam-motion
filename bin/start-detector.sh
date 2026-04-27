#!/bin/bash


#!/bin/bash

# run-objectdetector.sh
# Usage:
# ./run-objectdetector.sh <param1> <param2>

# Full path to program
APP="/media/yoo/data/devs/of_v0.11.2_linux64gcc6_release/apps/myApps/secam-objdetector/objectdetector"

# Check if executable exists
if [ ! -x "$APP" ]; then
    echo "Error: objectdetector not found or not executable:"
    echo "$APP"
    exit 1
fi

# Need exactly 2 parameters
if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <param1> <param2>"
    exit 1
fi

# Run from any folder
# Run app and capture result
result=$("$APP" "$1" "$2")

# Show result
#echo "$RESULT"

#result=`./objectdetector $1 $2`

if [ "$result" == "detections=0" ]; then

    file=$1

  #  rm -f $file
    
 #   file=$2
    
    rm -f $file
    
    
fi

echo $result


