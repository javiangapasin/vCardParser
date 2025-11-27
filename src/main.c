#include "VCParser.h"
#include "VCHelpers.h"

int main()
{
    // First, create a Card using your createCard function from a reference file
    Card *origCard = NULL;
    VCardErrorCode err = createCard("testCard-BdayText.vcf", &origCard);
    if (err != OK) {
        printf("Error creating original card: %s\n", errorToString(err));
        return 1;
    }

    //Print initial card parsed string
    char *origCardStr = cardToString(origCard);
    if (origCardStr == NULL) {
        printf("Error converting re-parsed card to string\n");
        deleteCard(origCard);
        return 1;
    }
    // Print the parsed card string with delimiters
    printf("Re-parsed Card string: ->%s<-\n", origCardStr);
    printf("Hex dump of parsed Card string:\n");
    // hexDump(origCardStr);
    printf("Calling print ascii on original parse\n");
    // printAscii(origCardStr);

    // Write the card to a file using writeCard
    err = writeCard("myTestCard.vcf", origCard);
    if (err != OK) {
        printf("Error writing card: %s\n", errorToString(err));
        deleteCard(origCard);
        return 1;
    }

    // Re-parse the written file to obtain a new Card
    Card *reParsedCard = NULL;
    err = createCard("myTestCard.vcf", &reParsedCard);
    if (err != OK) {
        printf("Error re-parsing card: %s\n", errorToString(err));
        deleteCard(origCard);
        return 1;
    }

    // Convert the re-parsed Card to a string
    char *reParsedStr = cardToString(reParsedCard);
    if (reParsedStr == NULL) {
        printf("Error converting re-parsed card to string\n");
        deleteCard(origCard);
        deleteCard(reParsedCard);
        return 1;
    }

    // Print the re-parsed card string with delimiters
    printf("Re-parsed Card string: ->%s<-\n", reParsedStr);
    printf("Hex dump of re-parsed Card string:\n");
    // hexDump(reParsedStr);
    printf("Calling print ascii on reparse\n");
    // printAscii(reParsedStr);

    // Compare the length of the re-parsed card string with your expected reference.
    const char *expected = "BEGIN:VCARD\r\nVERSION:4.0\r\nFN:Simon Perreault\r\nEND:VCARD\r\n";
    size_t lenParsed = strlen(reParsedStr);
    size_t lenExpected = strlen(expected);
    printf("Length of re-parsed string: %zu\n", lenParsed);
    printf("Length of expected string: %zu\n", lenExpected);
    if (lenParsed != lenExpected) {
        printf("Length mismatch!\n");
    } else {
        printf("Lengths match.\n");
    }

    // Optionally, compare field by field. For example, inspect the FN property:
    if (reParsedCard->fn != NULL) {
        printf("FN property name: ->%s<-\n", reParsedCard->fn->name);
        if (reParsedCard->fn->values != NULL) {
            ListIterator fnValIter = createIterator(reParsedCard->fn->values);
            char *val;
            while ((val = nextElement(&fnValIter)) != NULL) {
                printf("FN value: ->%s<-\n", val);
            }
        }
    }

    // Clean up
    free(origCardStr);
    free(reParsedStr);
    deleteCard(origCard);
    deleteCard(reParsedCard);


    /* // Testing for createCard

    Card *testCard = NULL;
    createCard("testCardMin.vcf", &testCard);

    // Test propertyToString
    // char * propString = propertyToString((testCard)->fn);
    // printf("Prop string: %s\n", propString);
    // free(propString); // Don't forget to free the allocated memory for propString

    // Test cardToString
    char *cardString = cardToString(testCard);
    printf("Card string for first card:\n%s", cardString);
    writeCard("myTestCard.vcf", testCard);

    //Check the result of writeCard
    Card *testCard2 = NULL;
    createCard("myTestCard.vcf", &testCard2);

    char *cardString2 = cardToString(testCard2);
    printf("Card string for second card:\n%s", cardString2);
    writeCard("writeCardOutput.vcf", testCard2);

    //Compare string values
    if (strcmp(cardString, cardString2) == 0)
    {
        printf("The strings are the same\n");
    }
    else
    {
        printf("The strings are different\n");
    }

    // Test validateCard
    VCardErrorCode error = validateCard(testCard);
    char *errorString = errorToString(error);
    printf("Error code: %d\n", error);
    printf("Error string: %s\n", errorString);

    free(cardString); // Don't forget to free the allocated memory for cardString
    free(cardString2); // Don't forget to free the allocated memory for cardString2

    // Call deleteCard
    deleteCard((testCard));
    deleteCard((testCard2));
     */

    return 0;
}
