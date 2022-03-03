FROM wiiuenv/devkitppc:20211229

COPY --from=wiiuenv/libiosuhax:latest /artifacts $DEVKITPRO

WORKDIR .
