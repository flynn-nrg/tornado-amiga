#!/bin/sh

/tmp/track2h -t sync_blur.blur1.track -h src/blur/blur_blur1.h
/tmp/track2h -t sync_blur.blur2.track -h src/blur/blur_blur2.h
/tmp/track2h -t sync_blur.blur3.track -h src/blur/blur_blur3.h

/tmp/track2h -i -t sync_blur.inst1.track -h src/blur/blur_inst1.h
/tmp/track2h -i -t sync_blur.inst2.track -h src/blur/blur_inst2.h
/tmp/track2h -i -t sync_blur.inst3.track -h src/blur/blur_inst3.h

/tmp/track2h -t sync_blur.x1.track -h src/blur/blur_x1.h
/tmp/track2h -t sync_blur.x2.track -h src/blur/blur_x2.h
/tmp/track2h -t sync_blur.x3.track -h src/blur/blur_x3.h

/tmp/track2h -t sync_blur.y1.track -h src/blur/blur_y1.h
/tmp/track2h -t sync_blur.y2.track -h src/blur/blur_y2.h
/tmp/track2h -t sync_blur.y3.track -h src/blur/blur_y3.h
