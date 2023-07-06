FROM ghcr.io/wiiu-env/devkitppc:20230402

COPY --from=ghcr.io/wiiu-env/libmocha:20230417 /artifacts $DEVKITPRO

RUN git config --global --add safe.directory /project

WORKDIR /project
