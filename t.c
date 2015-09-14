#include <stdio.h>

char line[1024];  /* Generously large value for most situations */

char *eof;

line[0] = '\0'; /* Ensure empty line if no input delivered */
line[sizeof(line)-1] = ~'\0';  /* Ensure no false-null at end of buffer */

eof = fgets(line, sizeof(line), stdin);
