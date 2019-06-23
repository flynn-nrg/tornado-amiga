#ifndef _DPRINT_H_
#define _DPRINT_H_

// x, y in tile coords
void dprint_locate(int wX, int wY);
void dprint_at(t_canvas *s, const char *psCadena, int x, int y, int nColor);
void dprint_color(int wColor);
void dprint(t_canvas *s, const char *pszFormat, ...);

void dprint_mode(int wMode);

#define PRINTMODE_PLAIN (0)
#define PRINTMODE_SHADOW (1)
#define PRINTMODE_INVALID (2)

#endif // _DPRINT_H_
