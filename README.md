# vCard Parser

A C-based vCard (virtual contact card) parser and manager that reads, validates, and writes vCard 4.0 format files. This project implements comprehensive parsing, validation, and manipulation of vCard data structures.

## Overview

This project implements a complete vCard 4.0 parser capable of:

- **Parsing** vCard (.vcf) files into structured data
- **Validating** vCard format and required properties
- **Writing** vCard objects back to files
- **Managing** contact properties including names, birthdays, and anniversaries
- **Handling** complex vCard structures with parameters, groups, and multiple values

## Project Structure

```
.
├── bin/                          # Compiled binaries and object files
│   ├── A3Main.py                # Python main entry point
│   ├── libvcparser.so           # Compiled shared library
│   └── cards/                   # Test vCard files
├── src/                         # Source code
│   ├── main.c                   # Test program
│   ├── VCParser.c               # vCard parsing logic
│   ├── VCHelpers.c              # Helper functions
│   └── LinkedListAPI.c          # Linked list implementation
├── include/                     # Header files
│   ├── VCParser.h               # vCard structures and API
│   ├── VCHelpers.h              # Helper function declarations
│   └── LinkedListAPI.h          # Linked list API
├── makefile                     # Build configuration
└── main                         # Compiled executable
```

## Key Data Structures

### `Card`

Represents a complete vCard object:

- `fn` - Full name property (required, must exist)
- `optionalProperties` - List of additional properties
- `birthday` - DateTime structure for birthday
- `anniversary` - DateTime structure for anniversary

### `Property`

Represents a vCard property:

- `name` - Property name (e.g., "FN", "TEL", "EMAIL")
- `group` - Optional group identifier
- `parameters` - List of parameters for the property
- `values` - List of values for the property

### `Parameter`

Represents a property parameter:

- `name` - Parameter name
- `value` - Parameter value

### `DateTime`

Represents date-time values for date-related properties:

- `UTC` - Boolean indicating UTC time
- `isText` - Boolean indicating text representation (e.g., "circa 1800")
- `date` - YYYYMMDD format
- `time` - HHMMSS format
- `text` - Text representation

## Building

### Prerequisites

- GCC compiler
- Make
- C11 standard library

### Compilation

Build the project using Make:

```bash
make all
```

This command:

1. Compiles the vCard parser library (`libvcparser.so`)
2. Compiles object files for all modules
3. Builds the main executable

Build individual components:

```bash
make parser          # Build just the shared library
make main            # Build just the main executable
```

### Cleaning

Remove all compiled files:

```bash
make clean
```

## Building Details

### Compiler Flags

- `-Wall` - Enable all warnings
- `-std=c11` - C11 standard
- `-g` - Include debugging symbols
- `-fPIC` - Position-independent code (for shared library)

### Library

A shared library (`libvcparser.so`) is built containing the core parsing logic:

- VCParser module
- VCHelpers module
- LinkedListAPI module

The main executable links against this library.

## Core Functions

### Card Management

- `createCard(fileName, &card)` - Parse a vCard file into a Card structure
- `deleteCard(card)` - Free all memory associated with a Card
- `cardToString(card)` - Convert a Card to a formatted string representation

### File I/O

- `writeCard(fileName, card)` - Write a Card object to a vCard file
- `validateCard(card)` - Validate a Card against vCard 4.0 spec

### Property Access (Assignment 3)

- `getFN(card)` - Get the full name
- `getBirthday(card)` - Get the birthday
- `getAnniversary(card)` - Get the anniversary
- `getOptionalPropertiesCount(card)` - Count optional properties
- `updateFN(card, newFN)` - Update the full name
- `updateBirthday(card, newBirthday)` - Update the birthday
- `updateAnniversary(card, newAnniv)` - Update the anniversary
- `newCard()` - Create a new empty card

### Error Handling

- `errorToString(errorCode)` - Convert error codes to readable messages

Error codes:

- `OK` - No error
- `INV_FILE` - Invalid file
- `INV_CARD` - Invalid card
- `INV_PROP` - Invalid property
- `INV_DT` - Invalid date-time
- `WRITE_ERROR` - Error writing file
- `OTHER_ERROR` - Other errors

## Usage Example

```c
#include "VCParser.h"

int main() {
    // Parse a vCard file
    Card *card = NULL;
    VCardErrorCode err = createCard("contact.vcf", &card);

    if (err != OK) {
        printf("Error: %s\n", errorToString(err));
        return 1;
    }

    // Access card information
    printf("Name: %s\n", getFN(card));
    printf("Birthday: %s\n", getBirthday(card));

    // Modify card
    updateFN(card, "John Doe");

    // Write to file
    err = writeCard("output.vcf", card);

    // Clean up
    deleteCard(card);
    return 0;
}
```

## Test Files

The `bin/cards/` directory contains various test vCard files:

- `testCardMin.vcf` - Minimal valid vCard
- `testCard.vcf` - Standard vCard
- `testCard-BdayText.vcf` - Birthday as text value
- `testCard-BdayTime.vcf` - Birthday with time
- `testCardGroup.vcf` - vCard with groups
- `testCardProp-Params.vcf` - Properties with parameters
- `testCardProps-compVal.vcf` - Complex property values
- Plus additional test cases for various vCard formats

## Implementation Notes

- Follows vCard 4.0 specification (RFC 6350)
- Uses linked lists for dynamic data management
- Assumes vCard version is always 4.0
- FN (Full Name) property is required and always present
- Birthday and Anniversary are optional DateTime properties
- Supports parameter groups and complex property values
- Handles folded lines (line folding/unfolding)

## Authors

CIS 2750 Assignment 3 - Winter 2025

## License

Academic assignment - University of Guelph CIS 2750

---

For more information on the vCard 4.0 format, refer to [RFC 6350](https://tools.ietf.org/html/rfc6350).
