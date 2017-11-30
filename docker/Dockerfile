FROM ubuntu:14.04

RUN apt-get update && apt-get install -y sudo && rm -rf /var/lib/apt/lists/*

RUN useradd --create-home --shell /bin/bash docker
RUN echo 'docker ALL=(ALL) NOPASSWD: ALL' >> /etc/sudoers

WORKDIR /home/docker/
USER docker:docker


RUN sudo apt-get update && sudo apt-get install -y \
    wget \
    build-essential \
    git \
    libfltk1.3-dev \
    libfltk1.3 \
    libblitz0-dev \
    libblitz0ldbl \
    libgsl0-dev \
    libccfits-dev \
    libboost-all-dev \
    libx11-dev \
    libxft-dev \
    subversion


RUN wget -O fltk-1.3.0-source.tar.gz http://fltk.org/pub/fltk/1.3.0/fltk-1.3.0-source.tar.gz
RUN tar -xzvf fltk-1.3.0-source.tar.gz
WORKDIR fltk-1.3.0
RUN ./configure
RUN make -j9
RUN sudo make install


WORKDIR /home/docker/

RUN git clone https://github.com/proyZ/FL_Flews.git
RUN ln -s FL_Flews flews-0.3.1
WORKDIR flews-0.3.1
RUN ./configure
RUN make -j9


WORKDIR /home/docker/


RUN wget -O blitz-0.10.tar.gz https://astuteinternet.dl.sourceforge.net/project/blitz/blitz/Blitz%2B%2B%200.10/blitz-0.10.tar.gz
RUN tar -xzvf blitz-0.10.tar.gz
WORKDIR blitz-0.10
COPY blitz-patch.patch .
RUN patch -p1 < blitz-patch.patch
RUN ./configure --disable-shared
RUN make -j9 lib
RUN make -j9 check-testsuite  # (see if all tests pass)
RUN sudo make install  # (you can ignore errors pertaining to .pdf files and .tex files)


WORKDIR /home/docker/


RUN wget ftp://ftp.gnu.org/gnu/gsl/gsl-2.4.tar.gz
RUN tar -xzvf gsl-2.4.tar.gz
WORKDIR gsl-2.4
RUN ./configure --disable-shared
RUN make -j9
RUN make -j9 check  # (see if all tests pass)
RUN sudo make install


WORKDIR /home/docker/


RUN wget -O cfitsio3420.tar.gz http://heasarc.gsfc.nasa.gov/FTP/software/fitsio/c/cfitsio3420.tar.gz
RUN tar -xzvf cfitsio3420.tar.gz
WORKDIR cfitsio
RUN ./configure
RUN make -j9
RUN sudo make install


WORKDIR /home/docker/

COPY . /home/docker/viewpoints
WORKDIR /home/docker/viewpoints

RUN sudo make

ENTRYPOINT ["./vp"]

CMD ["inline_skating.csv"]
