#!/usr/bin/env python3
import optparse
import git
import ujson
import subprocess
from os.path import exists
from split.split import *

#parentCommit->childCommit
prt2cld=[]
cld2num={}

# get relationships of commits.
def preprocess():
    global prt2cld
    global cld2num
    f=open('./linux_1.dict','rb')
    prt2cld=ujson.load(f)  
    f.close() 
    f1=open('./linux_2.dict', 'rb')
    cld2num=ujson.load(f1)
    f1.close()
    prt2cld.reverse()
    # print(prt2cld['69b41ac87e4a664de78a395ff97166f0b2943210'])

def writeCompile(makelog):
    makeFiles=[]

    # read files in makelog, we only update these files as they are compiled.
    with open(makelog) as file:
        for line in file:
            l=line.split()
            # this line include gcc or g++
            if "gcc" in l or "g++" in l or "CC" in l or "cc" in l:
                makeFiles.append(line)

    return makeFiles

def process(startId, program, upDir):
    repo=git.Repo(upDir)
    start=False
    sum=0
    merge=0
    #get make log
    #makeDir="/data/spatch/"+program+"/make/"
    if not exists(upDir+"makelog"):
        ea=subprocess.run(['./make.sh', upDir], stderr=subprocess.DEVNULL, stdout=subprocess.PIPE, check=False)
    makelogs=writeCompile(upDir+"makelog")
    #print(makelogs)

    #iterate each commit
    for it in prt2cld:
        prt=it[0]
        if prt==startId:
            start=True
        if start==False:
            continue
        repo.git.checkout(prt, force=True)
        for i in range(1, len(it)):
            #a merging commit
            if cld2num[it[i]]!=1:
                merge=merge+1
                print("Merge commit "+str(merge))
                continue
            sum=sum+1
            print("Analyzing "+str(sum))
            print("checking "+prt+" "+it[i])
            diff=repo.git.diff(prt, it[i], "-U0")
            #print("diff "+diff)
            split(prt, it[i], diff, upDir, program, makelogs)
            
        #break
    print("sum "+str(sum))

if __name__ == '__main__':
    parser = optparse.OptionParser()
    parser.add_option("-p", help="Upstream program name (e.g., linux)", dest="program", default='linux')
    parser.add_option("-u", help="The directory to upstream program", dest="upDir", default='/data/spatch-data/apps/linux-master/')
    #13a7a6ac0a11197edcd0f756a035f472b42cdf8b
    #0576722919f13da8430d00e47aa58ba151505d90 //linux commit
    parser.add_option("-c", help="The starting commit ID", dest="startId", default='ea4142f6b10585f271a40ee52eec2f55e48aeccf')
    (opts, args) = parser.parse_args()

    upDir=opts.upDir
    startId=opts.startId
    program=opts.program

    preprocess()
    process(startId, program, upDir)
