# Takes a 3dl file and encodes it into a .azasset file that can be read by the
# AssetProcessor

import json
import sys, getopt

gResourcePath = ''

################################################################################
def Convert3dlFile(inputFile, outputFile):
	lutIntervals = []
	lutValues = []
	with open(gResourcePath + inputFile) as lut_file:
		alist = [line.rstrip().lstrip() for line in lut_file]
		for idx, ln in enumerate(alist):
			if idx == 0:
				values = ln.split(" ")
				for v in values:
					lutIntervals.append(v)
			else:
				values = ln.split(" ")
				if values is None or len(values) < 3:
					continue
				lutValues.append(values)	
	# write to output file
	with open(gResourcePath + outputFile, mode='w') as f:
		f.write("{\n")
		f.write("	\"Type\": \"JsonSerialization\",\n")
		f.write("	\"Version\": 1,\n")
		f.write("	\"ClassName\": \"LookupTableAsset\",\n")
		f.write("	\"ClassData\": {\n")
		f.write("		\"Name\": \"LookupTable\",\n")
		f.write("		\"Intervals\": [\n")
		for idx, v in enumerate(lutIntervals):
			f.write(" %s" % (v))
			if idx < (len(lutIntervals)-1):
				f.write(", ")
		f.write("],\n")
		f.write("		\"Values\": [\n")	
		for idx, pix in enumerate(lutValues):
			f.write(" %s, %s, %s" % (pix[0], pix[1], pix[2]))
			if idx < (len(lutValues)-1):
				f.write(",")
			f.write("\n")
		f.write("]\n")
		f.write("	}\n")
		f.write("}\n")
        
inputfile = ''
outputfile = ''
try:
	opts, args = getopt.getopt(sys.argv[1:],"hi:o:",["ifile=","ofile="])
except getopt.GetoptError:
	print('test.py -i <inputfile> -o <outputfile>')
	sys.exit(2)
for opt, arg in opts:
	print("opt %s  arg %s" % (opt, arg))
	if opt == '-h':
		print('test.py -i <inputfile> -o <outputfile>')
		sys.exit()
	elif opt in ("-i", "--ifile"):
		inputfile = arg
	elif opt in ("-o", "--ofile"):
		outputfile = arg
print('Input file is "%s"' % inputfile)
print('Output file is "%s"' % outputfile)
if inputfile == '' or outputfile == '':
	print('test.py -i <inputfile> -o <outputfile>')
else:
	Convert3dlFile(inputfile, outputfile)
