FROM ubuntu:precise
RUN apt-get -y update && apt-get install -y libbz2-dev liblzo2-dev lzop bzip2 \
	libtar-dev libcurl4-openssl-dev realpath checkinstall libffcall1-dev \
	build-essential valgrind git tree strace ltrace automake iputils-ping \
	libtool vim wget
RUN mkdir -p /workspace/
WORKDIR /workspace/
