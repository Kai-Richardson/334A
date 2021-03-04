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
        $READ $TSTDIR/out$iter.db f $mode
    done
done

$CREATE $TSTDIR/american-english-small $TSTDIR/english-small.db

for mode in $(seq 0 1)
do
    printf "\nRunning american-english-small test $mode:\n"

    $READ $TSTDIR/english-small.db Barack $mode
done

$CREATE $TSTDIR/american-english $TSTDIR/english.db

for mode in $(seq 0 1)
do
    printf "\nRunning american-english test $mode:\n"

    $READ $TSTDIR/english.db Obamacare $mode
done