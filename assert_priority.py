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
        if(i == 2 and count % 3 == 1):
            PI_times += 1;
    count /= 3;
    print("priority_inversion times = {} for {} runs".format(PI_times, count))