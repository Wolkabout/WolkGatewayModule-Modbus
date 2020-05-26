FROM raspbian/stretch:latest

WORKDIR /build

RUN apt update
RUN apt-get install -y git gcc g++ cmake automake autotools-dev autoconf m4 zlib1g-dev cmake libtool libssl-dev devscripts debhelper

COPY make_deb.sh /build

#CMD ["bash", "make_deb.sh"]