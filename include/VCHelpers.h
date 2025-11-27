#include "VCParser.h"

//Helper functions for the parser
char *readAndCombineLines(FILE *file, VCardErrorCode *error);

//Helper functions to validate the card and it's different components
VCardErrorCode validateDateTime(const DateTime *dt);
bool isValidPropertyName(const char *name);
void printAscii(const char *str);
void hexDump(const char *str);
