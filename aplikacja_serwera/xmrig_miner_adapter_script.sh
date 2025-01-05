#!/bin/bash

miner_id=$1
input=$2
session=xmrig_miner_session_$1
nr=0

function handle_signal()
{
    echo "deleting ${session}_${nr}"
    tmux kill-session -t "${session}_${nr}" 
    echo "deleted ${session}_${nr}"
    exit
}

# trapping the SIGINT signal
trap handle_signal SIGINT
trap handle_signal SIGTERM

# start xmrig miner
while true
do
    tmux new-session -d -s "${session}_${nr}" "~/xmrig_directory/xmrig/build/xmrig $input" 2>/dev/null
    [ $? -ne 0 ] || break # if(ret == 0) break;
    nr=$(( $nr + 1 ))
    sleep 2
done
echo "${session}_${nr}" "~/xmrig_directory/xmrig/build/xmrig $input"

while true
do
    sleep 5
done
