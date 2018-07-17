# usage: place this file in directory with terminal output files from 
# ./RNAprofile named with .out extensions, then run program

import os
import glob
curdir = os.getcwd()
outdir = curdir + '/sequences'
if not os.path.exists(outdir):
    os.makedirs(outdir)
for filename in glob.glob('*.out'):
    outfile = open(outdir + '/' + filename[:-3] + 'txt', 'w')
    with open(filename, 'r') as fp:
        for i, line in enumerate(fp):
            if i == 0:
                line = line[(line.find(' is ') + 4):line.find(' with ')]
                print(line)
                with open(outdir + '/' + filename[:-3] + 'txt', 'w') as outfile:
                    outfile.write(line)
   
