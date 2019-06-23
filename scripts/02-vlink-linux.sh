#!/bin/bash

( cd vlink && mkdir -p objects && make )
mv vlink/vlink bin/
( cd vlink && rm objects/* && rmdir objects )

