#!/bin/bash
export IFS=";"
save="/data/spatch/"$4"/cpgs"
corpus="/data/spatch/"$4"/corpus"
workspace=$PWD"/workspace"
filelist=$PWD"/"$3
cd $1
git checkout $2 -f

mkdir -p $save
mkdir -p $corpus

rm -rf $workspace

# echo $3
# generate CPG for each file
while IFS= read -r file; do
  echo "Parsing "$file
  id=$2"_"$file
  identifier=$save'/'${id//\//+}
  if test -f "$identifier"; then
    continue
  fi
  echo "Generate CPG at "$identifier
  if test -f "$file"; then
    joern-parse --nooverlays $file --output $identifier
  fi
done < $filelist
