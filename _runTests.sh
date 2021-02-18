#!/bin/bash

TESTS=5
TSTDIR=./tests
CREATE=./CreateFixedLengthDB
READ=./ReadAndFindDB

make

for iter in $(seq 1 $TESTS)
do
    printf "\nRunning test $iter:\n"
    $CREATE $TSTDIR/test$iter.txt $TSTDIR/out$iter.db

    for mode in $(seq 0 1)
    do
        $READ $TSTDIR/out$iter.db aaa $mode
    done
done