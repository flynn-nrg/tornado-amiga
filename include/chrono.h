
#ifndef _CHRONO_H_
#define  _CHRONO_H_

typedef unsigned int t_ccap; // chrono capture

float chrono_get_cpu_freq ();

void chrono_reset ();

t_ccap chrono_get ();

float chrono_ccap2ms (t_ccap t);
unsigned int chrono_ccap2cycles (t_ccap t);


#endif  // _CHRONO_H_
