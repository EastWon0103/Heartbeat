FROM ubuntu:22.04

ARG DEBIAN_FRONTEND=noninteractive
ENV TZ=Asia/Seoul

RUN apt-get update
RUN apt-get install -y tzdata
RUN apt-get install -y gcc make
RUN apt-get install -y curl
RUN apt-get install -y openjdk-17-jdk

EXPOSE 22

CMD ["ls"]