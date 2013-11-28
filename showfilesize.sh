for i in 032825 033303 033741 034218 034656 035133 035610 040047 040526 041003 041440 041917 042354 042831 043308; do
	echo "Searching time $i";
	for f in `find /local? -path *AARTFAAC/CS00?_RSP*/AARTFAAC-20120712/1342063705-1342067305/$i/CS00*.dip`; do
		fname=`basename $f`;
		ls -lh $f | awk -F " " '{print $5 " " $9}';
	done;
	echo "-------- Done --------"
	echo "Size : `find /local? -path *AARTFAAC/CS00?_RSP*/AARTFAAC-20120712/1342063705-1342067305/$i -exec du -chs {} \;`"
	read;
done;
