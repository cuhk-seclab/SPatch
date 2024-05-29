#!/bin/bash

joern-parse --nooverlays $1 --output /home/anonym/safeport/main/cpg.bin
joern --script slice.sc --params cpgFile="/home/anonym/safeport/main/cpg.bin",methodName=$2,dstFile=$4,sourceLine=$3
