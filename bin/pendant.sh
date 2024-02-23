#!/bin/bash
# Start / stop pendant daemon

case "$1" in
    start)
        CNCJS_SECRET=$(cat /home/pi/.cncrc | jq -r '.secret') /home/pi/cncjs-pendant/bin/cncjs-pendant -d /dev/ttyUSB1 -p /dev/ttyUSB0 > /dev/null 2>&1 &
        # CNCJS_SECRET=$(cat /home/pi/.cncrc | jq -r '.secret') /home/pi/cncjs-pendant/bin/cncjs-pendant -d /dev/ttyUSB1 -p /dev/ttyUSB0 &
        echo "$0: started"
        ;;
    stop)
        kill $(ps aux | grep cncjs-pendant | grep -v grep | awk '{ print $2 }')
        echo "$0: stopped"
        ;;
    *)
        echo "Usage: $0 {start|stop}" >&2
        ;;
esac
