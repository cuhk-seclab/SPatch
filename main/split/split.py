#!/usr/bin/env python3
from collections import defaultdict
from os.path import exists
import os
import subprocess
import glob
import ujson
import git
import re
import networkx 
from networkx.algorithms.components.connected import connected_components
from split.gen import revert
import stat
import time
import shutil
from analyzer import *

totalFile=0
validFile=0
commitNum=0
validCommitNum=0
applidCommitNum=0
editAction=0
allCUlen=0
validCUlen=0
valid1CUlen=0
SPlen=0
uptFunnum=0
addFunnum=0
deleteFunnum=0


def checkin(ele, l2):
    for i in range(len(l2)):
        l=l2[i]
        if ele in l:
            return i
    
    return -1

def add2set(o, or1):
    for l in o:
        eles=l.split()
        eles=set(eles)
        or1.append(eles)

def checksubset(e, r):
    for i in r:
        if e.issubset(i):
            return True
    return False

def mergeDiff(o, p, u):
    or1, pr, ur=[], [], []
    ous, pus, oms, pms=set(), set(), set(), set()
    add2set(p, pr)
    u1=u.split("&")
    #print("u1 "+str(u1))
    #has update op
    #TODO: cannot handle pure add_node/delete_node operation currently because we dont know where the node is added/delete.
    if len(u1[0].split()):
        us=u1[0].split("\n")
        ous=set(us[0].split())
        pus=set(us[1].split())
    #has mov op
    if len(u1)>1 and len(u1[1].split()):
        us1=u1[1].split("\n")
        oms=set(us1[0].split())
        pms=set(us1[1].split())
    
    ret=dict()
    for i in pr:
        if len(i) and not checksubset(i, ret):
            ret[frozenset(i)]=[]
    if len(pus):
        if len(pus) and not checksubset(pus, ret):
            ret[frozenset(pus)]=[-1]
    if len(pms):
        if len(pms) and not checksubset(pms, ret):
            ret[frozenset(pms)]=oms

    return ret


#create a file to store function
def createFunc(dir, upFile, updatedFunc, commit, m, saveDir):
    #print("open "+dir+" "+upFile+" "+commit)

    tmp=set()
    ll=upFile.replace("/", "+")
    
    if isinstance(updatedFunc, set):
        for func in updatedFunc:
            filename=saveDir+commit+"_"+ll+"+"+func+".c"
            if not exists(filename) and func in m:
                #print("creating function file from a set: "+filename)
                tmp.add(func)
    else:
        filename=saveDir+commit+"_"+ll+"+"+updatedFunc+".c"
        if not exists(filename) and updatedFunc in m:
            #print("creating function file: "+filename)
            tmp.add(updatedFunc)

    max=-1
    fm=dict()
    i=1

    if not tmp:
        return

    repo=git.Repo(dir)
    repo.git.checkout(commit, force=True)
    
    for func in tmp:
        funcl=m[func]
        if(max<funcl[1]):
            max=funcl[1]

    with open(dir+upFile, errors='ignore') as file:
        #print(dir+upFile+""+str(line))
        for line in file:
            if i>max:
                break
            for func in tmp:
                funcl=m[func]
                if i>=funcl[0] and i<=funcl[1]:
                    if func in fm:
                        fm[func]=fm[func]+line
                    else:
                        fm[func]=line
                break
            i=i+1

    for f in fm:
        upFile=upFile.replace("/", "+")
        filename=saveDir+commit+"_"+ll+"+"+f+".c"
        fw=open(filename, "w")
        fw.write(fm[f])
        fw.close()

    #13a7a6ac0a11197edcd0f756a035f472b42cdf8b+arch+arm+mach-sa1100+neponset.c+neponset_probe

# when we need to "merge" code changes into a unit when they have overlapped stmts
def to_graph(l):
    G = networkx.Graph()
    for part in l:
        # each sublist is a bunch of nodes
        G.add_nodes_from(part)
        # it also imlies a number of edges:
        G.add_edges_from(to_edges(part))
    return G

def to_edges(l):
    """ 
        treat `l` as a Graph and returns it's edges 
        to_edges(['a','b','c','d']) -> [(a,b), (b,c),(c,d)]
    """
    it = iter(l)
    last = next(it)

    for current in it:
        yield last, current
        last = current    

#Gumtree is not always (even often not??) reliable, we need to use GNU/diff to "rebost" our outputs.
def parseDiff(diff):
    add=dict()
    delete=[]
    update=dict()
    diff='\n'+diff
    diff=diff.replace("<", " ")
    diff=diff.replace(">", " ")
    pattern="(\n[0-9]+.*\n)"
    actions=re.split(pattern, diff)
    actions.pop(0)
    loc=-1
    tp="unknown"
    #print(actions)
    it=1
    for action in actions:
        #info
        if it%2==1:
            if 'a' in action:
                action=action.replace("\n","")
                loc=action[0:action.find('a')]
                target=action[action.find('a')+1:]
                add[loc]=target
                tp='a'
            elif 'c' in action:
                action=action.replace("\n", "")
                loc=action[0:action.find('c')]
                target=action[action.find('c')+1:]
                update[loc]=target
                tp='c'
            elif 'd' in action:
                action=action.replace("\n", "")
                loc=action[0:action.find('d')]
                delete.append(loc)
                tp='d'
        it=it+1

    return add, delete, update

#check if an edit action is in the blaclists
#action:line(,linenumebr):(adu)
def checkValid(action, blacklists, add, delete, upt):
    if len(action)<3:
        return False 
    if action[len(action)-1]=='d':
        ac=action[:-2].split(',')
        for i in ac:
            i=i+'p'
            if i in blacklists:
                return False
    elif action[len(action)-1]=='a':
        ac=action[:-2]
        if ac not in add:
            return False
        ac=add[ac].split(',')
        for i in ac:
            i=i+'o'
            if i in blacklists:
                return False
    elif action[len(action)-1]=='u':
        k=action[:-2]
        if k not in upt:
            return False
        v=upt[k]
        k=k.split(',')
        v=v.split(',')
        for ki in k:
            ki=ki+'p'
            if ki in blacklists:
                return False
        for vi in v:
            vi=vi+'o'
            if vi in blacklists:
                return False   
    return True
        

# extract updated functions in a diff
# output: multiple pairs of original functions and updated functions in new commit
def generateFunc(prt, cld, diff, dir, program, makelogs):
    loc=defaultdict(list)
    loc1=defaultdict(list)
    if exists("file.txt"):
        os.remove("file.txt")
    fo = open("file.txt", "a")
    global commitNum, applidCommitNum, totalFile, validFile, validCommitNum
    isFull=True
    compFile=""
    fullCompFile=""
    tmpFilenum=validFile

    # split file
    files=diff.split("diff --git ")
    for f in files:
        #option: we do not want to waste much time analyzing a very huge commit.
        if len(loc)>500:
            break
        #####
        lines=f.split("\n")
        names=lines[0].split(" ")
        if(len(names)!=2):
            continue
        filename=names[1][2:]
        compFile=filename
        fullCompFile=dir+compFile
        # only C or C++ files
        if(filename.endswith(".c") or filename.endswith(".cc") or filename.endswith(".cpp")):
            totalFile=totalFile+1

            iscommand=False
            #is in make list?
            for cc in makelogs:
                l=cc.split()
                # compile this file
                if fullCompFile in l or compFile in l:
                    #print("Compile file: "+fullCompFile)
                    iscommand=True
                    break
            if not iscommand:
                continue

            validFile=validFile+1

            # we have to ensure the argument list is not too long
            for l in lines:
                # we do not analyze a newly added file or a deleted file
                if ("+++ /dev/null" in l or "--- /dev/null" in l):
                    isFull=False
                    break

                if(l.startswith("@@ -")):
                    #print(l)
                    data=l.split(" ")
                    line=data[1]
                    if line.startswith("-"):
                        d=line.find(",")
                        if d!=-1:
                            line=line[1:d]
                            loc1[filename].append(line)
                        elif line[1:].isdigit():
                            loc1[filename].append(line[1:])

                    line=data[2]
                    if line.startswith("+"):
                        d=line.find(",")
                        if d!=-1:
                            line=line[1:d]
                            loc[filename].append(line)
                        elif line[1:].isdigit():
                            loc[filename].append(line[1:])
                        # print(loc)
    
    tloc=list(loc.keys())+list(loc1.keys())
    for k in tloc:
        fo.write(k+"\n")
    fo.close()
    #print(str(tloc))

    #loc is not empty
    #if loc:
    #    validCommitNum=validCommitNum+1

    # generate cpg for each modified files in a commit. The CPG is stored in cpgDir.
    cpgDir="/data/spatch/"+program+"/cpgs/"
    genDir="/data/spatch/"+program+"/corpus/"
    subprocess.run(['./split/generateCPGs.sh', dir, prt, "file.txt", program], stderr=subprocess.DEVNULL, stdout=subprocess.DEVNULL, check=False)
    subprocess.run(['./split/generateCPGs.sh', dir, cld, "file.txt", program], stderr=subprocess.DEVNULL, stdout=subprocess.DEVNULL, check=False)
    updatedFunc=set()
    m0=dict()
    mm0=dict()
    file2Func=dict()
    m1=dict()
    mm1=dict()
    deleteF=dict()
    addF=dict()

    validCommit=False

    #get range of functions in original and updated files
    for l in loc:
        updatedFunc=set()
        upFile=dir+l
        ll=l.replace("/", "+")
        
        cpgFile=cpgDir+cld+"_"+ll
        idx=cpgFile.rfind('/')
        fileName=cpgFile[idx+1:]
        if exists(cpgFile):
            if not exists(genDir+"+"+fileName):
                print("create "+genDir+"+"+fileName)
                subprocess.run(['./split/rangeFunc.sh', cpgFile, program, genDir, upFile], stderr=subprocess.DEVNULL, stdout=subprocess.DEVNULL, check=False)
            #identify updated function names
            if not exists(genDir+"+"+fileName):
                continue
            f=open(genDir+"+"+fileName,'rb')
            m0=ujson.load(f)
            f.close() 
            mm0[l]=m0
            #addF[l]=m0
            for line in loc[l]:
                for key, value in m0.items():
                    #print(value)
                    if int(line)>value[0] and int(line)<value[1]:
                        #print("check Func: "+line+" "+key)
                        updatedFunc.add(key)
            file2Func[l]=updatedFunc
        
        #funcDir="/data/spatch/"+program+"/old/"
        #createFunc(dir, l, updatedFunc, prt, m0, funcDir)
        #e8e79ede44ec99e09f8604c23ee99dc25065a343 76c07b8265c68d9a89fb4c0a634e373a087f11be
    
    global valid1CUlen
    hasCom=False
    ovalid1CUlen=valid1CUlen
    for l in loc1:
        #updatedFunc=set()
        tmpl=l
        compFile=l
        fullCompFile=dir+l
        upFile=dir+l
        ll=l.replace("/", "+")

        cpgFile=cpgDir+prt+"_"+ll
        idx=cpgFile.rfind('/')
        fileName=cpgFile[idx+1:]
        if exists(cpgFile):
            if not exists(genDir+"+"+fileName):
                print("create "+genDir+"+"+fileName)
                subprocess.run(['./split/rangeFunc.sh', cpgFile, program, genDir, upFile], stderr=subprocess.DEVNULL, stdout=subprocess.DEVNULL, check=False)
            if not exists(genDir+"+"+fileName):
                continue
            f=open(genDir+"+"+fileName,'rb')
            m1=ujson.load(f)  
            f.close() 
            mm1[l]=m1
            #deleteF[l]=m1
            for line in loc1[l]:
                for key, value in m1.items():
                    #print(value)
                    if int(line)>value[0] and int(line)<value[1]:
                        #print("check Func1: "+line+" "+key)
                        #updatedFunc.add(key)
                        if l not in file2Func:
                            file2Func[l]=set()
                        file2Func[l].add(key)
            #file2Func[l]=updatedFunc

        #funcDir="/data/spatch/"+program+"/new/"
        #createFunc(dir, l, updatedFunc, cld, m1, funcDir)

    for l in file2Func:
        tmpl=l
        compFile=l
        fullCompFile=dir+l
        updatedFunc=file2Func[l]
        if l not in mm0:
            upFile=dir+l
            ll=l.replace("/", "+")
            
            cpgFile=cpgDir+cld+"_"+ll
            idx=cpgFile.rfind('/')
            fileName=cpgFile[idx+1:]
            if exists(cpgFile):
                if not exists(genDir+"+"+fileName):
                    print("create "+genDir+"+"+fileName)
                    subprocess.run(['./split/rangeFunc.sh', cpgFile, program, genDir, upFile], stderr=subprocess.DEVNULL, stdout=subprocess.DEVNULL, check=False)
                #identify updated function names
                if not exists(genDir+"+"+fileName):
                    continue
                f=open(genDir+"+"+fileName,'rb')
                m0=ujson.load(f)
                f.close() 
                mm0[l]=m0
        if l not in mm1:
            upFile=dir+l
            ll=l.replace("/", "+")

            cpgFile=cpgDir+prt+"_"+ll
            idx=cpgFile.rfind('/')
            fileName=cpgFile[idx+1:]
            if exists(cpgFile):
                if not exists(genDir+"+"+fileName):
                    print("create "+genDir+"+"+fileName)
                    subprocess.run(['./split/rangeFunc.sh', cpgFile, program, genDir, upFile], stderr=subprocess.DEVNULL, stdout=subprocess.DEVNULL, check=False)
                if not exists(genDir+"+"+fileName):
                    continue
                f=open(genDir+"+"+fileName,'rb')
                m1=ujson.load(f)  
                f.close() 
                mm1[l]=m1
        print("file function "+str(l)+" "+str(updatedFunc))
        funcDir="/data/spatch/"+program+"/old/"
        createFunc(dir, l, updatedFunc, prt, mm1[l], funcDir)
        funcDir="/data/spatch/"+program+"/new/"
        createFunc(dir, l, updatedFunc, cld, mm0[l], funcDir)
        ll=l.replace("/", "+")

        # generate CUs
        for func in updatedFunc:
            print(prt+" "+cld+" "+func)
            oldname="/data/spatch/"+program+"/old/"+prt+"_"+ll+"+"+func+".c"
            newname="/data/spatch/"+program+"/new/"+cld+"_"+ll+"+"+func+".c"
            generatedname="/data/spatch/"+program+"/generated/"+prt+"_"+cld+"_"+ll+"+"+func+".c"

            global uptFunnum

            #print(oldname+" "+newname)

            #generate a "patched" function from a new function
            if exists(oldname) and exists(newname):
                #edit actions
                ea=subprocess.run(['./split/diff.sh', oldname, newname], stderr=subprocess.DEVNULL, stdout=subprocess.PIPE, check=False)
                diff=ea.stdout.decode()
                add, delete, upt=parseDiff(diff)
                print("edit actions: ")
                print(add, delete, upt)
                global editAction
                editAction=editAction+len(add)+len(delete)+len(upt)

                #data dependencies of edit actions, loops, pointers, control-dependent stmts of ret, return stmts
                if len(add) or len(delete) or len(upt):
                    #validCommit=True
                    uptFunnum=uptFunnum+1
                    #check if we have analyzed the file, if not, backup the results.
                    #generate a hybrid file
                    diffu=subprocess.run(['./split/diffu.sh', oldname, newname], stderr=subprocess.DEVNULL, stdout=subprocess.PIPE, check=False).stdout.decode()
                    diffl=diffu.split("\n")[3:]
                    tmpf="/data/spatch/"+program+"/dataflow/"+"tmp.c"
                    tmpf1="/data/spatch/"+program+"/dataflow/"+"tmp1.c"
                    if exists(tmpf):
                        os.remove(tmpf)
                    if exists(tmpf1):
                        os.remove(tmpf1)
                    ftmp=open(tmpf, "a")
                    ftmp1=open(tmpf1, "a")
                    changed=[]
                    changed1=[]
                    #map lines in ftmp(1) to their original lines
                    lp2l=dict()
                    lo2l=dict()
                    ol, pl=0, 0
                    for i in range(len(diffl)):
                        if len(diffl[i])==0:
                            continue
                        if(diffl[i][0]=='+'):
                            changed.append(i+1)
                            diffl[i]=" "+diffl[i][1:]
                            ftmp.write(diffl[i]+"\n")
                            ftmp1.write("\n")
                            ol=ol+1
                        elif(diffl[i][0]=='-'):
                            changed1.append(i+1)
                            diffl[i]=" "+diffl[i][1:]
                            ftmp1.write(diffl[i]+"\n")
                            ftmp.write("\n")
                            pl=pl+1
                        else:
                            ftmp.write(diffl[i]+"\n")
                            ftmp1.write(diffl[i]+"\n")
                            ol=ol+1
                            pl=pl+1
                        lp2l[i+1]=pl
                        lo2l[i+1]=ol
                    ftmp.close()
                    ftmp1.close()
                    dataname="/data/spatch/"+program+"/dataflow/"+prt+"_"+cld+"_"+func
                    if not exists(dataname):
                        #data flow analysis
                        changes="_".join(str(x) for x in changed)
                        changes1="_".join(str(x) for x in changed1)
                        if changes!="" or changes1!="":
                            print("changes "+changes+" "+changes1)
                            subprocess.run(['./split/dataflow.sh', tmpf, func, changes, dataname], stderr=subprocess.DEVNULL, stdout=subprocess.DEVNULL, check=False) #oriigal function
                            subprocess.run(['./split/dataflow.sh', tmpf1, func, changes1, dataname], stderr=subprocess.DEVNULL, stdout=subprocess.DEVNULL, check=False) #patched function
                        #else purely add or delete
                    
                    #somehow still do not get the data?? skip the cornor cases
                    if not exists(dataname):
                        print("ErrorC func "+func)
                        #time.sleep(100)
                        continue

                    #note to convert dataname to diff!!!
                    #discard updates on loops and non-local vars
                    f=open(dataname, 'r')
                    blacklist_p=[]
                    blacklist_o=[]
                    data=f.read()
                    m=dict()
                    m1=dict()
                    dl=data.split("\n")
                    if len(dl)<10:
                        print("ErrorC dl "+str(dl))
                        continue
                    point_p=dl[5].replace("HashSet(", "").replace(")", "")
                    pointlist_p=point_p.split(", ")
                    for i in range(len(pointlist_p)):
                        if len(pointlist_p[i]) and int(pointlist_p[i]) in lp2l:
                            pointlist_p[i]=str(lp2l[int(pointlist_p[i])])
                    loop_p=dl[7].replace("HashSet(", "").replace(")", "")
                    looplist_p=loop_p.split(", ")
                    for i in range(len(looplist_p)):
                        if len(looplist_p[i]) and int(looplist_p[i]) in lp2l:
                            looplist_p[i]=str(lp2l[int(looplist_p[i])])
                    dp_ps=dl[9].split(')')
                    
                    for dp_p in dp_ps:
                        if not len(dp_p):
                            continue
                        change=dp_p.split(" ", 1)[0]
                        rel=dp_p.split(" ", 1)[1].replace('HashSet(', '')
                        rel_list=rel.split(", ")
                        for i in range(len(rel_list)):
                            if i in rel_list:
                                rel_list[i]=str(lp2l[int(rel_list[i])])
                        if int(change) not in lp2l:
                            continue
                        change=str(lp2l[int(change)])
                        m[change]=rel_list
                        #change pointers or update loops?
                        if (rel_list!=[''] and (not set(rel_list).isdisjoint(pointlist_p))) or change in looplist_p:
                            blacklist_p.append(change)
                    point_o=dl[0].replace("HashSet(", "").replace(")", "")
                    pointlist_o=point_o.split(", ")
                    for i in range(len(pointlist_o)):
                        if len(pointlist_o[i]) and int(pointlist_o[i]) in lo2l:
                            pointlist_o[i]=str(lo2l[int(pointlist_o[i])])
                    loop_o=dl[2].replace("HashSet(", "").replace(")", "")
                    looplist_o=loop_o.split(", ")
                    for i in range(len(looplist_o)):
                        if len(looplist_o[i]) and int(looplist_o[i]) in lo2l:
                            looplist_o[i]=str(lo2l[int(looplist_o[i])])
                    dp_os=dl[4].split(')')
                    for dp_o in dp_os:
                        if not len(dp_o):
                            continue
                        change=dp_o.split(" ", 1)[0]
                        rel=dp_o.split(" ", 1)[1].replace('HashSet(', '')
                        rel_list=rel.split(", ")
                        for i in range(len(rel_list)):
                            if i in rel_list:
                                rel_list[i]=str(lo2l[int(rel_list[i])])
                        if int(change) not in lo2l:
                            continue
                        change=str(lo2l[int(change)])
                        m1[change]=rel_list
                        #change pointers or update loops?
                        if (rel_list!=[''] and not set(rel_list).isdisjoint(pointlist_o)) or change in looplist_o:
                            blacklist_o.append(change)
                    
                    blacklists=set()
                    for i in blacklist_p:
                        blacklists.add(i+'p')
                    for i in blacklist_o:
                        blacklists.add(i+'o')
                    print("blacklists")
                    print(blacklists)
                    
                    #if code changes have dependency?
                    #first find the largest union set that do not insect with others, then find the key whose value overlap the largeest union set
                    dd=[]
                    ml=list(m.values())+list(m1.values())
                    #print(ml)
                    G = to_graph(ml)
                    dd=list(connected_components(G))

                    print("dd")
                    print(dd)
                    
                    dr=dict()
                    for k in m:
                        for i in dd:
                            if (not set(i).isdisjoint(m[k])):
                                if frozenset(i) not in dr:
                                    dr[frozenset(i)]=[]
                                dr[frozenset(i)].append(k+'p')
                                break
                    for k in m1:
                        for i in dd:
                            if (not set(i).isdisjoint(m1[k])):
                                if frozenset(i) not in dr:
                                    dr[frozenset(i)]=[]
                                dr[frozenset(i)].append(k+'o')
                                break
                    
                    dresults=dr.values()
                    print("dresults: ")
                    print(dresults)

                    #merge data dependencies with edit actions...
                    CUnitM=dict()
                    for dd in dresults:
                        CUnitM[frozenset(dd)]=set()

                    #a->b,c
                    for k in add:
                        pl=add[k].split(",")
                        #[b, c]
                        for l in pl:
                            l=l+'o'
                            for dd in dresults:
                                if l in dd:
                                    CUnitM[frozenset(dd)].add(k+':a')
                                    break
                    #a,b,c
                    for k in delete:
                        ol=k.split(",")
                        for l in ol:
                            l=l+'p'
                            for dd in dresults:
                                if l in dd:
                                    CUnitM[frozenset(dd)].add(k+":d")
                                    break
                    #a,b->c,d
                    for k in upt:
                        ol=k.split(",")
                        pl=upt[k].split(",")
                        #[a,b]
                        for l in ol:
                            l=l+'p'
                            for dd in dresults:
                                if l in dd:
                                    CUnitM[frozenset(dd)].add(k+':u')
                                    break
                        if frozenset(dd) not in CUnitM or (k+':u') in CUnitM[frozenset(dd)]:
                            continue
                        #[c,d]
                        for l in pl:
                            l=l+'o'
                            for dd in dresults:
                                if l in dd:
                                    CUnitM[frozenset(dd)].add(k+':u')
                                    break


                    #merge code changes into CUs.
                    global allCUlen
                    global SPlen
                    CUs=[]
                    CUnitL=[]
                    SPs=[]
                    for m in CUnitM.values():
                        if len(m):
                            CUnitL.append(list(m))
                    #print(ml)
                    print("CUnitL:")
                    print(CUnitL)
                    G = to_graph(CUnitL)
                    CUs=list(connected_components(G))
                    print("CUs:")
                    print(CUs)
                    allCUlen=allCUlen+len(CUs)

                    global validCUlen
                    validCUs=[]
                    invalidCUs=[]
                    #remove the CUs that include stmts in blacklists.
                    for CU in CUs:
                        valid=True
                        #iterate each edit actions in a CU (edit actions in a CU have data dependencies)
                        for action in CU:
                            valid=(valid and checkValid(action, blacklists, add, delete, upt))
                            if not valid:
                                invalidCUs.append(CU)
                                isFull=False
                                break
                        if valid:
                            validCUs.append(CU)
                    
                    print("valid CUs")
                    print(validCUs)
                    print("invalid CUs")
                    print(invalidCUs)

                    validCUlen=validCUlen+len(validCUs)
                    #if len(validCUs)==0:
                    #    continue

                    if exists(generatedname):
                        os.remove(generatedname)
                    revert(oldname, newname, generatedname, invalidCUs, add, delete, upt)

                    valid1CUs=[]
                    command=""
                    commandOp=""
                    iscommand=False
                    #find which lines cause compile errors and try to inline some functions
                    #Should we always update cg? Or we might miss some callee function because of the outdated CG. But now we have to toleratn it
                    for cc in makelogs:
                        l=cc.split()
                        # compile this file
                        if fullCompFile in l or compFile in l:
                            print("Compile file: "+fullCompFile)
                            command=cc
                            #need to adjust the locatiion here
                            commandOp=cc[cc.find("gcc")+1:]
                            validCommit=True
                            iscommand=True
                        if iscommand:
                            break
                    if not iscommand:
                        #validCommit=False
                        print("Check: "+fullCompFile+" "+compFile)
                        continue
                    #Replace the old functions with the generated func
                    #remember to checkout the old commit!
                    repo=git.Repo(dir)
                    repo.git.checkout(prt, force=True)
                    compDir=fullCompFile[:fullCompFile.rfind('/')]
                    newCompFile=compDir+"/1.c"
                    if exists(newCompFile):
                        os.remove(newCompFile)
                    fw=open(newCompFile, 'w')
                    contents=""
                    if not exists(fullCompFile):
                        print(fullCompFile)
                        continue
                    with open(fullCompFile, errors='ignore') as file:
                        contents=file.read()
                    contentsl=contents.split("\n")
                    min, max=0, 0
                    m1=mm1[tmpl]
                    if func in m1:
                        min=m1[func][0]
                        max=m1[func][1]
                    if min==0:
                        print("min: "+str(m1)+" "+func)
                        continue
                    print("min, max")
                    print(str(min)+" "+str(max))
                    prel=contentsl[:min-1]
                    postl=contentsl[max:]
                    mid=""
                    with open(generatedname, errors='ignore') as file:
                        mid=file.read()
                    midl=mid.split("\n")
                    newContentl=prel+midl+postl
                    newContent="\n".join(newContentl)
                    fw.write(newContent)
                    fw.close()

                    #compile the file and find errors.
                    command=command.replace(fullCompFile, newCompFile)
                    command=command.replace(compFile, newCompFile)

                    fw=open("command.sh", "w")
                    fw.write("#!/bin/bash\n")
                    fw.write("cd "+dir+"\n")
                    fw.write(command)
                    fw.close()
                    ea=subprocess.run(["./command.sh"], stderr=subprocess.PIPE, stdout=subprocess.PIPE, check=False)
                    err=ea.stderr.decode("utf-8") 
                    #print("compile error")
                    #print(err[:20])
                    comErr=[]
                    for e in err.split("\n"):
                        if "error:" in e and "implicit-function-declaration" not in e:
                            print("error "+e)
                            #iscomErr=True
                            eLocs=e.split("1.c:")
                            if len(eLocs)<2:
                                continue
                            e1=eLocs[1]
                            line=e1[:e1.find(":")]
                            if not line.isdigit():
                                continue
                            comErr.append(str(int(line)-min+1))

                    os.remove(generatedname)

                    if len(comErr)==0:
                        print("Find one! ")
                        hasCom=True
                        valid1CUlen=valid1CUlen+len(validCUs)
                        revert(oldname, newname, generatedname, invalidCUs, add, delete, upt)

                    else:
                        #apply no code changes as we cannot geneate valid program.
                        tmpL=[]
                        revert(newname, oldname, generatedname, tmpL, add, delete, upt)

                    #we have a compilable "minified program", let do symbolc execution (this py script "starts" the SE of Juxta)
                    if hasCom:
                        #identify the safe CUs that can be identified with data flow, and count the number
                        if len(dl[1])==0 and len(dl[6])==0:
                            SPlen=SPlen+1
                        else:
                            #if there are data flows, do SE.
                            if commandOp=="":
                                continue
                            subprocess.run(['./se.sh', "/data/spatch-data/targets/ori/", commandOp, dir], stderr=subprocess.DEVNULL, stdout=subprocess.DEVNULL, check=False)
                            shutil.copyfile(generatedname, fullCompFile+".c")
                            subprocess.run(['./se.sh', "/data/spatch-data/targets/upt/", commandOp, dir], stderr=subprocess.DEVNULL, stdout=subprocess.DEVNULL, check=False)
                            #the default storage dir is the dir we specified plus the source code dir
                            rel=test_analyzer_plus("/data/spatch-data/targets/ori/"+fullCompFile, "/data/spatch-data/targets/ori/"+fullCompFile+".c")
                            if not rel:
                                SPlen=SPlen+1
                            rel=test_analyzer("/data/spatch-data/targets/ori/"+fullCompFile, "/data/spatch-data/targets/ori/"+fullCompFile+".c")
                            if not rel:
                                SPlen=SPlen+1
                        

    #consider the deleted and added funcs
    global addFunnum
    global deleteFunnum
    for l in deleteF:
        if l not in addF:
            continue
        for f in deleteF[l]:
            if f not in addF[l]:
                #funcDir="/data/spatch/"+program+"/old/"
                #createFunc(dir, l, f, prt, m0, funcDir)
                deleteFunnum=deleteFunnum+1
        for f in addF[l]:
            if f not in deleteF[l]:
                #funcDir="/data/spatch/"+program+"/new/"
                #createFunc(dir, l, f, cld, m1, funcDir)
                addFunnum=addFunnum+1

    if validCommit and valid1CUlen!=ovalid1CUlen and hasCom:
        applidCommitNum=applidCommitNum+1
    commitNum=commitNum+1
    if validCommit:
        validCommitNum=validCommitNum+1

    #print("statistics:")
    #print(str(commitNum)+" "+str(validCommitNum)+" "+str(applidCommitNum)+" "+str(totalFile)+" "+str(validFile)+" "+str(editAction)+" "+str(allCUlen)+" "+str(validCUlen)+" " \
    #+str(valid1CUlen)+" "+" +SPlen+str(uptFunnum)+" "+str(addFunnum)+" "+str(deleteFunnum))
    
    return updatedFunc


#split a commit into multiple change units
def split(prt, cld, diff, dir, program, makelogs):
    #analyze functions
    updatedFunc=generateFunc(prt, cld, diff, dir, program, makelogs)

    #each function, find dependency code.

    #filter out loops and compilation errors.
