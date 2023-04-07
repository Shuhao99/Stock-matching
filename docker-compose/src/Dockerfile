FROM ubuntu:20.04
# ENV DEBIAN_FRONTEND noninteractive
RUN ln -sf /usr/share/zoneinfo/UTC /etc/localtime
RUN apt-get update
RUN apt-get install -y g++ make
RUN apt-get update && apt-get install -y libpqxx-dev
RUN mkdir /code
WORKDIR /code
ADD . /code/