#ifndef MAIN_H
#define MAIN_H

// manager config options
#define OPTFLAGS_PLAYMUSIC 2


#define AUTO_BUTTON_REP(v, b) if(v && (old_pad & b)) { \
                                 v++; \
                                 if(v > 20) {v = 0; new_pad |= b;} \
                                 } else v = 0;


#endif
