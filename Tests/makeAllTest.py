#!/usr/bin/env python3

import subprocess

TestFile = [
    "test-01-basic",
    "test-02-multiple-type",
    "test-03-add-count", 
    "test-04-priority-inversion", 
    "test-05-priority-inversion-PI",   
    "test-06-priority-inversion-PP",   
    "test-07-PI-chaining-effect",
    "test-08-benchmark",  
]

if __name__ == "__main__":
    for i in range(8):
        comp_proc = subprocess.run('make FILE={} NUM={} > /dev/null'.format(TestFile[i], i + 1), shell = True)
        if comp_proc.returncode:
            print(" ERROR: Compile Fail \n")
            exit(1)