# DisplayCluster

DisplayCluster is a software environment for interactively driving large-scale tiled displays. The software allows users to interactively view media such as high-resolution imagery and video, as well as stream content from remote sources such as laptops / desktops or high-performance remote visualization machines. Many users can simultaneously interact with DisplayCluster with devices such as joysticks or touch-enabled devices such as the iPhone / iPad / iTouch or Android devices. Additionally, a Python scripting interface is provided to automate interaction with DisplayCluster.

DisplayCluster is hosted on GitHub at https://github.com/TACC/DisplayCluster. The recommended way to download DisplayCluster is via Git. This will allow you to easily pull updates to the code as they are made. You can download the code with Git by executing the command:

git clone git://github.com/TACC/DisplayCluster.git

You may also download specific versions and packages at the Tags and Downloads link on the GitHub page.

The DisplayCluster manual is included in the distribution in the doc/ directory, and covers installation and usage.

# Containers

DisplayCluster can be run using containers.  This is primarily intended for use with Stallion/Rattler, and is built on top of the standard image for Stallion/Rattler images.   This is build from the https://github.com/GregAbram/TACC_ClusterBase project, and is available from the docker hub at docker://gregabram/tacc_clusterbase.   For non-Linux hosts your mileage will vary.

You can access the docker container at docker://gregabram/displaycluster. To run it under Apptainer on an appropriate Linux cluster you will need to pull your own  .sif file: on the destination host:

apptainer pull docker://gregabram/displaycluster

THis will create displaycluster_latest.sif, which you can run under apptainer

To create your own image, you can check out the docker branch.   In the docker folder you'll see the Dockerfile and the recipe for building all the dependencies and DC itself.   To use a different base image, change the FROM line; you can look at https://github.com/GregAbram/TACC_ClusterBase to see what your base image will need.



