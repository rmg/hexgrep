FROM buildpack-deps:xenial

ADD main.c /
RUN cc -static -O3 -o scan main.c && ls -al

FROM scratch

COPY --from=0 scan /

ENTRYPOINT ["/scan"]

