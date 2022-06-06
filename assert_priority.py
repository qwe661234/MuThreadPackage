#!/usr/bin/env python3

import subprocess
import numpy as np
import matplotlib.pyplot as plt

count = 0;

if __name__ == "__main__":
    comp_proc = subprocess.run('sh script.sh > data.txt', shell = True)
    output = np.loadtxt('data.txt', dtype = 'int32').T
    PI_times = 0;
    for i in output:
        count += 1;
    for i in range(count - 1):    
        if(output[i] == 2 and output[i + 1] == 3 and output[i + 2] == 1 and i % 3 == 0):
            PI_times += 1
        i += 3
    count /= 3;
    print("priority_inversion times = {} for {} runs".format(PI_times, count))