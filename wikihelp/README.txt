wikihelp - readme

The wikihelp folder contains a BASH script to automatically create a nice offline copy of the documentation available at doc.basic256.org.

To rebuild it from the WIKI:

1) make a temporary folder in /wikihelp named wiki

2) in the folder /wikihelp execute
	./downloadwiki.sh

3) AFTER step 2 is complete using BASIC256 execute the program in the /wikihelp folder
	cleanupHTML.kbs

4) in the folder /wikihelp execute
	./movelib.sh
	
5) once you are happy with the state of things delete your wiki download folder

j.m.reneau 2014-08-06
