#!/bin/sh
# gethelp.sh - download and clean up the docuwiki documentation for basic256
#
# modification history
# programmer... date.... description...
# samlt         20110221 original coding from https://www.dokuwiki.org/tips:offline-dokuwiki.sh
# j.m.reneau    20121127 added logic to clean up downloaded file names and links
#
# tested and designed for docuwiki site running 2012-09-10 "Adora Belle RC1"
# will need to change DEF_DEPTH to 1 and test with future versions
# 

# default values
DEF_HOSTNAME=doc.basic256.org
#DEF_LOCATION=path/to/start
DEF_LOCATION=fullindex
USERNAME=
PASSWORD=
PROTO=http
DEF_DEPTH=2
PROGNAME=${0##*/}
 
: ${DEPTH:=$DEF_DEPTH}
: ${HOSTNAME:=$DEF_HOSTNAME}
: ${LOCATION:=$DEF_LOCATION}
 
PREFIX="help"
 
echo "[WGET] downloading: start: http://$HOSTNAME/$LOCATION (login/passwd=${USERNAME:-empty}/${PASSWORD:-empty})"
wget  --no-verbose \
      --recursive \
      --level="$DEPTH" \
      --execute robots=off \
      --no-parent \
      --page-requisites \
      --convert-links \
      --http-user="$USERNAME" \
      --http-password="$PASSWORD" \
      --auth-no-challenge \
      --adjust-extension \
      --exclude-directories=_detail,_export \
      --reject="feed.php*,detail.php*,*do=backlink*,*do=edit*,*do=index*,*indexer.php?id=*,*&rev=*,*do=dif*,*do=recent*,*do=export*,*do=media*,*do=rev*,*do=register*,*do=login*,*do=resendp*,*start&idx=*" \
      --directory-prefix="$PREFIX" \
      --no-host-directories \
	   --restrict-file-names=windows \
      $ADDITIONNAL_WGET_OPTS \
      "$PROTO://$HOSTNAME/$LOCATION"
 
 
echo
echo "[SED] fixing links(href...) in the HTML sources"
sed -i -e 's#href="\([^:]\+:\)#href="./\1#g' \
       -e "s#\(indexmenu_\S\+\.config\.urlbase='\)[^']\+'#\1./'#" \
       -e "s#\(indexmenu_\S\+\.add('[^']\+\)#\1.html#" \
       -e "s#\(indexmenu_\S\+\.add([^,]\+,[^,]\+,[^,]\+,[^,]\+,'\)\([^']\+\)'#\1./\2.html'#" \
       ${PREFIX}/*.html

echo
echo "[SED] stripping outwanted divs and fix external links"
sed -i -e "/<div id=\"dokuwiki__usertools\">/,/<\/div>/g" \
	-e "/<div class=\"mobileTools\">/,/<\/div>/g" \
	-e "/<div id=\"dokuwiki__sitetools\">/,/<\/div>/g" \
	-e "/<div class=\"trace\">/,/<\/div>/g" \
	-e "/<div class=\"tools\">/,/<\/div>/g" \
	-e "/<div id=\"dokuwiki__pagetools\">/,/<\/div>/g" \
	-e "/<div class=\"no\">/,/<\/div>/g" \
	-e "/<div class=\"buttons\">/,/<\/div>/g" \
	-e "/<div class=\"inclmeta/,/<\/div>/g" \
	-e "s/.\/http:/http:/g" \
	       ${PREFIX}/*.html

echo
echo "renaming files to get rid of php crap"
for i in ${PREFIX}/*.html;
do
  mv "$i" "`echo $i | sed "s/doku\.php.id=//g" | sed "s/%3A/_/g"`";
done

echo
echo "renaming image files to get rid of php crap"
for i in ${PREFIX}/lib/exe/fetch*;
do
   mv "$i" "`echo $i | sed "s/fetch\.php.*media=//g" | sed "s/%3A/_/g"`";
done

echo
echo "renaming css files to get rid of php crap"
for i in ${PREFIX}/lib/exe/css*;
do
   mv "$i" "`echo $i | sed "s/css\.php.*tseed=/css/g"`";
done

echo
echo "renaming javascript files to get rid of php crap"
for i in ${PREFIX}/lib/exe/js*;
do
   mv "$i" "`echo $i | sed "s/js\.php.*tseed=/js/g"`";
done

echo
echo "fixing the links after the file and image rename"
sed -i -e "s/href=\"doku\.php/href=\"\.\/doku.php/g" \
	-e "s/doku\.php.id=//g" \
	-e "s/fetch\.php.*media=//g" \
	-e "s/css\.php.*tseed=/css/g" \
	-e "s/js\.php.*tseed=/js/g" \
	-e "s/%3A/_/g" \
	-e "s/%253A/_/g" \
	       ${PREFIX}/*.html

echo
echo "now remove all downloaded but unused lib files"
for i in `find ${PREFIX}/lib`
do
    name=`echo $i | sed -e "s/${PREFIX}\///"`
    if ! grep -q "$name" ./${PREFIX}/*; then
        echo "$i is not referenced"
    fi
done

echo "fixing UTF8 Encoding"
sed -i -e "s/<meta charset/<meta http-equiv=\"Content-Type\" content=\"text\/html; charset=UTF-8\" \/><meta charset/g" \
	       ${PREFIX}/*.html




