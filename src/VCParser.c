#include <stdio.h>
#include "VCParser.h"
#include "VCHelpers.h"

// ************* Card parser functions - MUST be implemented ***************
VCardErrorCode createCard(char *fileName, Card **obj)
{
    if (fileName == NULL || obj == NULL)
    {
        return INV_FILE; // Invalid input
    }

    // Check file extension
    const char *dot = strrchr(fileName, '.'); // Find last occurrence of '.'
    if (!dot || (strcmp(dot, ".vcf") != 0 && strcmp(dot, ".vcard") != 0))
    {
        *obj = NULL;
        return INV_FILE; // Reject if extension is not .vcf or .vcard
    }

    FILE *file = fopen(fileName, "r");
    if (file == NULL)
    {
        return INV_FILE; // File could not be opened
    }

    // Allocate memory for the Card structure
    *obj = malloc(sizeof(Card));

    // Allocate memory for the FN property
    (*obj)->fn = malloc(sizeof(Property));
    if ((*obj)->fn == NULL)
    {
        free(*obj);
        fclose(file);
        deleteCard(*obj);
        return OTHER_ERROR;
    }

    // Initialize the FN property
    (*obj)->fn->name = NULL;
    (*obj)->fn->group = NULL;
    (*obj)->fn->parameters = initializeList(&parameterToString, &deleteParameter, &compareParameters);
    (*obj)->fn->values = initializeList(&valueToString, &deleteValue, &compareValues);

    // Initialize space for other properties
    (*obj)->optionalProperties = initializeList(&propertyToString, &deleteProperty, &compareProperties);
    (*obj)->birthday = NULL;
    (*obj)->anniversary = NULL;

    // Check if memory allocation failed
    if ((*obj)->fn->values == NULL)
    {
        free((*obj)->fn);
        free(*obj);
        fclose(file);
        deleteCard(*obj);
        return OTHER_ERROR;
    }

    // Initialize values and memory for line size, the current property, and the fn name
    // char line[256];
    Property *currentProperty = malloc(sizeof(Property));
    (*obj)->fn->name = malloc(30);  // Allocate memory for the FN property
    (*obj)->fn->group = malloc(30); // Allocate memory for the group

    // Check if memory allocation failed
    if (currentProperty == NULL || (*obj)->fn->name == NULL)
    {
        free(currentProperty);
        deleteCard(*obj);
        fclose(file);
        return OTHER_ERROR;
    }
    currentProperty->name = NULL;  // Initialize the name of the current property to NULL
    currentProperty->group = NULL; // Initialize the group of the current property to NULL

    // Flag to check if there are begin and end tags
    bool beginTag = false;
    bool endTag = false;
    bool versionTag = false;
    bool fnTag = false;

    // Read the file line by line
    char *line = NULL;
    VCardErrorCode error = OK;

    // Loop to read each line from the file
    while ((line = readAndCombineLines(file, &error)) != NULL)
    {

        if (error == INV_CARD)
        {
            free(line);
            fclose(file);
            deleteCard(*obj);
            *obj = NULL;
            return INV_CARD;
        }

        // Make sure the begin property is present
        if (strcmp(line, "BEGIN:VCARD") == 0)
        { // Skip these lines
            beginTag = true;
            free(line);
            continue;
        }
        // Make sure the version is 4.0
        if (strcmp(line, "VERSION:4.0") == 0)
        { // Skip these lines
            versionTag = true;
            free(line);
            continue;
        }
        // Make sure we have an end property
        if (strcmp(line, "END:VCARD") == 0)
        { // Skip this line
            endTag = true;
            free(line);
            continue;
        }
        else if (strchr(line, ':') == NULL)
        {
            free(line);
            fclose(file);
            deleteCard(*obj);
            *obj = NULL;
            return INV_PROP;
        }
        // Make sure the line has a value before the colon
        if (line[0] == ':' || line[0] == ';')
        {
            free(line);
            fclose(file);
            free(currentProperty->name);
            free(currentProperty);
            deleteCard(*obj);
            *obj = NULL;
            return INV_PROP; // Missing property name
        }
        if (currentProperty->name != NULL)
        { // Free the name of the current property if it is not NULL
            free(currentProperty->name);
        }
        currentProperty->name = calloc(10000, sizeof(char)); // Allocate memory for the property name
        // Remove newline characters
        line[strcspn(line, "\r\n")] = 0;

        // Go through each character in the line, we start copying into the name of the property
        for (int i = 0; i < strlen(line); i++)
        {
            if (line[i] == ':' || line[i] == ';')
            { // If we hit a colon, then we want to stop copying the name and start copying into the value
                if (currentProperty->name == NULL)
                {
                    free(currentProperty->name);
                    deleteCard(*obj);
                    fclose(file);
                    return INV_PROP;
                }
                if (strcmp(currentProperty->name, "FN") == 0)
                { // If the property is FN
                    fnTag = true;
                    strcpy((*obj)->fn->name, currentProperty->name); // Set the name of the FN property
                    i++;                                             // Skip the colon
                    int j = 0;
                    char *fnValue = calloc(strlen(line) + 1, sizeof(char)); // Allocate memory for FN value

                    // Check if memory allocation failed
                    if (fnValue == NULL)
                    {
                        free(fnValue);
                        deleteCard(*obj);
                        return OTHER_ERROR;
                    }
                    while (line[i] != '\0')
                    { // Copy the value into the value string
                        fnValue[j] = line[i];
                        i++;
                        j++;
                    }
                    // **Check if no value was copied:**
                    if (j == 0)
                    {
                        free(fnValue);
                        deleteCard(*obj);
                        fclose(file);
                        *obj = NULL;
                        return INV_PROP; // Missing property value
                    }
                    insertBack((*obj)->fn->values, fnValue); // Insert the value into the values list
                    // Check if insertion failed
                    if ((*obj)->fn->values == NULL)
                    {
                        free(fnValue);
                        deleteCard(*obj);
                        return INV_PROP;
                    }
                    if ((*obj)->fn->name == NULL)
                    {
                        deleteCard(*obj);
                        return INV_PROP;
                    }
                    break;
                }
                // If the property is not FN, we copy into the FN propety values
                else
                {
                    // Allocate memory for new properties
                    // Key is to do this in here and initialize! Narrowest scope, and we need to initialize them if we want to free them,
                    // even if we don't use them. We need to do it so we can properly free them later.
                    Property *newProperty = malloc(sizeof(Property)); // Allocate memory for the new property
                    if (newProperty == NULL)
                    {
                        deleteCard(*obj);
                        fclose(file);
                        return OTHER_ERROR;
                    }
                    // We need to initialize the new property, and all of its values, even if we don't use them yet
                    newProperty->group = NULL; // Initialize the group of the new property to NULL
                    // Check if currentProperty->group is not NULL before copying
                    if (currentProperty->group != NULL)
                    {
                        newProperty->group = malloc(strlen(currentProperty->group) + 1); // +1 for null terminator
                        if (newProperty->group == NULL)
                        {
                            deleteCard(*obj);
                            fclose(file);
                            return OTHER_ERROR;
                        }
                        strcpy(newProperty->group, currentProperty->group);
                    }
                    else
                    {
                        // If the current property group is null, then we just put an empty string in the new property group
                        newProperty->group = calloc(1, sizeof(char));
                        if (newProperty->group == NULL)
                        {
                            deleteCard(*obj);
                            fclose(file);
                            return OTHER_ERROR;
                        }
                        strcpy(newProperty->group, "");
                    }

                    newProperty->parameters = initializeList(&parameterToString, &deleteParameter, &compareParameters); // Initialize the parameters list

                    // Initialize the new property lists and values
                    newProperty->name = calloc(30, sizeof(char));                                       // Allocate memory for the name
                    newProperty->values = initializeList(&valueToString, &deleteValue, &compareValues); // Initialize the values list

                    // Copy the name of the current property into the new property
                    strcpy(newProperty->name, currentProperty->name);

                    // If we hit a semicolon, we want to start parsing the parameters
                    if (line[i] == ';')
                    {        // Start parsing parameters
                        i++; // Skip the semicolon
                        while (line[i] != ':' && line[i] != '\0')
                        { // Continue until ':' or end of line
                            Parameter *newParameter = malloc(sizeof(Parameter));
                            if (newParameter == NULL)
                            {
                                deleteCard(*obj);
                                fclose(file);
                                return OTHER_ERROR;
                            }

                            // Allocate memory for parameter name and value
                            newParameter->name = calloc(30, sizeof(char));
                            newParameter->value = calloc(30, sizeof(char));
                            if (newParameter->name == NULL || newParameter->value == NULL)
                            {
                                free(newParameter->name);
                                free(newParameter->value);
                                free(newParameter);
                                deleteCard(*obj);
                                fclose(file);
                                return OTHER_ERROR; // Memory allocation failed
                            }

                            // Extract parameter name
                            int j = 0;
                            while (line[i] != '=' && line[i] != ':' && line[i] != '\0')
                            {
                                newParameter->name[j++] = line[i++];
                            }
                            newParameter->name[j] = '\0'; // Null-terminate the name

                            // Validate parameter name
                            if (j == 0 || line[i] != '=')
                            { // No name or missing '='
                                free(newParameter->name);
                                free(newParameter->value);
                                free(newParameter);
                                deleteCard(*obj);
                                fclose(file);
                                return INV_PROP;
                            }

                            if (line[i] == '=')
                            {
                                i++; // Skip the '='
                                // Extract parameter value
                                j = 0;
                                while (line[i] != ';' && line[i] != ':' && line[i] != '\0')
                                {
                                    newParameter->value[j++] = line[i++];
                                }
                                newParameter->value[j] = '\0'; // Null-terminate the value
                            }
                            else
                            {
                                // If there's no '=', treat it as an empty parameter
                                newParameter->value[0] = '\0';
                            }
                            // **Add check for empty parameter value:**
                            if (strlen(newParameter->value) == 0) {
                                free(newParameter->name);
                                free(newParameter->value);
                                free(newParameter);
                                deleteCard(*obj);
                                fclose(file);
                                *obj = NULL;
                                return INV_PROP;  // Parameter value is missing/empty
                            }

                            if (newParameter->name == NULL || newParameter->value == NULL)
                            {
                                deleteCard(*obj);
                                return INV_PROP;
                            }

                            // Add the parameter to the parameters list
                            insertBack(newProperty->parameters, newParameter);

                            if (line[i] == ';')
                            {
                                i++; // Skip the semicolon to move to the next parameter
                            }
                        }
                    }

                    if (strcmp(currentProperty->name, "BDAY") == 0 || strcmp(currentProperty->name, "ANNIVERSARY") == 0)
                    {
                        // Allocate memory for DateTime structure
                        DateTime *dateTime = malloc(sizeof(DateTime));
                        if (dateTime == NULL)
                        {
                            free(currentProperty->name);
                            deleteCard(*obj);
                            fclose(file);
                            return OTHER_ERROR;
                        }

                        // Initialize DateTime fields
                        dateTime->UTC = false;
                        dateTime->isText = false;
                        dateTime->date = calloc(11, sizeof(char)); // +2 extra space
                        dateTime->time = calloc(9, sizeof(char));  // +2 extra space
                        dateTime->text = calloc(strlen(line) + 2, sizeof(char));

                        // Check if memory allocation failed
                        if (dateTime->date == NULL || dateTime->time == NULL || dateTime->text == NULL)
                        {
                            free(dateTime->date);
                            free(dateTime->time);
                            free(dateTime->text);
                            free(dateTime);
                            free(currentProperty->name);
                            deleteCard(*obj);
                            fclose(file);
                            return OTHER_ERROR;
                        }

                        // Extract value from the line (everything after the colon)
                        char *value = strchr(line, ':') + 1;
                        if (value == NULL)
                        {
                            free(dateTime->date);
                            free(dateTime->time);
                            free(dateTime->text);
                            free(dateTime);
                            free(currentProperty->name);
                            deleteCard(*obj);
                            fclose(file);
                            return INV_PROP;
                        }

                        // Check if VALUE=text is explicitly set in parameters
                        bool isTextValue = false;
                        ListIterator paramIter = createIterator(newProperty->parameters);
                        Parameter *param;
                        while ((param = nextElement(&paramIter)) != NULL)
                        {
                            if (strcmp(param->name, "VALUE") == 0 && strcmp(param->value, "text") == 0)
                            {
                                isTextValue = true;
                                break;
                            }
                        }

                        // Properly parse DATE, TIME, or both
                        if (isTextValue)
                        {
                            // Explicitly marked as text-based date
                            dateTime->isText = true;
                            strncpy(dateTime->text, value, strlen(value) + 1);
                        }
                        else if (strchr(value, 'T') != NULL)
                        {
                            // If 'T' is found, split into date and time
                            char *timePos = strchr(value, 'T'); // Find 'T' position
                            size_t dateLen = timePos - value;   // Length of date part

                            strncpy(dateTime->date, value, dateLen); // Copy YYYYMMDD part
                            dateTime->date[dateLen] = '\0';          // Null terminate

                            strncpy(dateTime->time, timePos + 1, 6); // Copy HHMMSS part
                            dateTime->time[6] = '\0';                // Null terminate

                            size_t timeLen = strlen(timePos);
                            if (timeLen > 0 && timePos[timeLen - 1] == 'Z')
                            {
                                dateTime->UTC = true;
                            }
                        }
                        else if (value[0] == 'T')
                        {
                            // Time-only format (e.g., T143000)
                            strncpy(dateTime->time, value + 1, 6); // Skip 'T' and copy HHMMSS
                            dateTime->time[6] = '\0';
                        }
                        else
                        {
                            // Date-only format (e.g., 20090808)
                            strncpy(dateTime->date, value, 8); // Copy YYYYMMDD
                            dateTime->date[8] = '\0';
                        }

                        // Check for UTC indicator ('Z' at the end of the time)
                        size_t timeLen = strlen(dateTime->time);
                        if (timeLen > 0 && dateTime->time[timeLen - 1] == 'Z')
                        {
                            dateTime->UTC = true;
                            dateTime->time[timeLen - 1] = '\0'; // Remove 'Z'
                        }

                        // Assign to the correct field in Card
                        if (strcmp(currentProperty->name, "BDAY") == 0)
                        {
                            if ((*obj)->birthday != NULL)
                            { // Prevent duplicate BDAY
                                free(dateTime->date);
                                free(dateTime->time);
                                free(dateTime->text);
                                free(dateTime);
                                free(currentProperty->name);
                                deleteCard(*obj);
                                fclose(file);
                                return INV_PROP;
                            }
                            (*obj)->birthday = dateTime;
                        }
                        else if (strcmp(currentProperty->name, "ANNIVERSARY") == 0)
                        {
                            if ((*obj)->anniversary != NULL)
                            { // Prevent duplicate ANNIVERSARY
                                free(dateTime->date);
                                free(dateTime->time);
                                free(dateTime->text);
                                free(dateTime);
                                free(currentProperty->name);
                                deleteCard(*obj);
                                fclose(file);
                                return INV_PROP;
                            }
                            (*obj)->anniversary = dateTime;
                        }
                    }

                    // Get values of the property otherwise
                    char *propValue = calloc(strlen(line) + 1, sizeof(char)); // Allocate memory for the property value
                    // Check if memory allocation failed
                    if (propValue == NULL)
                    {
                        free(propValue);
                        deleteCard(*obj);
                        return OTHER_ERROR;
                    }

                    int j = 0;
                    i++; // Skip the colon
                    char *currentValue = NULL;
                    while (line[i] != '\0')
                    {
                        // Get the values for our new properties
                        if (currentValue == NULL)
                        {
                            currentValue = calloc(strlen(line) + 1, sizeof(char)); // Allocate memory for the value
                            if (currentValue == NULL)
                            {
                                deleteCard(*obj);
                                return OTHER_ERROR;
                            }
                        }
                        if (line[i] == '\\' && line[i + 1] == ';')
                        {                            // Handle escaped semicolon
                            currentValue[j++] = ';'; // Add literal semicolon
                            i += 2;                  // Skip over "\;"
                        }
                        else if (line[i] == ';')
                        {                                                  // Split values on semicolon
                            currentValue[j] = '\0';                        // Null-terminate the current value
                            insertBack(newProperty->values, currentValue); // Add to values list
                            currentValue = NULL;                           // Reset for the next value
                            j = 0;                                         // Reset index
                            i++;
                        }
                        else
                        {
                            currentValue[j++] = line[i++]; // Add character to the current value
                        }
                    }
                    // Add the last value (after the final semicolon or end of line)
                    if (currentValue != NULL)
                    {
                        currentValue[j] = '\0'; // Null-terminate
                        insertBack(newProperty->values, currentValue);
                        currentValue = NULL;
                    }
                    free(currentValue); // Free the current value
                    free(propValue);    // Free the property value
                    if (newProperty->values == NULL)
                    {
                        deleteCard(*obj);
                        return INV_PROP;
                    }
                    if (newProperty->name == NULL)
                    {
                        deleteCard(*obj);
                        return INV_PROP;
                    }

                    if (strcmp(newProperty->name, "BDAY") != 0 && strcmp(newProperty->name, "ANNIVERSARY") != 0)
                    {
                        insertBack((*obj)->optionalProperties, newProperty); // Insert the new property into the optional properties list
                    }
                    else
                    {
                        deleteProperty(newProperty);
                    }
                }
            }
            else if (line[i] == '.')
            {
                char *dotPos = strchr(line, '.');   // Find the dot
                char *colonPos = strchr(line, ':'); // Find the colon
                char *semiPos = strchr(line, ';');  // Find first semicolon (parameters)

                // Extract the group (everything before the dot)
                size_t groupLen = dotPos - line;
                if (groupLen > 0)
                {
                    currentProperty->group = calloc(groupLen + 1, sizeof(char));
                    if (currentProperty->group == NULL)
                    {
                        deleteCard(*obj);
                        fclose(file);
                        return OTHER_ERROR;
                    }
                    strncpy(currentProperty->group, line, groupLen);
                    currentProperty->group[groupLen] = '\0'; // Null terminate
                }
                else
                {
                    // No group found
                    currentProperty->group = calloc(1, sizeof(char));
                }

                // Extract property name (everything after dot but before `;` or `:`)
                char *nameStart = dotPos + 1;
                char *nameEnd = (semiPos && semiPos < colonPos) ? semiPos : colonPos; // Stop at first `;` or `:`
                size_t nameLen = (nameEnd) ? (nameEnd - nameStart) : strlen(nameStart);

                if (nameLen > 0)
                {
                    // currentProperty->name = calloc(nameLen + 1, sizeof(char));
                    if (currentProperty->name == NULL)
                    {
                        deleteCard(*obj);
                        fclose(file);
                        return OTHER_ERROR;
                    }
                    strncpy(currentProperty->name, nameStart, nameLen);
                    currentProperty->name[nameLen] = '\0'; // Null terminate
                }
                else
                {
                    deleteCard(*obj);
                    fclose(file);
                    return INV_PROP;
                }
            }
            else
            {
                currentProperty->name[i] = line[i]; // Copy the character into the name
            }
        }
        free(line);
    }
    fclose(file);

    // If FN was not found, return an error
    if ((*obj)->fn == NULL)
    {
        deleteCard(*obj);
        return INV_CARD;
    }

    if ((*obj)->fn->group == NULL)
    {
        (*obj)->fn->group = calloc(1, sizeof(char)); // Allocates one char (initialized to '\0')
        if ((*obj)->fn->group == NULL)
        {
            deleteCard(*obj);
            return OTHER_ERROR;
        }
    }
    if (!fnTag)
    {
        deleteCard(*obj);
        return INV_CARD;
    }
    if (beginTag == false || endTag == false || versionTag == false)
    {
        deleteCard(*obj);
        *obj = NULL;
        return INV_CARD;
    }

    free(currentProperty->name);
    free(currentProperty);

    return OK;
}

void deleteCard(Card *obj)
{
    if (obj == NULL)
    {
        return; // Nothing to free
    }

    // 1. Free the FN property
    deleteProperty(obj->fn);

    // Free optional properties list
    if (obj->optionalProperties != NULL)
    {
        freeList(obj->optionalProperties); // Frees list and elements
    }

    // Free birthday and anniversary properties
    if (obj->birthday != NULL)
    {
        deleteDate(obj->birthday);
    }
    if (obj->anniversary != NULL)
    {
        deleteDate(obj->anniversary);
    }

    free(obj); // Free the Card object itself
    obj = NULL;
}

char *cardToString(const Card *obj)
{
    if (obj == NULL)
    {
        return NULL;
    }

    // Allocate a large enough buffer for the vCard string
    size_t bufferSize = 4096;
    char *string = calloc(bufferSize, sizeof(char));
    if (string == NULL)
    {
        return NULL;
    }

    // Append required headers
    strcat(string, "BEGIN:VCARD\r\n");
    strcat(string, "VERSION:4.0\r\n");

    // Add FN (Full Name)
    if (obj->fn != NULL)
    {
        char *fnString = propertyToString(obj->fn);
        if (fnString != NULL)
        {
            strcat(string, fnString);
            strcat(string, "\r\n");
            //printf("Length of fnString in cardToString: %ld\r\n", strlen(fnString));
            free(fnString);
        }
    }

    // Add Birthday
    if (obj->birthday != NULL)
    {
        char *bdayString = dateToString(obj->birthday);
        if (bdayString != NULL)
        {
            strcat(string, "BDAY:");
            strcat(string, bdayString);
            strcat(string, "\r\n");
            //("Length of bdayString in cardToString: %ld\r\n", strlen(bdayString));
            free(bdayString);
        }
    }

    // Add Anniversary
    if (obj->anniversary != NULL)
    {
        char *annString = dateToString(obj->anniversary);
        if (annString != NULL)
        {
            strcat(string, "ANNIVERSARY:");
            strcat(string, annString);
            strcat(string, "\r\n");
            //printf("Length of annString in cardToString: %ld\r\n", strlen(annString));
            free(annString);
        }
    }

    // Check if optionalProperties is NULL before iterating
    if (obj->optionalProperties != NULL)
    {
        ListIterator iter = createIterator(obj->optionalProperties);
        Property *prop;

        while ((prop = nextElement(&iter)) != NULL)
        {
            char *propString = propertyToString(prop);
            if (propString != NULL)
            {
                strcat(string, propString);
                strcat(string, "\r\n");
                //printf("Length of propString in cardToString: %ld\r\n", strlen(propString));
                free(propString);
            }
        }
    }

    // Append required footer
    strcat(string, "END:VCARD\r\n");

    return string;
}

char *errorToString(VCardErrorCode err)
{
    switch (err)
    {
    case OK:
        return "OK";
    case INV_FILE:
        return "Invalid File";
    case INV_CARD:
        return "Invalid Card";
    case INV_PROP:
        return "Invalid Property";
    case OTHER_ERROR:
        return "Other Error";
    default:
        return "Unknown Error";
    }
}
// ************* Assignment 2 functions - MUST be implemented ***************
VCardErrorCode writeCard(const char *fileName, const Card *obj)
{
    if (fileName == NULL || obj == NULL)
    {
        return WRITE_ERROR; // Invalid input
    }

    // Validate file extension
    const char *dot = strrchr(fileName, '.');
    if (!dot || (strcmp(dot, ".vcf") != 0 && strcmp(dot, ".vcard") != 0))
    {
        return WRITE_ERROR;
    }

    // Open file for writing
    FILE *file = fopen(fileName, "w");
    if (file == NULL)
    {
        return WRITE_ERROR;
    }


    
    //Declare everything before, it's weird, i don't know why
    char *fnString = propertyToString(obj->fn);
    if (fnString != NULL){
        strcat(fnString, "\r\n");
    }
    char *bdayString = dateToString(obj->birthday);
    if (bdayString != NULL){
        strcat(bdayString, "\r\n");
    }
    char *annString = dateToString(obj->anniversary);
    if (annString != NULL){
        strcat(annString, "\r\n");
    }
    ListIterator iter = createIterator(obj->optionalProperties);
    Property *prop;

    // Write vCard header
    fprintf(file, "BEGIN:VCARD\r\n");
    fprintf(file, "VERSION:4.0\r\n");

    // Write FN (Full Name)
    if (obj->fn != NULL && obj->fn->values != NULL)
    {
        fprintf(file, "%s", fnString);

        //printf("%s", fnString);
        //printf("REEEESULLTTTTT of strcmp: %d\n", strcmp(fnString, "FN:Simon Perreault\r\n"));

        free(fnString);
    }

    // Write BDAY
    if (obj->birthday != NULL)
    {

        fprintf(file, "BDAY%s", bdayString);
        // printAscii(bdayString);
        free(bdayString);
    }

    // Write ANNIVERSARY
    if (obj->anniversary != NULL)
    {
        fprintf(file, "ANNIVERSARY%s", annString);
        // printAscii(annString);
        free(annString);
    } 

    // Write optional properties
    //ListIterator iter = createIterator(obj->optionalProperties);
    //Property *prop;
    while ((prop = nextElement(&iter)) != NULL)
    {
        char *propString = propertyToString(prop);
        strcat(propString, "\r\n");
        fprintf(file, "%s", propString);
        free(propString);
    }

    // Write vCard footer
    fprintf(file, "END:VCARD\r\n");

    // Close file
    fclose(file);

    return OK;
}

VCardErrorCode validateCard(const Card *obj)
{

    if (obj == NULL)
    {
        //printf("[DEBUG] Card object is NULL\n");
        return INV_CARD; // Card object cannot be NULL
    }

    // Check required properties
    if (obj->fn == NULL || obj->fn->values == NULL || getLength(obj->fn->values) == 0)
    {
        //printf("[DEBUG] Missing FN property\n");
        return INV_CARD; // FN is required
    }

    // Make sure optionalProperties isn't NULL
    if (obj->optionalProperties == NULL)
    {
        //printf("[DEBUG] Missing otherProperties list\n");
        return INV_CARD;
    }

    // Check if its BDAY or TIME, before checking optional properties
    ListIterator tempIter = createIterator(obj->optionalProperties);
    Property *tempProp;
    while ((tempProp = nextElement(&tempIter)) != NULL)
    {
        if (strcmp(tempProp->name, "BDAY") == 0 || strcmp(tempProp->name, "ANNIVERSARY") == 0)
        {
            //printf("[DEBUG] DateTime property found in optionalProperties: %s\n", tempProp->name);
            return INV_DT;
        }
    }

    // Check for invalid VERSION property in optionalProperties
    ListIterator propIter = createIterator(obj->optionalProperties);
    Property *prop;
    while ((prop = nextElement(&propIter)) != NULL)
    {
        if (strcmp(prop->name, "VERSION") == 0)
        {
            //printf("[DEBUG] Invalid VERSION property\n");
            return INV_CARD; // VERSION should not be in optionalProperties
        }
    }

    // Validate optional properties
    propIter = createIterator(obj->optionalProperties);
    // bool foundN = false;
    int nCount = 0;
    while ((prop = nextElement(&propIter)) != NULL)
    {
        // Ensure property name is valid
        if (!isValidPropertyName(prop->name))
        {
            //printf("[DEBUG] Invalid property name: %s\n", prop->name);
            return INV_PROP;
        }

        // Ensure values are not NULL
        if (prop->values == NULL || getLength(prop->values) == 0)
        {
            //printf("[DEBUG] Missing values for property: %s\n", prop->name);
            return INV_PROP;
        }

        // Special case: `N` must have exactly 5 components
        if (strcmp(prop->name, "N") == 0)
        {
            nCount++;
            // foundN = true;
            if (getLength(prop->values) != 5)
            {
                //printf("[DEBUG] Invalid number of components for property N\n");
                return INV_PROP;
            }
        }

        // Check parameters
        ListIterator paramIter = createIterator(prop->parameters);
        Parameter *param;
        while ((param = nextElement(&paramIter)) != NULL)
        {
            if (param->name == NULL || param->value == NULL || strlen(param->name) == 0 || strlen(param->value) == 0)
            {
                //printf("[DEBUG] Invalid parameter: %s=%s\n", param->name, param->value);
                return INV_PROP;
            }
        }
    }

    // Ensure `N` does not appear more than once
    if (nCount > 1)
    {
        //printf("[DEBUG] Property N appears more than once\n");
        return INV_PROP;
    }

    // Validate DateTime fields (BDAY & ANNIVERSARY)
    if (obj->birthday != NULL)
    {
        VCardErrorCode dtResult = validateDateTime(obj->birthday);
        if (dtResult != OK)
        {
            //printf("[DEBUG] Invalid BDAY property\n");
            return dtResult;
        }
    }
    if (obj->anniversary != NULL)
    {
        VCardErrorCode dtResult = validateDateTime(obj->anniversary);
        if (dtResult != OK)
        {
            //printf("[DEBUG] Invalid ANNIVERSARY property\n");
            return dtResult;
        }
    }

    return OK; // Everything is valid
}

// *************************************************************************

// ************* List helper functions - MUST be implemented ***************
void deleteProperty(void *toBeDeleted)
{
    if (toBeDeleted == NULL)
    {
        return;
    }

    Property *property = (Property *)toBeDeleted; // Cast the void pointer to a Property pointer

    // printf("[DEBUG] Deleting property: %s\n", property->name ? property->name : "(null)");

    if (property->name != NULL)
    {
        free(property->name);
        property->name = NULL;
    }
    if (property->group != NULL)
    {
        free(property->group);
        property->group = NULL;
    }
    if (property->parameters != NULL)
    {
        freeList(property->parameters);
        property->parameters = NULL;
    }
    if (property->values != NULL)
    {
        freeList(property->values);
        property->values = NULL;
    }

    free(property);
}

int compareProperties(const void *first, const void *second)
{
    int result = strcmp(((Property *)first)->name, ((Property *)second)->name); // Compare the names of the two properties
    return result;                                                              // Return the result
}

char *propertyToString(void *prop)
{

    if (prop == NULL)
    {
        return NULL;
    }

    Property *property = (Property *)prop;

    // Ensure property->name is not NULL before using it
    if (property->name == NULL)
    {
        return NULL;
    }

    char *string = malloc(1000*sizeof(char)); // Allocate memory
    if (string == NULL)
    {
        return NULL;
    }

    // Start with the property name
    strcpy(string, property->name);
    //printf("Name: %s\n",string);

    // Ensure parameters list is not NULL before using it
    if (property->parameters != NULL)
    {
        ListIterator paramIter = createIterator(property->parameters);
        Parameter *param;
        while ((param = nextElement(&paramIter)) != NULL)
        {
            if (param->name != NULL && param->value != NULL) // Ensure parameter fields are not NULL
            {
                strcat(string, ";");
                strcat(string, param->name);
                strcat(string, "=");
                strcat(string, param->value);
                //printf("%s\n",string);
            }
        }
    }

    // Add values
    strcat(string, ":");
    //printf("Add colon%s\n",string);

    // Ensure values list is not NULL before iterating
    if (property->values != NULL)
    {
        ListIterator valueIter = createIterator(property->values);
        char *value;
        bool firstValue = true;

        // Detect if this property should use semicolons (structured values)
        bool isStructured = (strcmp(property->name, "N") == 0 || strcmp(property->name, "ADR") == 0);

        int valueCount = 0;
        while ((value = nextElement(&valueIter)) != NULL)
        {
            valueCount++;

            // If not the first value, insert the correct separator
            if (!firstValue)
            {
                strcat(string, isStructured ? ";" : ","); // Use ';' for structured props, ',' for normal
                //printf("Adding value: %s\n",string);
            }

            // Ensure value is not NULL before copying
            if (value != NULL)
            {
                // printf("value: %s\n", value);
                strcat(string, value);
                //printf("Adding value: %s\n",string);
            }
            firstValue = false;
        }

        // If structured, ensure at least 5 values (for N, ADR)
        if (isStructured && valueCount < 5)
        {
            while (valueCount < 5)
            {
                strcat(string, ";");
                //printf("%s\n",string);
                valueCount++;
            }
        }
    }

    // strcat(string,"\r\n");

    /* // Make string the exact size needed
    int strLength = strlen(string);
    string = realloc(string, strLength + 1); */

    //printf("[DEBUG] Final property string: '%s'\n", string);
    return string;
}

void deleteParameter(void *toBeDeleted)
{
    if (toBeDeleted == NULL)
    {
        return;
    }
    Parameter *param = (Parameter *)toBeDeleted; // Cast the void pointer to a Parameter pointer
    if (param->name != NULL)
    {
        free(param->name); // Free the name string
    }
    if (param->value != NULL)
    {
        free(param->value); // Free the value string
    }
    free(param); // Free the Parameter object
}
int compareParameters(const void *first, const void *second)
{
    int result = strcmp(((Parameter *)first)->name, ((Parameter *)second)->name); // Compare the names of the two parameters
    return result;                                                                // Return the result
}
char *parameterToString(void *param)
{
    if (param == NULL)
    {
        return NULL;
    }
    Parameter *parameter = (Parameter *)param; // Cast the void pointer to a Parameter pointer
    char *string = malloc(1000);
    if (string == NULL)
    {
        free(string);
        return NULL; // Allocation failed
    }
    strcpy(string, "Parameter: ");
    strcat(string, parameter->name);
    strcat(string, "=");
    strcat(string, parameter->value);
    return string;

    return NULL; // Placeholder
}

void deleteValue(void *toBeDeleted)
{
    if (toBeDeleted == NULL)
    {
        return;
    }
    char *value = (char *)toBeDeleted; // Cast the void pointer to a char pointer
    free(value);                       // Free the value string
}
int compareValues(const void *first, const void *second)
{
    int result = strcmp((char *)first, (char *)second); // Compare the two strings
    return result;                                      // Return the result
}
char *valueToString(void *val)
{
    char *string = malloc(1000);
    if (string == NULL)
    {
        free(string);
        return NULL; // Allocation failed
    }
    strcpy(string, (char *)val);
    return string;
}

void deleteDate(void *toBeDeleted)
{
    if (toBeDeleted == NULL)
    {
        return;
    }
    DateTime *date = (DateTime *)toBeDeleted; // Cast the void pointer to a DateTime pointer

    if (date->date != NULL)
    {
        free(date->date); // Free the date string
    }
    if (date->time != NULL)
    {
        free(date->time); // Free the time string
    }
    if (date->text != NULL)
    {
        free(date->text); // Free the text string
    }

    free(date); // Free the DateTime object
}
int compareDates(const void *first, const void *second)
{
    int result = strcmp(((DateTime *)first)->date, ((DateTime *)second)->date); // Compare the dates of the two DateTime objects
    return result;
}
char *dateToString(void *date)
{

    if (date == NULL)
    {
        return NULL;
    }

    DateTime *dateTime = (DateTime *)date;
    char *string = calloc(100, sizeof(char));
    if (string == NULL)
    {
        return NULL;
    }

    if (dateTime->isText)
    {
        strcat(string,";VALUE=text:");
        strcat(string, dateTime->text);
    }
    else
    {
        strcat(string,":");
        if (strlen(dateTime->date) > 0)
        {
            strcat(string, dateTime->date);
        }
        if (strlen(dateTime->time) > 0)
        {
            strcat(string, "T");
            strcat(string, dateTime->time);
        }

        if (dateTime->UTC)
        {
            strcat(string, "Z");
        }
    }

    return string;
}


// ************* ASSIGNMENT 3 FUNCTIONS HERE *************** //

// Returns a copy of the FN property value from the card.
// The caller is responsible for freeing the returned string.
char* getFN(const Card* card) {
    if (card == NULL || card->fn == NULL || card->fn->values == NULL) {
        return NULL;
    }
    // getFromFront returns a void* pointing to the first element in the list.
    // We cast it to char* since FN property values are strings.
    return (char*) getFromFront(card->fn->values);
}

// Returns a formatted birthday string from the card.
char* getBirthday(const Card* card) {
    if (card == NULL || card->birthday == NULL) {
        return NULL;
    }
    DateTime* bday = card->birthday;
    char* result = malloc(100);
    if (result == NULL) {
        return NULL;
    }
    if (bday->isText) {
        snprintf(result, 100, "%s", bday->text);
    } else {
        snprintf(result, 100, "%sT%s%s", bday->date, bday->time, bday->UTC ? "Z" : "");
    }
    return result;
}

// Returns a formatted anniversary string from the card.
char* getAnniversary(const Card* card) {
    if (card == NULL || card->anniversary == NULL) {
        return NULL;
    }
    DateTime* anniv = card->anniversary;
    char* result = malloc(100);
    if (result == NULL) {
        return NULL;
    }
    if (anniv->isText) {
        snprintf(result, 100, "%s", anniv->text);
    } else {
        snprintf(result, 100, "%sT%s%s", anniv->date, anniv->time, anniv->UTC ? "Z" : "");
    }
    return result;
}

// Returns the number of optional properties in the card.
int getOptionalPropertiesCount(const Card* card) {
    if (card == NULL || card->optionalProperties == NULL) {
        return 0;
    }
    return getLength(card->optionalProperties);
}

// Updates the FN (full name) property in the Card.
// It frees the old value in the FN values list (the first element)
// and replaces it with the new value.
// Returns OK on success or an appropriate error code.
VCardErrorCode updateFN(Card* card, const char* newFN) {
    if (card == NULL || card->fn == NULL || newFN == NULL || strlen(newFN) == 0) {
        return INV_PROP;
    }
    // Ensure the FN property’s name is set to "FN".
    if (card->fn->name == NULL) {
        card->fn->name = malloc(3);  // 2 characters for "FN" + 1 for null terminator.
        if (card->fn->name == NULL) {
            return OTHER_ERROR;
        }
        strcpy(card->fn->name, "FN");
    } else {
        // In case it already exists but is empty:
        if (strlen(card->fn->name) == 0) {
            strcpy(card->fn->name, "FN");
        }
    }
    // Now update the value.
    if (card->fn->values && card->fn->values->head) {
        free(card->fn->values->head->data);
        char* updated = malloc(strlen(newFN) + 1);
        if (updated == NULL) {
            return OTHER_ERROR;
        }
        strcpy(updated, newFN);
        card->fn->values->head->data = updated;
    } else {
        char* new_value = malloc(strlen(newFN) + 1);
        if (new_value == NULL) {
            return OTHER_ERROR;
        }
        strcpy(new_value, newFN);
        insertBack(card->fn->values, new_value);
    }
    return OK;
}

// Updates the Birthday field in the Card.
// For simplicity, we assume that the new birthday is given as a text value.
// We mark the DateTime as text-based and store the new value in the 'text' field,
// clearing any numeric date or time.
VCardErrorCode updateBirthday(Card* card, const char* newBirthday) {
    if (card == NULL || newBirthday == NULL) {
        return INV_PROP;
    }
    if (card->birthday == NULL) {
        card->birthday = malloc(sizeof(DateTime));
        if (card->birthday == NULL) return OTHER_ERROR;
        card->birthday->date = calloc(9, sizeof(char));  // 8 digits + null
        card->birthday->time = calloc(7, sizeof(char));  // 6 digits + null
        card->birthday->text = calloc(strlen(newBirthday) + 1, sizeof(char));
        if (!card->birthday->date || !card->birthday->time || !card->birthday->text) {
            free(card->birthday->date);
            free(card->birthday->time);
            free(card->birthday->text);
            free(card->birthday);
            return OTHER_ERROR;
        }
    }
    card->birthday->isText = true;
    card->birthday->UTC = false;
    strcpy(card->birthday->text, newBirthday);
    card->birthday->date[0] = '\0';
    card->birthday->time[0] = '\0';
    return OK;
}

// Updates the Anniversary field in the Card.
// Similar to updateBirthday: treats the new anniversary as a text value.
VCardErrorCode updateAnniversary(Card* card, const char* newAnniv) {
    if (card == NULL || newAnniv == NULL) {
        return INV_PROP;
    }
    if (card->anniversary == NULL) {
        card->anniversary = malloc(sizeof(DateTime));
        if (card->anniversary == NULL) return OTHER_ERROR;
        card->anniversary->date = calloc(9, sizeof(char));
        card->anniversary->time = calloc(7, sizeof(char));
        card->anniversary->text = calloc(strlen(newAnniv) + 1, sizeof(char));
        if (!card->anniversary->date || !card->anniversary->time || !card->anniversary->text) {
            free(card->anniversary->date);
            free(card->anniversary->time);
            free(card->anniversary->text);
            free(card->anniversary);
            return OTHER_ERROR;
        }
    }
    card->anniversary->isText = true;
    card->anniversary->UTC = false;
    strcpy(card->anniversary->text, newAnniv);
    card->anniversary->date[0] = '\0';
    card->anniversary->time[0] = '\0';
    return OK;
}

// Allocates and returns a new vCard with a minimal structure.
// We need this so that we can “create” a new card on disk when the user presses OK in the Create view
Card* newCard() {
    Card* card = malloc(sizeof(Card));
    if (!card) return NULL;

    // Allocate and initialize FN property.
    card->fn = malloc(sizeof(Property));
    if (!card->fn) {
        free(card);
        return NULL;
    }
    card->fn->name = calloc(30, sizeof(char));  
    strcpy(card->fn->name, "FN");
    card->fn->group = calloc(30, sizeof(char));
    card->fn->parameters = initializeList(&parameterToString, &deleteParameter, &compareParameters);
    card->fn->values = initializeList(&valueToString, &deleteValue, &compareValues);
    
    // Initialize optional properties list.
    card->optionalProperties = initializeList(&propertyToString, &deleteProperty, &compareProperties);
    
    card->birthday = NULL;
    card->anniversary = NULL;
    return card;
}