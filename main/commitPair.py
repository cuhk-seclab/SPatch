#!/usr/bin/env python3
import json 

prt2cld=[]
cld2num={}

with open('./out') as file:
	for line in file:
		s=line.split()
		if(len(s)>=2):
			prt2cld.append(s)
			for i in range(1, len(s)):
				n=0
				if s[i] in cld2num.keys():
					n=cld2num[s[i]]
				n=n+1
				cld2num[s[i]]=n

f=open('./linux_1.dict','w')  
json.dump(prt2cld,f)
f.close()

f1=open('./linux_2.dict', 'w')
json.dump(cld2num,f1)
f1.close()

	

	
