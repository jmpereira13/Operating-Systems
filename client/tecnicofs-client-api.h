#ifndef API_H
#define API_H

#include "tecnicofs-api-constants.h"

/* send the CREATE command to server and receiving it's answer */
int tfsCreate(char *path, char nodeType);
/* send the DELETE command to server and receiving it's answer */
int tfsDelete(char *path);
/* send the LOOKUP command to server and receiving it's answer */
int tfsLookup(char *path);
/* send the MOVE command to server and receiving it's answer */
int tfsMove(char *from, char *to);
/* send the PRINT command to server and receiving it's answer */
int tfsPrint(char *path);
/* creating socket ( client ), binding, and setting socket adresses */
int tfsMount(char* serverName);
/* closing socket (end of communication) */
int tfsUnmount();

#endif /* CLIENT_H */
