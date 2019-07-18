#!/bin/bash
# The shell script for assignment 1.2
# Tobias Ranfft
# Philipp Schwarz
# 2019-04-08
usage() {
cat <<EOF
$0 [OPTIONS]
Tests mockup.c
OPTIONS:
-h --help Display this help
EOF
}
# ############################################################
echo "start"
seed_value_list="2806 225 353"
page_size_list="8 16 32 64"
page_rep_algo_list="FIFO CLOCK AGING"
search_algo_list="quicksort bubblesort"
if [ $# -lt 1 ]; then
echo "start after arg check"
mkdir results
rm results/*
echo "deleted results files"
for page_size in $page_size_list
do
    echo "first for loop"
    gcc mockup.c -o p1 -D VMEM_PAGESIZE=$page_size -D STOP_BEFORE_END
    gcc mockup.c -o p2 -D VMEM_PAGESIZE=$page_size
    echo "compiled mockup.c"
    
    for page_rep_algo in $page_rep_algo_list
    do
        echo "second for loop"
        for seed_value in $seed_value_list
        do
            echo "third for loop"
            for search_algo in $search_algo_list
            do
                echo "fourth for loop"
                ./p1 -$page_repo_algo &
                p1pid=$!
                echo "started p1 and saved pid"
                sleep 1s
                echo "slept 1s"
                ./p2 -seed=$seed_value -$search_algo > results/output_${seed_value}_${search_algo}_${page_repo_algo}_${page_size}
                echo "outputted p2"
                kill -s KILL $p1pid
                wait 2> /dev/null
                echo "killed p1"
            done
        done
    done
done

else
# at least 1 arg, let’s check it
case $1 in
"-h" | "--help") # the only valid arg
usage
;;
*) # anything else is not valid
echo "Invalid option"
esac
fi
exit 0
