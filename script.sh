make &>/dev/null
for i in {1..50};
do
    sudo taskset -c 7 ./test
done