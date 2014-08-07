#!/bin/sh
# movelib.sh - move the lib files needed from the wiki folder
#
# modification history
# programmer... date.... description...
# j.m.reneau    20140806 r
#
# tested and designed for docuwiki site running 2013-12-18 "Blinky"
# will need to change DEF_DEPTH to 1 and test with future versions (2 is default)
# 

DOWNLOAD="wiki"
PREFIX="help"
 
echo
echo "moving image files and renaming"
mkdir ${PREFIX}/lib
mkdir ${PREFIX}/lib/exe

for i in ${DOWNLOAD}/lib/exe/fetch.php*;
do
   newi="`echo $i | sed "s/fetch\.php.*media=//g" | sed "s/%3A/_/g" | sed "s/^${DOWNLOAD}/${PREFIX}/"`"
    cmp $i $newi > /dev/null
    if [ $? -eq 0 ]; then
        echo "unchanged $i"
	     rm "$i"
    else
        echo "updated move $i to $newi"
        rm -f "$newi"
        mv "$i" "$newi";
    fi
done

echo
echo "renaming css files to get rid of php crap"
for i in ${DOWNLOAD}/lib/exe/css.php*;
do
   newi="`echo $i | sed "s/css\.php.*tseed=/css/g" | sed "s/^${DOWNLOAD}/${PREFIX}/"`"
    cmp $i $newi > /dev/null
    if [ $? -eq 0 ]; then
        echo "unchanged $i"
	     rm "$i"
    else
        echo "updated move $i to $newi"
        rm -f "$newi"
        mv "$i" "$newi";
    fi
done

exit

echo
echo "now remove all unreferenced html files"
for i in ${PREFIX}/*.html
do
    name=`echo $i | sed -e "s/${PREFIX}\///"`
    if ! grep -q "$name" ./${PREFIX}/*; then
        echo "$i is not referenced - deleted"
        rm -rf $i
    fi
done


echo
echo "now remove all downloaded but unused lib files (not .svn stuff)"
for i in `find ${PREFIX}/lib`
do
    echo "$i" | grep "\.svn" > /dev/null
    if [ $? -eq 1 ]; then
        name=`echo $i | sed -e "s/${PREFIX}\///"`
        if ! grep -q "$name" ./${PREFIX}/*; then
            echo "$i is not referenced - deleted"
            rm -rf $i
        fi
    fi
done


