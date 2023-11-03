#! /usr/bin/env python2

# example launch script for DisplayCluster, executed by startdisplaycluster
# this should work for most cases, but can be modified for a particular
# installation if necessary

import os
import xml.etree.ElementTree as ET
import subprocess
import shlex

if 'DISPLAYCLUSTER_DIR' in os.environ:
    dcPath = os.environ['DISPLAYCLUSTER_DIR']
else:
    print 'could not get DISPLAYCLUSTER_DIR!'
    exit(-3)

# get rank from appropriate MPI API environment variable
myRank = None

if 'OMPI_COMM_WORLD_RANK' in os.environ:
    myRank = int(os.environ['OMPI_COMM_WORLD_RANK'])
elif 'OMPI_MCA_ns_nds_vpid' in os.environ:
    myRank = int(os.environ['OMPI_MCA_ns_nds_vpid'])
elif 'MPIRUN_RANK' in os.environ:
    myRank = int(os.environ['MPIRUN_RANK'])
elif 'PMI_ID' in os.environ:
    myRank = int(os.environ['PMI_ID'])
else:
    print 'could not determine MPI rank!'
    exit(-4)

if 'DISPLAYCLUSTER_EXEC' not in os.environ:
    startCommand = 'displaycluster'
else:
    startCommand = os.environ['DISPLAYCLUSTER_EXEC']
    
if myRank == 0:
    subprocess.call(shlex.split(startCommand))
else:
    # configuration.xml gives the display
    display = None

    try:
        if 'DISPLAYCLUSTER_CONFIG' in os.environ:
            doc = ET.parse(os.environ['DISPLAYCLUSTER_CONFIG'])
        else:
            doc = ET.parse(dcPath + '/configuration.xml')

        elems = doc.findall('.//process')

        if len(elems) < myRank:
            print 'could not find process element for rank ' + str(myRank)
            exit(-5)

        elem = elems[myRank - 1]

        display = elem.get('display')

        if display != None:
            os.environ['DISPLAY'] = display
        else:
            os.environ['DISPLAY'] = ':0'
    except:
        print 'Error processing configuration.xml. Make sure you have created a configuration.xml and put it in ' + dcPath + '/. An example is provided in the examples/ directory.'
        exit(-6)

    subprocess.call(shlex.split(startCommand))
