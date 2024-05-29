#!/usr/bin/env python3

def revert(oldname, newname, generatedname, inValidCUs, add, delete, upt):
    add1=dict()
    delete1=[]
    upt1=dict()
    for CU in inValidCUs:
        for action in CU:
            if len(action)<3:
                continue
            if action[len(action)-1]=='d':
                action=action[:-2]
                actionl=action.split(',')
                delete1.append(actionl)
            elif action[len(action)-1]=='a':
                action=action[:-2]
                if action not in add:
                    continue
                v=add[action]
                vl=v.split(',')
                add1[action]=vl
            elif action[len(action)-1]=='u':
                action=action[:-2]
                if action not in upt:
                    continue
                v=upt[action]
                k=action.split(',')
                v=v.split(',')
                upt1[action]=v

    addC=dict()
    uptC=dict()

    for i in add1:
        addC[i]=""
    for i in upt1:
        uptC[i]=""

    linenum=0
    with open(oldname) as file:
        for line in file:
            linenum=linenum+1
            flag=False
            for up in upt1:
                if str(linenum) in upt1[up]:
                    uptC[up]=uptC[up]+line
                    flag=True
                    break
            if flag:
                continue
            for ad in add1:
                if str(linenum) in add1[ad]:
                    addC[ad]=addC[ad]+line
                    break

    print("upt1: "+str(upt1)+" "+str(uptC))
    print("add1: "+str(add1)+" "+str(addC))

    linenum=0
    fw=open(generatedname, 'w')
    with open(newname) as file:
        for line in file:
            #have found the action?
            linenum=linenum+1
            flag=False
            for uk in uptC:
                if str(linenum) in uk.split(","):
                    ukn=uk[uk.rfind(",")+1:]
                    print("ukn "+str(ukn))
                    if linenum==int(ukn):
                        fw.write(uptC[uk])
                    flag=True
                    break
            if flag:
                continue
            for dk in delete1:
                if str(linenum) in dk:
                    print("dk "+str(dk))
                    flag=True
                    break
            if flag:
                continue
            for ak in addC:
                if linenum==int(ak)+1:
                    print("ak "+str(ak))
                    fw.write(addC[ak])
                    flag=True
                    break
            #we still need to add original lines in cases of 'add'
            #if flag:
            #   continue

            fw.write(line)

        fw.close()
