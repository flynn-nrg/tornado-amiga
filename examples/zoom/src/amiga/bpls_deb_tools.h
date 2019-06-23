

static void fill_bpls_v(unsigned char *bpls, int nplanes, int w, int h) {
  int x, y, b;
  int planesz = (w >> 3) * h;
  for (y = 0; y < h; y++) {
    unsigned char *line = bpls + ((w >> 3) * y);
    for (x = 0; x < (w >> 3); x++) {
      for (b = 0; b < nplanes; b++) {
        line[x + (b * planesz)] = (((y >> b) & 1) == 0) ? 0 : 0xff;
      }
    }
  }
}

/*
static void fill_bpls_h (unsigned char* bpls, int nplanes, int w, int h)
{
    int x, y, b;
    int planesz = (w >> 3) * h;
    for (y=0;y<h;y++)
    {
        unsigned char* line = bpls + ((w >> 3) * y);
        for (x=0;x<(w >> 3);x++)
        {
            for (b=0;b<nplanes;b++)
            {
                line[x + (b * planesz)] = (((y>>b) & 1) == 0) ? 0 : 0xff;
            }
        }
    }
}
*/
