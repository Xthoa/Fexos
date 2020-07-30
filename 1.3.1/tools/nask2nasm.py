#nask2nasm.py
import sys
import os
file=open(sys.argv[1])
lines=file.readlines()
outln=['bits 32']
outln.extend(lines[6:])
fout=open(sys.argv[2],'w')
fout.writelines(outln)
if len(sys.argv)>3 and sys.argv[3]!='-q':
    print(outln)
