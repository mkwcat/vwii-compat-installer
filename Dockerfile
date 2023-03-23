FROM wiiuenv/devkitppc:20220917 AS final

COPY --from=wiiuenv/libmocha:20220919 /artifacts $DEVKITPRO

WORKDIR /project
