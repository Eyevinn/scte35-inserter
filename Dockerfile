# syntax=docker/dockerfile:1
FROM debian:bookworm
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update
RUN apt-get -y install libgstreamer1.0-0 gstreamer1.0-plugins-bad gstreamer1.0-plugins-good cmake gcc g++ make gdb libglib2.0-dev libgstreamer1.0-dev libgstreamer-plugins-bad1.0-dev

WORKDIR /src
ADD ./ /src

RUN cmake -DCMAKE_BUILD_TYPE=Release -G "Unix Makefiles" .
RUN make

FROM debian:bookworm
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update
RUN apt-get -y install libgstreamer1.0-0 gstreamer1.0-plugins-bad gstreamer1.0-plugins-good gstreamer1.0-tools

WORKDIR /app
COPY --from=0 /src/scte35-inserter ./scte35-inserter

ENTRYPOINT ["./scte35-inserter"]
