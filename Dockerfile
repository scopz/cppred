FROM ubuntu:22.04

RUN apt-get update
RUN apt-get install -y build-essential cmake
RUN apt-get install -y libboost-all-dev libsdl2-dev
RUN apt-get clean && apt-get autoclean

ENV PATH="/src:${PATH}"
VOLUME /src
WORKDIR /src

CMD ["./build_unix.sh"]