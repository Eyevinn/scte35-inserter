# MPEG-TS SCTE-35 insert

A tool for insert SCTE-35 splice in/out commands into an MPEG-TS stream for test purposes.

## Getting started

Supported platforms are Ubuntu 21.10 and OSX.

### Ubuntu 21.10

#### Install dependencies

```
apt-get install libgstreamer1.0-0 gstreamer1.0-plugins-bad gstreamer1.0-plugins-good cmake gcc g++ make gdb libglib2.0-dev libgstreamer1.0-dev libgstreamer-plugins-bad1.0-dev
```

#### Build

```
cmake -DCMAKE_BUILD_TYPE=Release -G "Unix Makefiles" .
make
```

#### Run
```
./scte35-inserter -i <MPEG-TS input address:port> -o [MPEG-TS output address:port] -n <SCTE-35 splice interval s> -d <SCTE-35 splice duration s> [--immediate] --file [output file name (instead of UDP output)]
```

### OSX

#### Requirements

XCode command line tools installed.

Install additional dependencies using homebrew:
```
brew install gstreamer gst-plugins-good gst-plugins-bad cmake
```

#### Build

```
cmake -DCMAKE_BUILD_TYPE=Release -G "Unix Makefiles" .
make
```

#### Run
```
export GST_PLUGIN_PATH=/usr/local/lib/gstreamer-1.0/
./scte35-inserter -i <MPEG-TS input address:port> -o [MPEG-TS output address:port] -n <SCTE-35 splice interval s> -d <SCTE-35 splice duration s> [--immediate] --file [output file name (instead of UDP output)]
```

### Docker Container

Build container (uses multi-stage builds):

```
docker build -t scte35-inserter:dev .
```

Run container (example):

```
docker run --rm scte35-inserter:dev -i <MPEG-TS input address:port> -o [MPEG-TS output address:port] -n <SCTE-35 splice interval s> -d <SCTE-35 splice duration s> [--immediate] --file [output file name (instead of UDP output)]
```

## License (Apache-2.0)

```
Copyright 2022 Eyevinn Technology AB

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
```

## About Eyevinn Technology

Eyevinn Technology is an independent consultant firm specialized in video and streaming. Independent in a way that we are not commercially tied to any platform or technology vendor.

At Eyevinn, every software developer consultant has a dedicated budget reserved for open source development and contribution to the open source community. This give us room for innovation, team building and personal competence development. And also gives us as a company a way to contribute back to the open source community.

Want to know more about Eyevinn and how it is to work here. Contact us at work@eyevinn.se!
