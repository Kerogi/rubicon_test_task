#!/bin/bash

echo "runing nodes" 
./build/nweb 12301 data data1.xml > nweb1.log &
./build/nweb 12302 data data2.xml > nweb2.log &
./build/nweb 12303 data data3.xml > nweb3.log &
./build/nweb 12304 data data4.xml > nweb4.log &

echo "runing node proxy" 
./build/nweb 12300 proxy peers.xml

killall nweb
