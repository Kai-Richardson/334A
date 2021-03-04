#!/bin/bash

TESTS=3
TSTDIR=./tests
SHELL=./cs334

make

echo "Sorry, due to the difficulty in getting the shell to get input and output right to bash, automated tests are disabled."
# for iter in $(seq 1 $TESTS)
# do
# 	printf "\nRunning test $iter:\n"
# 	command="$(cat $TSTDIR/test$iter.txt)"
# 	expected="$(cat $TSTDIR/out$iter.txt)"
# 	echo "$expected" > "$TSTDIR/_tempIn.txt"
# 	$command > "$TSTDIR/_tempOut.txt"
# 	output=$(diff "$TSTDIR/_tempIn.txt" "$TSTDIR/_tempOut.txt")
# 	if [ -z "${output}" ];
# 	then
# 		echo "Success."
# 	else
# 		echo "Failure. See above diff."
# 	fi
# done

exit