FROM debian:jessie
RUN apt-get -y update && apt-get install -y libbz2-dev liblzo2-dev lzop bzip2 \
	libtar-dev libcurl4-openssl-dev realpath checkinstall libffcall1-dev \
	build-essential valgrind git tree strace ltrace automake iputils-ping \
	libtool vim wget
RUN apt-get install -y libarchive-dev
RUN apt-get install -y moreutils
RUN apt-get install -y python
RUN apt-get install -y doxygen
RUN apt-get install -y graphviz
RUN apt-get install -y lsb-release
RUN apt-get install -y pkg-config
RUN apt-get install -y zlib1g-dev
RUN mkdir -p /workspace/
WORKDIR /workspace/
