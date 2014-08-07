#!/bin/sh
# downloadwiki.sh - download the wiki into a temporary folder
#
# modification history
# programmer... date.... description...
# samlt         20110221 original coding from https://www.dokuwiki.org/tips:offline-dokuwiki.sh
# j.m.reneau    20121127 added logic to clean up downloaded file names and links
# j.m.reneau    20121129 restructured to only change the files that need changing (nice to svn)
# j.m.reneau    20140309 added rule to strip span with class of tooltip
# j.m.reneau    20140806 cleaned up sed logic to do a better job
# j.m.reneau    20140806 split into own script
#
# tested and designed for docuwiki site running 2013-12-18 "Blinky"
# will need to change DEF_DEPTH to 1 and test with future versions (2 is default)
# 

# default values
DEF_HOSTNAME=doc.basic256.org
DEF_LOCATION=start
USERNAME=
PASSWORD=
PROTO=http
DEF_DEPTH=2
PROGNAME=${0##*/}
 
: ${DEPTH:=$DEF_DEPTH}
: ${HOSTNAME:=$DEF_HOSTNAME}
: ${LOCATION:=$DEF_LOCATION}



PREFIX="wiki"
 
echo "[WGET] downloading: start: http://$HOSTNAME/$LOCATION (login/passwd=${USERNAME:-empty}/${PASSWORD:-empty})"
wget  --no-verbose \
      --level="$DEPTH" \
      --execute robots=off \
      --no-parent \
      --recursive \
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
 
echo "now remove all downloaded but unused lib files (not .svn stuff)"
