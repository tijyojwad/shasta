#!/usr/bin/python3

import shasta
import argparse


parser = argparse.ArgumentParser(description=
    'Write a FASTA file containing all the reads present in a local read graph.')
    
parser.add_argument('--readId', type=int, required=True)
parser.add_argument('--strand', type=int, choices=range(2), required=True)
parser.add_argument('--maxDistance', type=int, required=True)
parser.add_argument('--allowChimericReads', action='store_true')
arguments = parser.parse_args()

readId = arguments.readId
strand = arguments.strand
maxDistance = arguments.maxDistance
allowChimericReads = arguments.allowChimericReads

a = shasta.Assembler()
a.accessReadsReadOnly()
a.accessReadFlags()
a.accessReadNamesReadOnly()
a.accessAlignmentData()
a.accessReadGraph()
a.accessMarkers()
a.writeLocalReadGraphReads(
    readId=readId, strand=strand, 
    maxDistance=maxDistance, allowChimericReads=allowChimericReads)


