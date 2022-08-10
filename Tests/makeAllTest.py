#!/usr/bin/env python3

import subprocess
import argparse
import sys

TestFile = [
    "test-01-basic",
    "test-02-multiple-type",
    "test-03-add-count", 
    "test-04-priority-inversion", 
    "test-05-priority-inversion-PI",   
    "test-06-priority-inversion-PP",   
    "test-07-benchmark",  
    "test-08-benchmark-PI",  
    "test-09-benchmark-PP",  
    "test-10-condvar-signal",  
    "test-11-condvar-signal_pi", 
    "test-12-prio-wake", 
]

if __name__ == "__main__":
    if len(sys.argv) == 2 and sys.argv[1] == "pth":
        print("using pthread")
        for i in range(12):
            comp_proc = subprocess.run('make FILE={} NUM={} PTHREAD=1 > /dev/null'.format(TestFile[i], i + 1), shell = True)
            if comp_proc.returncode:
                print(" ERROR: Compile Fail \n")
                exit(1)
    else:
        print("using muthread")
        for i in range(12):
            comp_proc = subprocess.run('make FILE={} NUM={} > /dev/null'.format(TestFile[i], i + 1), shell = True)
            if comp_proc.returncode:
                print(" ERROR: Compile Fail \n")
                exit(1)