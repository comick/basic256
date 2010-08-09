#!/bin/bash
# ziplinuxsource.sh
# create a zip file of the trunk source for uploading to sourceforge

make clean
tar --exclude=".*" --exclude="*~" --exclude="./*.kbs" --exclude="./BASIC256" --exclude="./debug" --exclude="./tmp" --exclude="./release" --exclude="./ziplinuxsource.sh" -cvzf ../basic256_n.n.nx.tgz *
