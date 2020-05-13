FROM gitpod/workspace-full

USER gitpod

RUN sudo apt-get -q update && \
     sudo apt-get install -yq libnl-3-dev libnl-genl-3-dev && \
     sudo rm -rf /var/lib/apt/lists/*

# More information: https://www.gitpod.io/docs/config-docker/
