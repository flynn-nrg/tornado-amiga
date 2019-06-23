#ifndef PROF_H
#define PROF_H

void prof_enabled(int);
void prof_reset();
void prof_reset_chrono();
float prof_get_time(const char *, int);
void prof_show_times(t_canvas *, unsigned int debug_color, int y);

#endif // PROF_H
