#!/bin/sh
echo "Content-type: text/html"
echo ""
if [ "$REQUEST_METHOD" = "POST" ]; then
    read QUERY_STRING
fi
eval $(echo "$QUERY_STRING"|awk -F'&' '{for(i=1;i<=NF;i++){print $i}}')

#cmd=`httpd -d $cmd`
#can_net_debug=`httpd -d $can_net_debug`
#com_net_debug=`httpd -d $com_net_debug`
#pos_net_debug=`httpd -d $pos_net_debug`
#rf433_debug=`httpd -d $rf433_debug`
#debug_local_remote=`httpd -d $debug_local_remote`
#debug_local_pts=`httpd -d $debug_local_pts`
#debug_remote_ip=`httpd -d $debug_remote_ip`

user_log_file="/data/syslog/user_normal_log"
sys_log_file="/data/syslog/system_log"
debug_log_file="/var/volatile/log/user_debug_log"
applog_tmp_file="/tmp/applog.tmp"
applog_script="/etc/init.d/syslogd.busybox"

cmdline=""

#echo "QUERY_STRING=$QUERY_STRING" > /test.txt

if [ "$cmd" = "set" ]; then
    if [ $debug_local_remote = "local" ]; then
        nvram set debug_local_remote local
        nvram set debug_local_pts $debug_local_pts
    else
        nvram set debug_local_remote remote
        nvram set debug_remote_ip $debug_remote_ip
    fi
    nvram commit
    for i in `echo "$QUERY_STRING"|awk -F'&' '{for(i=1;i<=NF;i++){if (index($i,"debug_level_"))print $i}}'`
    do
        $applog_script setlevel `echo $i | sed -e 's/=/ /g'`
    done
    
    $applog_script restart
elif [ "$cmd" = "clear" ]; then
    #echo "clear,logtype=$logtype" >> /test.txt
    if [ $logtype = "user_log" ]; then
        rm -f $user_log_file
    elif [ $logtype = "sys_log" ]; then
        rm -f $sys_log_file
    elif [ $logtype = "debug_log" ]; then
        rm -f $debug_log_file
    fi
elif [ "$cmd" = "refresh" ]; then
    #echo "refresh,logtype=$logtype" >> /test.txt
    if [ $logtype = "user_log" ]; then
        if [ -s $user_log_file ]; then
            cat $user_log_file
        fi
    elif [ $logtype = "sys_log" ]; then
        if [ -s $sys_log_file ]; then
            cat $sys_log_file
        fi
    elif [ $logtype = "debug_log" ]; then
        if [ -s $debug_log_type ]; then
            cat $debug_log_file
        fi
    fi
elif [ "$cmd" = "getconf" ]; then
    #echo "getconf" >> /test.txt
    echo "debug_local_remote=`nvram get debug_local_remote`"
    echo "debug_local_pts=`nvram get debug_local_pts`"
    echo "debug_remote_ip=`nvram get debug_remote_ip`"
    cat $applog_tmp_file
fi

