#! /bin/sh
#
# rfrepeater.sh          - Execute the rfrepeater command.
#
# Version: 1.0.0
#

name="rfrepeater"
cmd=/usr/sbin/rfrepeater
test -x "$cmd" || exit 1

case "$1" in
	start)
		echo -n "Starting $name. "
		start-stop-daemon --start --quiet --exec $cmd
		echo "."
		;;

	stop)
		echo -n "Stopping $name. "
		start-stop-daemon --stop --quiet --exec $cmd
		echo "."
		;;

	restart)
		echo -n "Stopping $name. "
		start-stop-daemon --stop --quiet --exec $cmd
		echo "."
		echo -n "Waiting for cmd to die off"
		for i in 1 2 3 ;
		do
			sleep 1
			echo -n "."
		done
		echo ""
		echo -n "Starting $name. "
		start-stop-daemon --start --quiet --exec $cmd
		echo "."
		;;

	*)
        echo "Usage: $0 {start|stop|restart}"
        exit 1
esac

exit 0
