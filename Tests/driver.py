#!/usr/bin/env python3

import subprocess
import numpy as np

RED = '\033[91m'
GREEN = '\033[92m'
WHITE = '\033[0m'

TestFile = [
    "test-01-basic",
    "test-02-multiple-type",
    "test-03-add-count", 
    "test-04-priority-inversion", 
    "test-05-priority-inversion-PI",   
    "test-06-priority-inversion-PP",  
    "test-07-benchmark",  
]
def printInColor(text, color):
    print(color, text, WHITE, sep = '')

if __name__ == "__main__":
    comp_proc = subprocess.run('make clean all> /dev/null', shell = True)
    if comp_proc.returncode:
        printInColor(" ERROR: Compile Fail \n", RED)
        exit(1)
    for i in range(1,4):
        comp_proc = subprocess.run('sudo ./{}'.format(TestFile[i - 1]), shell = True)
        if comp_proc.returncode:
            printInColor("+++ Test-{} Fail +++ \n".format(i), RED)
            exit(1)
        else:
            printInColor("+++ Test-{} Pass +++ \n".format(i), GREEN)
    for i in range (4,7):
        PI_times = 0;
        for runs in range(3):
            comp_proc = subprocess.run('sudo taskset -c 3 ./{} > data.txt'.format(TestFile[i - 1]), shell = True)
            output = np.loadtxt('data.txt', dtype = 'int32').T      
            if (output[0] == 2 and output[1] == 3 and output[2] == 1):
                PI_times += 1
        
        if comp_proc.returncode:
            printInColor("+++ Test-{} Fail +++ \n".format(i), RED)
            exit(1)
        else:
            if i == 4:
                printInColor("Normal mutex, Priority_inversion times = {} for {} runs".format(PI_times, runs + 1), WHITE)
            if i == 5:
                printInColor("Priority Inheritance mutex, Priority_inversion times = {} for {} runs".format(PI_times, runs + 1), WHITE)
            if i == 6:
                printInColor("Priority Protection mutex, Priority_inversion times = {} for {} runs".format(PI_times, runs + 1), WHITE)
            printInColor("+++ Test-{} Pass +++ \n".format(i), GREEN)