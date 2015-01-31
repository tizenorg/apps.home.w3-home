#!/bin/sh

#--------------------------------------
# W-Home
#--------------------------------------

# user data

# vconf
/usr/bin/vconftool set -t int "db/private/org.tizen.w-home/apps_initial_popup" 1 -g 5000 -s org.tizen.w-home
/usr/bin/vconftool set -t string "memory/homescreen/music_status" ";" -i -g 5000 -f -s system::vconf_system

# db

# smack
