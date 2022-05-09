# MPEG-TS SCTE-35 inserter

A tool for inserting SCTE-35 splice in/out commands into an MPEG-TS stream for test purposes. Input is a unicast or multicast MPEG-TS stream. Output is a new MPEG-TS stream or a .ts file.

## Getting started

Requires gstreamer 1.20. Packaged and run as a docker container to simplify usage.

### Build container (uses multi-stage builds):

```
docker build -t scte35-inserter:dev .
```

### Usage

```
docker run --rm scte35-inserter:dev -i <MPEG-TS input address:port> -o [MPEG-TS output address:port] -n <SCTE-35 splice interval s> -d <SCTE-35 splice duration s> [--immediate] [--autoreturn] --file [output file name (instead of UDP output)]
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
