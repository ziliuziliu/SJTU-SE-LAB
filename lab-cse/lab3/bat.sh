#!/bin/bash  
  
for i in $(seq 1 100)  
do   
echo $i
./start.sh 5
./test-lab2-part3-b ./yfs1 ./yfs2
./stop.sh
done
