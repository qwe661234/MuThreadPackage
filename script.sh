for i in {1..100};
do
    sudo taskset -c 7 ./test
done