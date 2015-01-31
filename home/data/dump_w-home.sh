#!/bin/sh

W_HOME_DEBUG=$1/w-home
mkdir -p ${W_HOME_DEBUG}
/bin/cp -r /tmp/.w-home_log ${W_HOME_DEBUG}/log
