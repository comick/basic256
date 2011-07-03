#!/bin/bash
# ziplinuxsource.sh
# create a zip file of the trunk source for uploading to sourceforge

make clean

# rename files we do not want to incluse in the tar
for file in *.sqlite3 *.kbs *.bin
do
	mv $file $file.donttar
	echo Moved $file to $file.donttar
done

tar \
	-cvzf ../basic256_n.n.nx.tgz \
	--exclude='.*' \
	--exclude='*.donttar' \
	--exclude='*~' \
	--exclude='BASIC256' \
	--exclude='debug' \
	--exclude='tmp' \
	--exclude='release' \
	*

# name all of the files in the . folde rback to their previous names
for file in *.donttar
do
	mv $file `basename $file .donttar`
	echo Moved $file back to original `basename $file .donttar`
done


