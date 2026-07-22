#! /usr/bin/env python3

# example launch script for DisplayCluster, executed by startdisplaycluster
# this should work for most cases, but can be modified for a particular
# installation if necessary

import os
import json
import subprocess
import shlex

if 'DISPLAYCLUSTER_DIR' in os.environ:
    dcPath = os.environ['DISPLAYCLUSTER_DIR']
else:
    print( 'could not get DISPLAYCLUSTER_DIR!')
    exit(-3)

# get rank from appropriate MPI API environment variable
myRank = 0

if 'OMPI_COMM_WORLD_RANK' in os.environ:
    myRank = int(os.environ['OMPI_COMM_WORLD_RANK'])
elif 'OMPI_MCA_ns_nds_vpid' in os.environ:
    myRank = int(os.environ['OMPI_MCA_ns_nds_vpid'])
elif 'MPIRUN_RANK' in os.environ:
    myRank = int(os.environ['MPIRUN_RANK'])
elif 'PMI_ID' in os.environ:
    myRank = int(os.environ['PMI_ID'])
else:
    print( 'could not determine MPI rank! Assuming testing with myRank = 0')

if 'DISPLAYCLUSTER_INSTALL' in os.environ:
    install_dir = os.environ['DISPLAYCLUSTER_INSTALL']
else:
    install_dir = '/usr/local'

if 'DISPLAYCLUSTER_EXEC' not in os.environ:
    executable = 'displaycluster'
else:
    executable = os.environ['DISPLAYCLUSTER_EXEC']

startCommand = install_dir + "/bin/" + executable
    
if myRank == 0:
    subprocess.call(shlex.split(startCommand))
else:
    # configuration.json gives the display
    display = None

    try:
        configPath = os.environ.get('DISPLAYCLUSTER_CONFIG', dcPath + '/configuration.json')

        with open(configPath) as f:
            config = json.load(f)

        processes = config['processes']

        if len(processes) < myRank:
            print( 'could not find process entry for rank ' + str(myRank))
            exit(-5)

        process = processes[myRank - 1]

        display = process.get('display')

        if display != None:
            os.environ['DISPLAY'] = display
        else:
            os.environ['DISPLAY'] = ':0'
    except:
        print( 'Error processing configuration.json. Make sure you have created a configuration.json and put it in ' + dcPath + '/. An example is provided in the examples/ directory.')
        exit(-6)

    subprocess.call(shlex.split(startCommand))
