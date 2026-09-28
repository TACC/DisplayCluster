docker build -f docker/Dockerfile -t displaycluster .
docker save displaycluster:latest -o /tmp/displaycluster.tar
apptainer build displaycluster.sif docker-archive:/tmp/displaycluster.tar
