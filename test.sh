#!/bin/bash 

set +x
echo "Copying random data to files"
set -x

dd if=/dev/urandom of=./random1 bs=M count=10 &
dd if=/dev/urandom of=./random2 bs=M count=10 &
dd if=/dev/urandom of=./random3 bs=M count=10 &
dd if=/dev/urandom of=./random4 bs=M count=10 &
dd if=/dev/urandom of=./random5 bs=M count=15 &

wait

set +x
echo "Running five clients and five servers"
set -x

./rw  random1 > /dev/null &
sleep 6 && (./rw  random2 > /dev/null) &
./rw  random3 > /dev/null &
./rw  random4 > /dev/null &
./rw  random5 > /dev/null &
./rw  > /dev/null &
sleep 3 && (./rw  > /dev/null) &
./rw  > /dev/null &
./rw  > /dev/null &
./rw  > /dev/null &


wait

set +x
echo -e "-----------------------------------------------------------------"
echo -e "                            RESULTS                              "
echo -e "-----------------------------------------------------------------"
set -x

diff random1 random1.out 
diff random2 random2.out
diff random3 random3.out
diff random4 random4.out
diff random5 random5.out

rm random*
