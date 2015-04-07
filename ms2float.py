#!/usr/bin/python
# Program to extract visibility data from .MS files, and write them out to 
# specified file as floats, to allow matlab to read the data.
# pep/08Feb12
# Added frequency information with every correlation matrix.
# pep/19May12

import sys;
import traceback
try:
	print 'PYTHONPATH: ';
	print  sys.path;
	from pyrap.tables import *
except ImportError:
	print 'Pyrap module not found!';

import numpy;
import os;


# Input MS files
# tsb0 = table ('SB001_1202-1207.MS');

def main ():
	if (len(sys.argv) < 4):
		print 'Usage: ', sys.argv[0], ' filename.MS Starttime(MJD) Endtime(MJD)';
		sys.exit (-1);

	msfile = sys.argv[1]; 
	nelem  = 288;        # TODO: Get from MS
	nblines= nelem*(nelem+1)/2; 
	nsubs  = 1;         # Number of subbands
	bufsize = 30; # In units of seconds.
	
	# Create an array for every timeslice from all subbands
	tobs = numpy.zeros (1, 'd'); # Time of obs, as double
	acm = numpy.zeros (nsubs*2*nblines, 'f');
	tab = table (sys.argv[1]); #Need to open table for getting spectral info
	tab1 = table(tab.getkeyword('SPECTRAL_WINDOW'));
	nchan = tab1[0]['NUM_CHAN'];
	print '-->Found ', nchan, ' Spectral channels';
	freqobs = numpy.zeros (nchan, 'd'); # Freq. of obs, Hz, as double
	for i in range (0, nchan):
		freqobs[i] = tab1[0]['CHAN_FREQ'][i];

	# Dont need the tables anymore
	tab1.close(); tab.close()

	if nchan >  1:
		print '### Currently handle only 1 channel!';
		# sys.exit(-1);

	# TODO: Dont try reading the entire MS into mem! Go in chunks
	cmd = 'select TIME from $msfile where TIME in {%20.8f, %20.8f}' % (float (sys.argv[2]), float (sys.argv[3]));
	ttim0 = taql (cmd);
	ntimes = ttim0.nrows()/nblines;
	print '---> Ants = ',nelem,' Subbands = ',nsubs,' Times = ', ntimes;
	print '---> Time range of data: ' + str (ttim0[0]['TIME']) + ' and ' + str(ttim0[-1]['TIME']);

	# tsb1 = taql ('select DATA from SB001_1202-1207.MS');
	# tsb2 = taql ('select DATA from SB002_1202-1207.MS');
	# tsb3 = taql ('select DATA from SB003_1202-1207.MS');
	# tsb4 = taql ('select DATA from SB004_1202-1207.MS');
	
	# Output .bin file
	# fname = '115908_taql.bin';
	foutname = sys.argv[1].split('.')[0] + '_YY.bin';
	print 'Writing to file: ', foutname;
	if not os.path.isfile (foutname):
		ffloat = open (foutname, 'wb');
	else:
		print '	   ### File exists! Quitting!';
		sys.exit (-1);
	
	i = 0;
	done = 0;
	for i in range (0, ntimes, bufsize):
		# Load the data for this subset
		print '--> Now reading between times %20.8f and %20.8f' % (ttim0[i*nblines]['TIME'], ttim0[(i+bufsize-1)*nblines]['TIME']);
		cmd = 'select DATA from $msfile where TIME in {%20.8f, %20.8f}' % (ttim0[i*nblines]['TIME'], ttim0[(i+bufsize-1)*nblines]['TIME']);
		tsb0 = taql (cmd);
		print 'Read %d rows, %d records' % (tsb0.nrows(), tsb0.nrows()/nblines);
		nrecs = tsb0.nrows()/nblines; # NOTE that sometimes lesser or more rows 
		# may be read due to the specification of the time to the TAQL command.
		for k in range (0, nrecs):
			if done == 1:
				print 'Done: ',i, 'timeslices' ;
				break;
		
		
			print 'Time:%f of %f, Freq:%f' % (ttim0[(i+k)*nblines]['TIME'], ttim0[-1]['TIME'], freqobs[0]) ;
			tobs[0] = ttim0[(i+k)*nblines]['TIME'];
			for j in range (0, nblines): # NOTE: Previously had an off-by-one error! pep/27Apr12
				try:
					b = tsb0[k*nblines+j]['DATA'];
					# acm[2*j  ]=b[0][0].real;
					# acm[2*j+1]=b[0][0].imag;

					#Changed to extract out Y-pols. pep/29Mar15
					acm[2*j  ]=b[0][3].real;
					acm[2*j+1]=b[0][3].imag;
					# print 'j:', '%05d' % j, '(','%8.4f' % tsb0[i*nblines+j]['DATA'][0][0].real,',', '%8.4f' % tsb0[i*nblines+j]['DATA'][0][0].imag, ')';
				except KeyboardInterrupt:
					print '    ###  Keyboard interrupt...';
					print '    ### Quit after current timeslice!';
					print '(i,j)=',i,j;
					done = 1;

			tobs.tofile (ffloat);
			freqobs.tofile(ffloat);
			acm.tofile  (ffloat);
			bytes = tobs.nbytes + freqobs.nbytes + acm.nbytes;
	
	ffloat.close ();
	return;

if __name__ == "__main__":
	main ();
