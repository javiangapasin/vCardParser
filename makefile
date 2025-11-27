# Compiler and flags
CC = gcc
CFLAGS = -Wall -std=c11 -g -fPIC
LDFLAGS = -L.

# Directories
INC = include/
SRC = src/
BIN = bin/

# Shared library
LIB = $(BIN)libvcparser.so

# Object files (excluding main.o)
OBJ = $(BIN)VCParser.o $(BIN)VCHelpers.o $(BIN)LinkedListAPI.o

# Default target: build the shared library 
all: parser main

# Build the shared library
parser: $(LIB)

$(LIB): $(OBJ)
	$(CC) -shared -o $(LIB) $(OBJ)

# Compile the main parser file into an object file
$(BIN)VCParser.o: $(SRC)VCParser.c $(INC)VCParser.h $(INC)LinkedListAPI.h $(BIN)
	$(CC) $(CFLAGS) -I$(INC) -c $(SRC)VCParser.c -o $(BIN)VCParser.o

# Compile the helpers file into an object file
$(BIN)VCHelpers.o: $(SRC)VCHelpers.c $(INC)VCParser.h $(INC)LinkedListAPI.h $(BIN)
	$(CC) $(CFLAGS) -I$(INC) -c $(SRC)VCHelpers.c -o $(BIN)VCHelpers.o

# Compile the linked list file into an object file
$(BIN)LinkedListAPI.o: $(SRC)LinkedListAPI.c $(INC)LinkedListAPI.h $(BIN)
	$(CC) $(CFLAGS) -I$(INC) -c $(SRC)LinkedListAPI.c -o $(BIN)LinkedListAPI.o

# Compile the main test program into an object file
$(BIN)main.o: $(SRC)main.c $(INC)VCParser.h $(BIN)
	$(CC) $(CFLAGS) -I$(INC) -c $(SRC)main.c -o $(BIN)main.o

# Build the test program using main.o
main: $(BIN)main.o $(LIB)
	$(CC) $(CFLAGS) -I$(INC) -o main $(BIN)main.o  -I$(INC) -L$(BIN) -lvcparser 
	
# Clean up all generated files
clean:
	rm -f $(BIN)*.o $(BIN)/*.so 
