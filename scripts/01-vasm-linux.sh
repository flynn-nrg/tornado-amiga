#!/bin/bash

( cd vasm && make clean && make CPU=m68k SYNTAX=mot )
mv vasm/{vasmm68k_mot,vobjdump} bin/
( cd vasm && rm obj/* && make clean )

