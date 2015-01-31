#!/bin/sh
#---------------------------------------------
#     w3-home
#---------------------------------------------

# vconf reset
/usr/bin/vconftool set -t int "memory/private/org.tizen.w-home/tutorial" 0 -i -g 5000 -f -s org.tizen.w-home
/usr/bin/vconftool set -t int "db/private/org.tizen.w-home/enabled_tutorial" 1 -g 5000 -f -s org.tizen.w-home
/usr/bin/vconftool set -t int "db/private/org.tizen.w-home/apps_first_boot" 1 -g 5000 -f -s org.tizen.w-home
/usr/bin/vconftool set -t int "db/private/org.tizen.w-home/apps_flickup_count" 0 -g 5000 -f -s org.tizen.w-home
/usr/bin/vconftool set -t int "db/private/org.tizen.w-home/apps_initial_popup" 1 -g 5000 -f -s org.tizen.w-home
/usr/bin/vconftool set -t string "db/private/org.tizen.w-home/logging" ";" -g 5000 -f -s csystem::vconf_system
/usr/bin/vconftool set -t int "memory/homescreen/clock_visibility" 0 -i -g 5000 -f -s system::vconf_system
/usr/bin/vconftool set -t string "memory/homescreen/music_status" ";" -i -g 5000 -f -s system::vconf_system
/usr/bin/vconftool set -t int "memory/private/org.tizen.w-home/auto_feed" 1 -i -g 5000 -f -s org.tizen.w-home
/usr/bin/vconftool set -t int "memory/private/org.tizen.w-home/sensitive_move" 1 -i -g 5000 -f -s org.tizen.w-home

#badge DB reset
/usr/bin/sqlite3 /opt/dbspace/.badge.db "delete from badge_data; delete from badge_option; VACUUM;"
