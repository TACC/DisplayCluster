FROM dc-base0

ENV HOME /root
SHELL ["/bin/bash", "-c"]

COPY wrapper wrapper
COPY base1-recipe base1-recipe
RUN sh wrapper
