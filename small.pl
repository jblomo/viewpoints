#!/usr/bin/perl

print (")O+_06 Small-body initial data  (WARNING: Do not delete this line!!)\n");
print (") Lines beginning with `)' are ignored.\n");
print (" style (Cartesian, Asteroidal, Cometary) = Ast\n");

for ($i=0; $i<10000; $i++)
{
	$phase = rand(360.0);
	print ("MINOS0    ep=2450400.5\n");
	print ("1.1513383 0.4127106 3.93863 239.5017 344.85893 $phase 0 0 0\n");
}
