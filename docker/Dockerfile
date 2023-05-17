FROM dc-base1

ENV HOME /root
SHELL ["/bin/bash", "-c"]

RUN apt-get update
RUN apt-get install -y python2-dev

COPY displaycluster-recipe displaycluster-recipe
RUN sh displaycluster-recipe
