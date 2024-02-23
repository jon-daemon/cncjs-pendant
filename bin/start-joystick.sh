#!/usr/bin/env bash
CNCJS_SECRET=$(cat /home/pi/.cncrc | jq -r '.secret') /home/pi/cncjs-pendant/bin/cncjs-pendant-joystick --joystick=/dev/ttyACM0 -p /dev/ttyUSB0
