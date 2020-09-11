#!/bin/bash

#[1]: # of childs
#[2]: # of params
#[3]: max time
#[4]: exe

find . -name 'probe.[0-9]*' | xargs rm -f
rm -f probe.txt

name=$(basename $0 | cut -f 1 -d '.')
count=1

while true
do
    PARAMS=""

    for i in $(seq 1 $2)
    do
	PARAMS+=$(( $RANDOM % $3 + 1 ))" "
    done

    output="$name.txt"

    echo "./main $1 $4 $PARAMS"
    ./main $1 $4 $PARAMS > $output 2>&1

    grep "Error" $output

    if [[ $? -eq 0 ]]; then
	mv $output "$name.$count"
	echo "Error found in $name.$count!"
	count=$(( $count + 1 ))
    fi
done
