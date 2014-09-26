#!/bin/bash

usage() {
	echo -e "$0 [setup|start|stop]\n"
}

if [[ $EUID -ne 0 ]]; then
	echo "Please run script as root"
	exit
fi

if [[ "$1" == "setup" ]]; then
	gnome-terminal -x /bin/bash -c "/usr/bin/watch -n 1 sensors" 2>/dev/null &
	gnome-terminal -x /bin/bash -c "/usr/bin/top" 2>/dev/null &

elif [[ "$1" == "start" ]]; then
	for i in {0..8}; do
		./bench cpu -t 10000 &
		PID=$!
		taskset -p $i $PID
	done

elif [[ "$1" == "stop" ]]; then
	for i in $(pgrep bench); do
		kill -9 $i
	done
else
	usage
	exit
fi

