docker run -it -v /tmp/.X11-unix:/tmp/.X11-unix -e DISPLAY=$DISPLAY -h $HOSTNAME -v $HOME/.Xauthority:/home/lyonn/.Xauthority gregabram/displaycluster



# docker run -it -e"DISPLAY=host.docker.internal:0" -v /tmp/.X11-unix:/tmp/.X11-unix:rw  gregabram/displaycluster
