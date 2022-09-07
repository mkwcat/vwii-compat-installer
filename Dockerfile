FROM wiiuenv/devkitppc:20220806 AS final

RUN git clone --recursive https://github.com/wiiu-env/libmocha --single-branch && \
 cd libmocha && \
 make -j$(nproc) && \
 make install && \
 cd .. && \
 rm -rf libmocha

WORKDIR /project
