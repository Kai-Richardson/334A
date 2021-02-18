#!/bin/bash

TESTS=5
TSTDIR=./tests
CREATE=./CreateFixedLengthDB
READ=./ReadAndFindDB

make

for iter in $(seq 1 $TESTS)
do
    echo "Running test $iter:"
    $CREATE $TSTDIR/test$iter.txt $TSTDIR/out$iter.db
    $READ $TSTDIR/out$iter.db aaa 0
done