#!/bin/bash

pkill ffmpeg
exit
PIDFILE="/tmp/secam-recorder.pid"

if [ ! -f "$PIDFILE" ]; then
    echo "[REC] No PID file found."
    exit 1
fi

PID=$(cat "$PIDFILE")

if kill -0 "$PID" 2>/dev/null; then
    echo "[REC] Stopping ffmpeg PID $PID"
    
    # Graceful stop
    kill -INT "$PID"

    # Wait until process exits
    while kill -0 "$PID" 2>/dev/null; do
        sleep 0.2
    done

    echo "[REC] Stopped."
else
    echo "[REC] Process not running."
fi

rm -f "$PIDFILE"






