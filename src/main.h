#ifndef MAIN_H
#define MAIN_H


#ifdef CEEDLING
#   define LOOP_FOREVER (0)
int app_main (void);
void on_wdt (void);
#else
#   define LOOP_FOREVER (1)
#endif

#endif // MAIN_H
