#!/bin/bash

( cd vbcc && mkdir -p bin && make TARGET=m68k && make TARGET=m68ks )
mv vbcc/bin/{vbcc*,vc,vprof} bin/
( cd vbcc && rm bin/* && rmdir bin && rm machines/{m68k,m68ks}/*.o )

