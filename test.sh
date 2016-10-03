#!/bin/bash -x
dd if=/dev/urandom of=./random1 bs=M count=10
dd if=/dev/urandom of=./random2 bs=M count=10
dd if=/dev/urandom of=./random3 bs=M count=10
dd if=/dev/urandom of=./random4 bs=M count=10
dd if=/dev/urandom of=./random5 bs=M count=15


#echo  > random
#echo 22222222222222222222222 > random2
#echo 33333333333333333333333 > random3
#echo 44444444444444444444444 > random4
#echo 55555555555555555555555 > random5

( (./rw  random1 > /dev/null 2>&1) &
  (./rw  random2 > /dev/null 2>&1) &
  (./rw  random3 > /dev/null 2>&1) &
  (./rw  random4 > /dev/null 2>&1) &
  (./rw  random5 > /dev/null 2>&1) &
  (./rw  > /dev/null 2>&1) &
  (./rw  > /dev/null 2>&1) &
  (./rw  > /dev/null 2>&1) &
  (./rw  > /dev/null 2>&1) &
  (./rw  > /dev/null 2>&1) )


wait


(diff random1 random1.out 
 diff random2 random2.out
 diff random3 random3.out
 diff random4 random4.out
 diff random5 random5.out)

rm random*
