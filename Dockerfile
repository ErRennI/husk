FROM ubuntu:24.04 AS build

RUN apt-get update &&\
    apt-get install -y --no-install-recommends build-essential valgrind &&\
    rm -rf /var/lib/apt/lists/*

WORKDIR /husk

COPY . .

RUN make

RUN make test

FROM ubuntu:24.04

COPY --from=build /husk/build/husk /usr/local/bin/husk

CMD [ "husk" ]
