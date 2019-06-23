#!/bin/sh

/tmp/track2h -s 0.0001 -t sync_zoom.deltaX.track -h src/zoom/zoom_x.h
/tmp/track2h -s 0.0001 -t sync_zoom.deltaY.track -h src/zoom/zoom_y.h
/tmp/track2h -t sync_zoom.z.track -h src/zoom/zoom_z.h

