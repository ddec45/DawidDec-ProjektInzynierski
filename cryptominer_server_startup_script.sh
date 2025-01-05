#!/bin/bash
cd $HOME/DawidDec-ProjektInzynierski/aplikacja_serwera
./cryptominer_server >> $HOME/cryptominer_server_log.txt 2>&1 &
cd $HOME/DawidDec-ProjektInzynierski/program_wyswietlania
sudo python main.py  > /dev/null 2>&1 &