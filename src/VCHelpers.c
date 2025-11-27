#include "VCHelpers.h"
#include "VCParser.h"

// This function reads one "logical" line from the file.
// The function uses a static buffer ("leftoverLine") to hold a line that was read
// but not processed because it starts a new logical line.
char *readAndCombineLines(FILE *file, VCardErrorCode *error)
{
    static char leftoverLine[1024] = ""; // Holds a line that was read but not processed
    char lineFromFile[1024];             // Buffer to store a line from the file
    char *currentLine = NULL;            // The combined line we will build
    size_t currentLineLength = 0;        // Current length of the combined line

    // If we have a leftoverLine line from the previous call, use it first.
    if (strlen(leftoverLine) > 0)
    {
        currentLineLength = strlen(leftoverLine);
        currentLine = malloc(currentLineLength + 1);
        if (currentLine == NULL)
        {
            return NULL;
        }
        strcpy(currentLine, leftoverLine);
        leftoverLine[0] = '\0'; // Clear leftoverLine so it's not used again
    }

    // Read each line from the file.
    while (fgets(lineFromFile, sizeof(lineFromFile), file))
    {
        size_t physLen = strlen(lineFromFile);
        //printf("Read line from file: %s\n", lineFromFile);
        //printf("Length of line from file: %ld\n", strlen(lineFromFile));

        // Check for valid CRLF (\r\n) line endings
        if (physLen >= 2 && lineFromFile[physLen - 2] == '\r' && lineFromFile[physLen - 1] == '\n') {
            lineFromFile[physLen - 2] = '\0'; // Remove CR and LF
        } else if (physLen >= 1 && lineFromFile[physLen - 1] == '\n') {
            *error = INV_CARD; // LF-only line endings are invalid
            free(currentLine);
            return NULL;
        } else {
            *error = INV_CARD; // No valid line ending
            free(currentLine);
            return NULL;
        }

        // printf("Read physical line: '%s'\n", lineFromFile);

        // Check if the line is a continuation:
        if (lineFromFile[0] == ' ' || lineFromFile[0] == '\t')
        {
            //  We want to remove the first character and append the rest to currentLine.
            size_t extra = physLen > 0 ? physLen - 1 : 0; // Extra length without the first character

            // If no logical line exists yet, start one (this is unexpected but we handle it)
            if (currentLine == NULL)
            {
                currentLine = malloc(extra + 1);
                if (currentLine == NULL)
                {
                    return NULL;
                }
                strcpy(currentLine, lineFromFile + 1);
                currentLineLength = extra;
            }
            else
            {
                // Increase the memory for currentLine and append the new part.
                char *temp = realloc(currentLine, currentLineLength + extra + 1);
                if (temp == NULL)
                {
                    free(currentLine);
                    return NULL;
                }
                currentLine = temp;
                strcat(currentLine, lineFromFile + 1); // Append after skipping the first character
                currentLineLength += extra;
            }
        }
        else
        {
            // If the physical line is NOT a continuation, then we have a new logical line.
            // But if we already built a logical line, we must return it now.
            if (currentLine != NULL)
            {
                // Save the new physical line into 'leftoverLine' for the next call.
                strcpy(leftoverLine, lineFromFile);
                return currentLine;
            }
            else
            {
                // No logical line built yet: start one with the current physical line.
                currentLine = malloc(physLen + 1);
                if (currentLine == NULL)
                {
                    return NULL;
                }
                strcpy(currentLine, lineFromFile);
                currentLineLength = physLen;
                // Continue the loop to check if the next physical line is a continuation.
            }
        }
    }
    return currentLine;
}

// Function to check if a property name is valid (Sections 6.1 - 6.9.3)
/*
@param name - the property name to check
@return true if the property name is valid, false otherwise
*/
bool isValidPropertyName(const char *name)
{
    const char *validProps[] = {
        "FN", "N", "BDAY", "ANNIVERSARY", "GENDER", "LANG", "ORG",
        "ADR", "TEL", "EMAIL", "GEO", "KEY", "TZ", "URL"
    };
    int validCount = sizeof(validProps) / sizeof(validProps[0]); // Number of valid properties is the size of the array

    for (int i = 0; i < validCount; i++)
    {
        if (strcmp(name, validProps[i]) == 0)
        {
            return true;
        }
    }
    return false;
}

/*
@param prop - the property to validate
@return the error code if the property is invalid, OK otherwise
*/
// Function to validate a DateTime object
VCardErrorCode validateDateTime(const DateTime *dt)
{
    if (dt == NULL)
    {
        return INV_DT;
    }

    if (dt->isText)
    {
        // For text DateTime, date and time must be empty and UTC must be false
        if (strlen(dt->date) > 0 || strlen(dt->time) > 0)
        {
            return INV_DT;
        }
        if (dt->UTC) // Text DateTime cannot be UTC
        {
            return INV_DT;
        }
    }
    else
    {
        // For non-text DateTime, the text field must be empty.
        if (strlen(dt->text) > 0)
        {
            return INV_DT;
        }
        // (You could also validate dt->date and dt->time formats here, if required.)
    }
    return OK;
}

void printAscii(const char *str) {
    if (str == NULL) {
        printf("(null)\n");
        return;
    }
    int i = 0; 
    printf("The first character in the passed string is: %c\n",str[i]);
    while (i < strlen(str)){
        printf("%c(%d) ", str[i], (int)str[i]);
        i++;
        printf("\n");
    }
}

void hexDump(const char *str) {
    if (str == NULL) {
        printf("(null)\n");
        return;
    }
    for (size_t i = 0; i < strlen(str); i++) {
        printf("%02x ", (unsigned char)str[i]);
    }
    printf("\n");
}