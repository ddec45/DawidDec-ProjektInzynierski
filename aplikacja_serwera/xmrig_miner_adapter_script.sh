#!/bin/bash

miner_id=$1
input=$2
session=xmrig_miner_session_$1
nr=0

function handle_sigint()
{
    tmux kill-session -t "${session}_${nr}" 
    exit
}

# trapping the SIGINT signal
trap handle_sigint SIGINT

# start xmrig miner
while true
do
    tmux new-session -d -s "${session}_${nr}" "/home/ddec45/xmrig_folder/xmrig/build/xmrig $input" 2>/dev/null
    [ $? -ne 0 ] || break # if(ret == 0) break;
    nr=$(( $nr + 1 ))
    sleep 5
done
echo "${session}_${nr}" "/home/ddec45/xmrig_folder/xmrig/build/xmrig $input"

while true
do
    sleep 5
done
