#!/usr/bin/env python3
from asciimatics.widgets import Frame, ListBox, Layout, Divider, Text, Label, TextBox, Button, Widget
from asciimatics.scene import Scene
from asciimatics.screen import Screen
from asciimatics.exceptions import ResizeScreenError, NextScene, StopApplication
import sys
import os
import ctypes
from ctypes import *
import mysql.connector
import datetime
from datetime import datetime #needed for datetime.now()

# Ctypes setup for libvcparser.so
vcparser = CDLL("./libvcparser.so")

# Set up createCard (expects a filename and a pointer to a Card pointer)
vcparser.createCard.argtypes = [c_char_p, POINTER(c_void_p)]
vcparser.createCard.restype = c_int

# Set up validateCard (expects a Card pointer)
vcparser.validateCard.argtypes = [c_void_p]
vcparser.validateCard.restype = c_int

# Set up writeCard (expects a filename and a Card pointer)
vcparser.writeCard.argtypes = [c_char_p, c_void_p]
vcparser.writeCard.restype = c_int

# Set up functions to extract fields from a Card, we wrote C functions for this
vcparser.getFN.argtypes = [c_void_p]
vcparser.getFN.restype = c_char_p

vcparser.getBirthday.argtypes = [c_void_p]
vcparser.getBirthday.restype = c_char_p

vcparser.getAnniversary.argtypes = [c_void_p]
vcparser.getAnniversary.restype = c_char_p

# Define the argument and return types for getOptionalPropertiesCount.
vcparser.getOptionalPropertiesCount.argtypes = [c_void_p]
vcparser.getOptionalPropertiesCount.restype = c_int

# Set up the updateFN function.
vcparser.updateFN.argtypes = [c_void_p, c_char_p]
vcparser.updateFN.restype = c_int

# Set up the updateBirthday function.
vcparser.updateBirthday.argtypes = [c_void_p, c_char_p]
vcparser.updateBirthday.restype = c_int

# Set up the updateAnniversary function.
vcparser.updateAnniversary.argtypes = [c_void_p, c_char_p]
vcparser.updateAnniversary.restype = c_int

# Set up newCard
vcparser.newCard.argtypes = []
vcparser.newCard.restype = c_void_p


# Python wrapper functions for C library
#Create Card wrapper function
def createCard_c(filename):
    ##print(f"Python wrapper: Calling createCard with '{filename}'")
    c_filename = filename.encode("utf-8")  # Convert Python string to C string (bytes)
    card_ptr = c_void_p()  # Pointer to a Card pointer
    returnCode = vcparser.createCard(c_char_p(c_filename), byref(card_ptr)) # Call the C function
    ##print(f"Python wrapper: createCard returned {returnCode}, card_ptr={card_ptr.value}")
    return returnCode, card_ptr.value

#Validate Card wrapper function
def validateCard_c(card_ptr):
    ##print(f"Python wrapper: Calling validateCard with card_ptr={card_ptr}")
    #This is like doing card_ptr->validateCard() in C
    returnCode = vcparser.validateCard(c_void_p(card_ptr)) # Call the C function
    #print(f"Python wrapper: validateCard returned {returnCode}")
    return returnCode

#Write Card wrapper function
def writeCard_c(filename, card_ptr):
    #print(f"Python wrapper: Calling writeCard with filename '{filename}' and card_ptr={card_ptr}")
    returnCode = vcparser.writeCard(filename.encode("utf-8"), c_void_p(card_ptr))
    #print(f"Python wrapper: writeCard returned {returnCode}")
    return returnCode

#Get FN wrapper function, which is a helper function that extracts the FN field from a Card.
def get_fn(card_ptr):
    result = vcparser.getFN(c_void_p(card_ptr))
    return result.decode('utf-8') if result else "" 

#Get Birthday wrapper function, which is a helper function that extracts the Birthday field from a Card.
def get_birthday(card_ptr):
    result = vcparser.getBirthday(c_void_p(card_ptr))
    return result.decode('utf-8') if result else ""

#Get Anniversary wrapper function, which is a helper function that extracts the Anniversary field from a Card.
def get_anniversary(card_ptr):
    result = vcparser.getAnniversary(c_void_p(card_ptr))
    return result.decode('utf-8') if result else ""

#Get Optional Properties Count wrapper function, which is a helper function that extracts the count of optional properties from a Card.
def get_optional_properties_count(card_ptr):
    count = vcparser.getOptionalPropertiesCount(c_void_p(card_ptr))
    #print(f"Python wrapper: optional properties count = {count}")
    return count

#Update FN wrapper function, which is a helper function that updates the FN field of a Card.
def update_fn(card_ptr, new_fn):
    """Update the FN property of the card via the C function."""
    returnCode = vcparser.updateFN(c_void_p(card_ptr), new_fn.encode('utf-8'))
    #print(f"Python wrapper: updateFN returned {returnCode}")
    return returnCode

#Update Birthday wrapper function, which is a helper function that updates the Birthday field of a Card.
def update_birthday(card_ptr, new_bday):
    """Update the Birthday field of the card via the C function."""
    returnCode = vcparser.updateBirthday(c_void_p(card_ptr), new_bday.encode('utf-8'))
    #print(f"Python wrapper: updateBirthday returned {returnCode}")
    return returnCode

#Update Anniversary wrapper function, which is a helper function that updates the Anniversary field of a Card.
def update_anniversary(card_ptr, new_anniv):
    """Update the Anniversary field of the card via the C function."""
    returnCode = vcparser.updateAnniversary(c_void_p(card_ptr), new_anniv.encode('utf-8'))
    #print(f"Python wrapper: updateAnniversary returned {returnCode}")
    return returnCode

#New Card wrapper function, which is a helper function that creates a new Card.
def new_card_c():
    #print("Python wrapper: Calling newCard")
    card_ptr = vcparser.newCard()
    #print(f"Python wrapper: newCard returned card_ptr={card_ptr}")
    return card_ptr


# Function to scan the 'cards/' folder for valid vCard files.
# For each file ending with .vcf or .vcard, it calls createCard and validateCard.
# Only files for which both return OK are considered valid.
def scan_cards_folder(folder="cards"):
    valid_files = [] # List of valid vCard filenames.
    for file in os.listdir(folder): #Go through all files in the folder
        if file.lower().endswith((".vcf", ".vcard")):
            full_path = os.path.join(folder, file)
            #print(f"Scanning file: {full_path}")
            # Call createCard and validateCard for each file.
            # createCard returns a code and a pointer to the Card, so we set both
            returnCode, card_ptr = createCard_c(full_path) 
            if returnCode == 0 and card_ptr is not None:
                validateReturn = validateCard_c(card_ptr)
                if validateReturn == 0:
                    #print(f"File '{file}' is valid.")
                    valid_files.append(file)
                #else:
                    #print(f"File '{file}' is invalid (validateCard returned {validateReturn}).")
            else:
                print(f"File '{file}' could not be created (createCard returned {returnCode}).")
    return valid_files


# -------------------------------
# Database Manager Class
# This class will handle all the database operations
class DatabaseManager:
    #Set up init to store the connection details
    def __init__(self, host, database, user, password):
        self.host = host
        self.database = database
        self.user = user
        self.password = password
        self.conn = None
        self.cursor = None
        self._errorLabel = Label("")

    #The connect function will connect to the database and create the tables if they don't exist
    def connect(self):
        try:
            self.conn = mysql.connector.connect(
                host=self.host,
                database=self.database,
                user=self.user,
                password=self.password
            )
            self.conn.autocommit = True
            # Use a buffered cursor so results are immediately fetched.
            self.cursor = self.conn.cursor(buffered=True)
            self.create_tables() #Create the tables if they don't exist
            #print("Database connected and tables created (if not exist).")
            return True
        except mysql.connector.Error as err:
            print("DB Connection error:", err)
            return False

    #Create the FILE and CONTACT tables if they don't exist
    def create_tables(self):
        # Create the FILE table. Adding UNIQUE to file_name prevents duplicates.
        file_table = """
            CREATE TABLE IF NOT EXISTS FILE (
                file_id INT AUTO_INCREMENT PRIMARY KEY,
                file_name VARCHAR(60) NOT NULL UNIQUE,
                last_modified DATETIME,
                creation_time DATETIME NOT NULL
            );
        """
        # Create the CONTACT table.
        contact_table = """
            CREATE TABLE IF NOT EXISTS CONTACT (
                contact_id INT AUTO_INCREMENT PRIMARY KEY,
                name VARCHAR(256) NOT NULL,
                birthday DATETIME,
                anniversary DATETIME,
                file_id INT NOT NULL,
                FOREIGN KEY (file_id) REFERENCES FILE(file_id) ON DELETE CASCADE
            );
        """
        self.cursor.execute(file_table)
        self.cursor.execute(contact_table)

    #Function to insert a file into the FILE table
    def insert_file(self, file_name, creation_time, last_modified):
        # Check if the file already exists in the FILE table.
        self.cursor.execute("SELECT file_id FROM FILE WHERE file_name = %s", (file_name,))
        row = self.cursor.fetchone() #This will return the first row of the result
        if row:
            # Return the existing file_id.
            return row[0]
        # Otherwise, insert the new file record.
        self.cursor.execute(
            "INSERT INTO FILE (file_name, creation_time, last_modified) VALUES (%s, %s, %s)",
            (file_name, creation_time, last_modified)
        )
        return self.cursor.lastrowid

    #Function to insert a contact into the CONTACT table
    def insert_contact(self, name, birthday, anniversary, file_id):
        # Only insert a contact if one doesn't already exist for that file.
        self.cursor.execute("SELECT contact_id FROM CONTACT WHERE file_id = %s", (file_id,))
        row = self.cursor.fetchone()
        if row:
            # If a contact already exists, update it instead.
            self.cursor.execute(
                "UPDATE CONTACT SET name = %s, birthday = %s, anniversary = %s WHERE contact_id = %s",
                (name, birthday, anniversary, row[0])
            )
            return row[0]
        else:
            self.cursor.execute(
                "INSERT INTO CONTACT (name, birthday, anniversary, file_id) VALUES (%s, %s, %s, %s)",
                (name, birthday, anniversary, file_id)
            )
            return self.cursor.lastrowid

    #Function to update the last_modified field of a file in the FILE table, we need this for the last_modified field in the FILE table
    def update_file(self, file_id, last_modified):
        self.cursor.execute("UPDATE FILE SET last_modified = %s WHERE file_id = %s", (last_modified, file_id))

    #Function to update a contact in the CONTACT table, we need this to update the contact details
    def update_contact(self, contact_id, name, birthday, anniversary):
        self.cursor.execute(
            "UPDATE CONTACT SET name = %s, birthday = %s, anniversary = %s WHERE contact_id = %s",
            (name, birthday, anniversary, contact_id)
        )

    def close(self):
        self.cursor.close()
        self.conn.close()

# -------------------------------
# VCardModel Class:
class VCardModel():
    def __init__(self, db_manager):
        self._vcards = []  # List of valid vCard filenames.
        self.current_data = {}  # Dictionary holding data of the currently loaded card.
        self.current_card_ptr = None  # Pointer (int) to the current card (from C library).
        self.db_manager = db_manager  # Instance of DatabaseManager.
        self._reload()

    def _reload(self):
        #print("Reloading vCard list from disk...")
        self._vcards = scan_cards_folder("cards")
        #print("Valid vCard files found:", self._vcards)
        
        # For each valid file, insert into database
        for filename in self._vcards:
            self._loadFileToDB(filename)

    #Get the list of vCard files
    def get_vcard_list(self):
        #Return a list of tuples where each tuple is (display_name, return_value)
        return [(f, f) for f in self._vcards] 

    #Load a vCard file
    def load_vcard(self, filename):
        #print(f"load_vcard() called for file: {filename}")
        full_path = os.path.join("cards", filename)
        ret, card_ptr = createCard_c(full_path) #Create the card
        if ret == 0 and card_ptr is not None:
            if validateCard_c(card_ptr) == 0:
                self.current_card_ptr = card_ptr
                #Get the FN, birthday, and anniversary fields from the card
                fn = get_fn(card_ptr)
                bday = get_birthday(card_ptr)
                anniv = get_anniversary(card_ptr)
                other_props = get_optional_properties_count(card_ptr)
                #Set these values in the current_data dictionary that holds the current card's data
                self.current_data = {
                    "filename": filename,
                    "contactName": fn,
                    "birthday": bday,
                    "anniversary": anniv,
                    "otherProps": other_props
                }
                #Use the database manager to insert or update the file and contact
                #datetime.now formats the current time as a string, e.g., "2021-09-30 12:34:56"
                now = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
                #Insert the file and contact into the database, using filename, creation time, and last modified time
                file_id = self.db_manager.insert_file(filename, now, now)
                #Insert the contact into the database, using the contact name, birthday, anniversary, and file_id
                #If the birthday or anniversary is an empty string, we store None in the database
                self.db_manager.insert_contact(fn, bday if bday != "" else None, anniv if anniv != "" else None, file_id)
            else:
                #print(f"Error: validateCard failed for file {filename}")
                self.current_data = {}
        else:
            #print(f"Error: createCard failed for file {filename}")
            self.current_data = {}

    #Create a new vCard
    def create_new_vcard(self, data):
        # Validate required input fields.
        #Check if the filename is empty, use strip to remove whitespace
        if data["filename"].strip() == "": 
            print("Error: Filename cannot be empty.")
            return  # Do not proceed if filename is empty.
        if data["contactName"].strip() == "":
            print("Error: Contact Name cannot be empty.")
            return  # Do not proceed if FN is empty.
        
        # Create a new Card using the newCard function.
        new_card_ptr = new_card_c()
        if not new_card_ptr:
            print("Error: newCard failed.")
            return

        # Update the FN property in the new card.
        update_code = update_fn(new_card_ptr, data["contactName"])
        if update_code != 0:
            print("Error: updateFN failed.")
            return

        # Validate the new card before writing.
        if validateCard_c(new_card_ptr) != 0:
            print("Error: New card is invalid. Please check the entered values.")
            return

        # (Birthday and Anniversary are not entered during creation, so we leave them blank.)
        file_path = os.path.join("cards", data["filename"])
        ret = writeCard_c(file_path, new_card_ptr)
        if ret != 0:
            print("Error: writeCard failed for new card.")
            return

        # If we reached here, the card is valid and has been written
        print("New card written successfully.")
        self._vcards.append(data["filename"])
        self.current_data = data
        self.current_card_ptr = new_card_ptr

        # Update the database.
        now = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        file_id = self.db_manager.insert_file(data["filename"], now, now)
        self.db_manager.insert_contact(data["contactName"], None, None, file_id)

    #Function to update a vCard, takes two arguments: the filename and the data dictionary
    def update_vcard(self, data):
        #print("update_vcard() called with data:")
        #print(data)
        file_path = os.path.join("cards", data["filename"])
        if self.current_card_ptr is not None:
            # Update the fields in the current card.
            update_fn(self.current_card_ptr, data["contactName"])
            update_birthday(self.current_card_ptr, data["birthday"])
            update_anniversary(self.current_card_ptr, data["anniversary"])
            ret = writeCard_c(file_path, self.current_card_ptr)
            #print(f"update_vcard: writeCard returned {ret}")
            if ret != 0:
                print("Error: writeCard failed during update.")
        else:
            print("Warning: No current card pointer; update not written to disk.")
        self.current_data = data
        # Reload the card so the model reflects what was saved.
        self.load_vcard(data["filename"])

    #Function to display all contacts in the database
    def db_display_all(self):
        #print("db_display_all() called")
        # Use DATE_FORMAT in the query so MySQL returns strings instead of Python datetime objects
        query = """
            SELECT contact_id,
                name,
                DATE_FORMAT(birthday, '%Y-%m-%d %H:%i:%s') AS bday, -- DATE_FORMAT() to get a string
                DATE_FORMAT(anniversary, '%Y-%m-%d %H:%i:%s') AS ann,
                file_id
            FROM CONTACT
        """
        self.db_manager.cursor.execute(query)
        rows = self.db_manager.cursor.fetchall() #Get all rows from the query

        # SQL query returns a list of tuples, one per row.
        # I want to fix how that looks when we run this query
        # So, I'll build a list of strings, one per row, with columns separated by " | "
        lines = [] #List to hold the lines for the UI
        for row in rows: #Iterate through each row
            # row might look like (1, "Simon Perreault", "2008-06-05 00:00:00", "2009-08-08 14:30:00", 2)
            # Convert each row to a list of strings, so we can join them with " | "
            #We use str(col) to convert each column to a string
            row_as_str = [str(col) for col in row] 
            lines.append(" | ".join(row_as_str)) #Join the columns with " | " and add to the lines list

        # Return a single string with newlines for the UI
        return "\n".join(lines)

    #Function that we'll use to find contacts with June birthdays
    def db_find_june_birthdays(self):
        query = """
            SELECT
            c.name, -- Contact name
            DATE_FORMAT(c.birthday, '%Y-%m-%d') AS bday -- DATE_FORMAT() to get a string
            FROM CONTACT c
            JOIN FILE f ON c.file_id = f.file_id -- Join with FILE to get last_modified
            WHERE MONTH(c.birthday) = 6 -- Filter by June birthdays
            ORDER BY TIMESTAMPDIFF(YEAR, c.birthday, f.last_modified) DESC -- Sort by age, the TIMESTAMPDIFF function calculates the difference in years
        """
        self.db_manager.cursor.execute(query)
        rows = self.db_manager.cursor.fetchall()

        # Do a similar process as db_display_all to format the rows for the UI
        lines = []
        for row in rows:
            # This time, we only have two columns: name and birthday
            # row[0] = name, row[1] = bday
            lines.append(f"{row[0]} | {row[1]}")

        return "\n".join(lines)

    #Load a vCard file's data into the database if not already present.
    #If the file or contact(s) exist, we update them instead of re-inserting.
    def _loadFileToDB(self, filename):
        full_path = os.path.join("cards", filename)

        # Attempt to create/validate the card in memory to get its data (FN, bday, anniv).
        returnCode, card_ptr = createCard_c(full_path)
        if returnCode != 0 or card_ptr is None:
            ##print(f"Error: Could not create card for file '{filename}' (code {returnCode}).")
            return
        validCode = validateCard_c(card_ptr)
        if validCode != 0:
            ##print(f"Error: Card '{filename}' is invalid (validateCard code {validCode}).")
            return
        # Extract relevant data from the Card to store in DB.
        # If these are empty strings, we might store None in the DB
        fn = get_fn(card_ptr)
        bday = get_birthday(card_ptr)
        anniv = get_anniversary(card_ptr)
        # If empty, store as None so we can insert a proper NULL into DATETIME columns:
        bday_val = bday if bday else None
        anniv_val = anniv if anniv else None

        # Build timestamps for DB:
        # creation_time: we usually set it to "now" if the file is brand-new in DB
        # last_modified: get from the OS (the file's actual last-mod time)
        creation_time = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        
        # Get OS-level last modified time for this file, convert to a DATETIME string
        file_mod_ts = os.path.getmtime(full_path)  
        last_modified = datetime.fromtimestamp(file_mod_ts).strftime("%Y-%m-%d %H:%M:%S")

        # Check if the file already exists in the FILE table.
        # If it does, we update its last_modified; if not, we insert it.
        self.db_manager.cursor.execute(
            "SELECT file_id FROM FILE WHERE file_name=%s",
            (filename,)
        )
        row = self.db_manager.cursor.fetchone()

        #If row is not None, the file already exists in the FILE table
        if row: 
            # If the file already in DB, we update last_modified
            file_id = row[0]
            self.db_manager.update_file(file_id, last_modified)
        else:
            # Insert brand-new FILE row
            file_id = self.db_manager.insert_file(filename, creation_time, last_modified)

        # Check if a CONTACT row for this file already exists.
        # If it exists, we do an UPDATE. If not, we do an INSERT.
        self.db_manager.cursor.execute(
            "SELECT contact_id FROM CONTACT WHERE file_id=%s",
            (file_id,)
        )
        contact_row = self.db_manager.cursor.fetchone()

        #If the contact row exists, we update it
        if contact_row:
            # We have a row, do an UPDATE with new data
            contact_id = contact_row[0] #Get the contact_id from the row
            #Update the contact with the new data
            self.db_manager.update_contact(
                contact_id, 
                name=fn or "Unknown",
                birthday=bday_val,
                anniversary=anniv_val
            )
            ##print(f"Updated existing CONTACT row (contact_id={contact_id}) for file_id={file_id}.")
        else:
            # We do not have a row, INSERT new contact
            new_contact_id = self.db_manager.insert_contact(
                name=fn or "Unknown",
                birthday=bday_val,
                anniversary=anniv_val,
                file_id=file_id
            )
            ##print(f"Inserted new CONTACT row (contact_id={new_contact_id}) for file_id={file_id}.")

        ##print(f"Database updated for file '{filename}'.")

# -------------------------------
# UI Classes using Asciimatics.
# Main view: shows list of valid vCard files.
class VCardListView(Frame):
    def __init__(self, screen, model):
        super(VCardListView, self).__init__(
            screen,
            screen.height * 2 // 3,
            screen.width * 2 // 3,
            on_load=self._reload_list,
            hover_focus=True,
            can_scroll=False,
            title="vCard List"
        )
        self._model = model
        self._list_view = ListBox(
            Widget.FILL_FRAME,
            model.get_vcard_list(),
            name="vcard_list",
            add_scroll_bar=True,
            on_change=self._on_pick,
            on_select=self._edit
        )
        self._create_button = Button("Create", self._create)
        self._edit_button = Button("Edit", self._edit)
        self._db_button = Button("DB Queries", self._db_queries)
        self._exit_button = Button("Exit", self._quit)
        layout = Layout([100], fill_frame=True)
        self.add_layout(layout)
        layout.add_widget(self._list_view)
        layout.add_widget(Divider())
        layout2 = Layout([1, 1, 1, 1])
        self.add_layout(layout2)
        layout2.add_widget(self._create_button, 0)
        layout2.add_widget(self._edit_button, 1)
        layout2.add_widget(self._db_button, 2)
        layout2.add_widget(self._exit_button, 3)
        self.fix()
        self._on_pick()

    #Reload the list of vCard files
    def _reload_list(self, new_value=None):
        self._list_view.options = self._model.get_vcard_list()
        if new_value:
            self._list_view.value = new_value

    #On pick function to enable/disable the edit button
    def _on_pick(self):
        self._edit_button.disabled = (self._list_view.value is None)

    #Create button function for when the create button is pressed
    def _create(self):
        ##print("Main View: Create button pressed")
        self._model.current_data = {
            "filename": "",
            "contactName": "",
            "birthday": "",  # Uneditable in Create view.
            "anniversary": "",
            "otherProps": 0
        }
        raise NextScene("Create") #Call the Create scene

    #Edit button function for when the edit button is pressed
    def _edit(self):
        ##print("Main View: Edit button pressed")
        self.save() #Save the current field values into self.data
        selected_file = self.data["vcard_list"] #Get the selected file from the list
        if selected_file:
            self._model.load_vcard(selected_file)
            raise NextScene("Edit")

    #DB Queries button function for when the DB Queries button is pressed
    def _db_queries(self):
        ##print("Main View: DB Queries button pressed")
        raise NextScene("DBQueries")

    @staticmethod
    def _quit():
        ##print("Main View: Exit button pressed")
        raise StopApplication("User pressed quit")

# Create view: for new cards (birthday and anniversary are non-editable labels).
class VCardCreateView(Frame):
    def __init__(self, screen, model):
        super(VCardCreateView, self).__init__(
            screen,
            screen.height * 2 // 3,
            screen.width * 2 // 3,
            hover_focus=True,
            can_scroll=False,
            title="New vCard Details"
        )
        self._model = model
        layout = Layout([100], fill_frame=True)
        self.add_layout(layout)
        self._filename = Text("File name:", "filename", max_length=50)
        layout.add_widget(self._filename)
        self._contact = Text("Contact:", "contactName", max_length=50)
        layout.add_widget(self._contact)
        # Birthday and Anniversary as non-editable labels.
        self._birthday = Label("Birthday: (non-editable)")
        layout.add_widget(self._birthday)
        self._anniv = Label("Anniversary: (non-editable)")
        layout.add_widget(self._anniv)
        self._other = Label("Other properties: 0")
        layout.add_widget(self._other)
        # Add an error label for displaying validation messages.
        self._error_label = Label("", align="^")
        layout.add_widget(self._error_label)
        
        layout2 = Layout([1, 1])
        self.add_layout(layout2)
        layout2.add_widget(Button("OK", self._ok), 0)
        layout2.add_widget(Button("Cancel", self._cancel), 1)
        self.fix()

    #Reset function to reset the fields in the view
    def reset(self):
        super(VCardCreateView, self).reset()
        data = self._model.current_data
        self._filename.value = data.get("filename", "")
        self._contact.value = data.get("contactName", "")
        self._birthday.text = f"Birthday: {data.get('birthday', '')}"
        self._anniv.text = f"Anniversary: {data.get('anniversary', '')}"
        self._other.text = f"Other properties: {data.get('otherProps', 0)}"
    #Ok function for when the OK button is pressed
    def _ok(self):
        self.save()  # Save the current field values into self.data
        data = self.data
        
        # Basic input validation: ensure filename and contact name are provided.
        if data["filename"].strip() == "":
            self._error_label.text = "Error: Filename cannot be empty."
            self._error_label.update(None)  # Update the widget on screen
            return  # Do not proceed
        
        if data["contactName"].strip() == "":
            self._error_label.text = "Error: Contact Name is required."
            self._error_label.update(None)
            return
        
        # Create a new card using our C function wrappers.
        new_card_ptr = new_card_c()
        if not new_card_ptr:
            self._error_label.text = "Error: Could not create a new card."
            self._error_label.update(None)
            return
        
        # Update the FN field in the new card.
        if update_fn(new_card_ptr, data["contactName"]) != 0:
            self._error_label.text = "Error: Failed to update FN field."
            self._error_label.update(None)
            return
        
        # Validate the new card.
        if validateCard_c(new_card_ptr) != 0:
            self._error_label.text = "Error: The vCard is invalid. Please check your input."
            self._error_label.update(None)
            return
        
        # If valid, write the card to disk.
        file_path = os.path.join("cards", data["filename"])
        ret = writeCard_c(file_path, new_card_ptr)
        if ret != 0:
            self._error_label.text = "Error: Failed to write the vCard to disk."
            self._error_label.update(None)
            return
        
        # Otherwise, proceed as normal.
        self._model.create_new_vcard(data)
        self._model.load_vcard(data["filename"])
        raise NextScene("Main")

    @staticmethod
    def _cancel():
        ##print("Create View: Cancel button pressed")
        raise NextScene("Main")

# Edit view: for existing cards (all fields are editable).
class VCardEditView(Frame):
    def __init__(self, screen, model):
        super(VCardEditView, self).__init__(
            screen,
            screen.height * 2 // 3,
            screen.width * 2 // 3,
            hover_focus=True,
            can_scroll=False,
            title="Edit vCard Details"
        )
        self._model = model
        layout = Layout([100], fill_frame=True)
        self.add_layout(layout)

        self._filename = Text("File name:", "filename", max_length=50)
        layout.add_widget(self._filename)

        self._contact = Text("Contact:", "contactName", max_length=50)
        layout.add_widget(self._contact)

        self._birthday = Text("Birthday:", "birthday", max_length=10)
        layout.add_widget(self._birthday)

        self._anniv = Text("Anniversary:", "anniversary", max_length=10)
        layout.add_widget(self._anniv)

        self._other = Label("Other properties: 0")
        layout.add_widget(self._other)

        self._error_label = Label("", align="^")  # Validation message
        layout.add_widget(self._error_label)

        layout2 = Layout([1, 1])
        self.add_layout(layout2)
        layout2.add_widget(Button("OK", self._ok), 0)
        layout2.add_widget(Button("Cancel", self._cancel), 1)

        self.fix()

    def reset(self):
        super(VCardEditView, self).reset()
        data = self._model.current_data
        self._filename.value = data.get("filename", "")
        self._contact.value = data.get("contactName", "")
        self._birthday.value = data.get("birthday", "")
        self._anniv.value = data.get("anniversary", "")
        self._other.text = f"Other properties: {data.get('otherProps', 0)}"
        self._error_label.text = ""  # Clear previous error messages

    def _ok(self):
        self.save()
        data = self.data

        # Validation
        if data["filename"].strip() == "":
            self._error_label.text = "Error: Filename cannot be empty."
            self._error_label.update(None)
            return

        if data["contactName"].strip() == "":
            self._error_label.text = "Error: Contact Name is required."
            self._error_label.update(None)
            return

        if self._model.current_card_ptr is not None:
            update_fn(self._model.current_card_ptr, data["contactName"])
            update_birthday(self._model.current_card_ptr, data["birthday"])
            update_anniversary(self._model.current_card_ptr, data["anniversary"])

            file_path = os.path.join("cards", data["filename"])
            ret = writeCard_c(file_path, self._model.current_card_ptr)
            if ret != 0:
                self._error_label.text = "Error: Failed to save vCard to file."
                self._error_label.update(None)
                return

            self._model.load_vcard(data["filename"])
            raise NextScene("Main")
        else:
            self._error_label.text = "Error: No vCard loaded for editing."
            self._error_label.update(None)

    @staticmethod
    def _cancel():
        raise NextScene("Main")

# Database Queries view.
class DBQueriesView(Frame):
    def __init__(self, screen, model):
        super(DBQueriesView, self).__init__(
            screen,
            screen.height * 2 // 3,
            screen.width * 2 // 3,
            hover_focus=True,
            can_scroll=False,
            title="DB Queries"
        )
        self._model = model
        layout = Layout([100], fill_frame=True)
        self.add_layout(layout)
        self._results_box = TextBox(
            Widget.FILL_FRAME,
            label="Query Results:",
            name="results",
            as_string=True,
            line_wrap=True
        )
        layout.add_widget(self._results_box)
        layout.add_widget(Divider())
        layout2 = Layout([1, 1, 1])
        self.add_layout(layout2)
        layout2.add_widget(Button("Display all contacts", self._display_all), 0)
        layout2.add_widget(Button("Find contacts born in June", self._find_june), 1)
        layout2.add_widget(Button("Cancel", self._cancel), 2)
        self.fix()

    def reset(self):
        super(DBQueriesView, self).reset()
        self._results_box.value = ""

    #When the Display all contacts button is clicked, call the db_display_all function
    def _display_all(self):
        ##print("DBQueriesView: Display all contacts button clicked")
        results = self._model.db_display_all()
        self._results_box.value = results
        #self._results_box.save()

    #When the Find contacts born in June button is clicked, call the db_find_june_birthdays function
    def _find_june(self):
        ##print("DBQueriesView: Find contacts born in June button clicked")
        results = self._model.db_find_june_birthdays()
        self._results_box.value = results
        #self._results_box.save()

    @staticmethod
    def _cancel():
        ##print("DBQueriesView: Cancel button clicked")
        raise NextScene("Main")

# ---------------------------------------------------------------------------
# LoginView: The first scene 
class LoginView(Frame):
    def __init__(self, screen):
        self._last_arguments = None  # Stores the db_manager on success
        super(LoginView, self).__init__(
            screen,
            screen.height * 2 // 3,
            screen.width * 2 // 3,
            hover_focus=True,
            can_scroll=False,
            title="Login"
        )
        layout = Layout([100], fill_frame=True)
        self.add_layout(layout)

        # Username
        self._username = Text("Username:", "username", max_length=50)
        layout.add_widget(self._username)

        # Password
        # We can use hide_char to mask input:
        self._password = Text("Password:", "password", max_length=50, hide_char="*")
        layout.add_widget(self._password)

        # DB name
        self._dbname = Text("Database:", "dbname", max_length=50)
        layout.add_widget(self._dbname)

        # An error label for messages if login fails
        self._error_label = Label("")
        layout.add_widget(self._error_label)

        layout2 = Layout([1,1])
        self.add_layout(layout2)
        layout2.add_widget(Button("OK", self._ok), 0)
        layout2.add_widget(Button("Cancel", self._cancel), 1)

        self.fix()

        # We'll store the successful DB manager here if the login works.
        self.db_manager = None

    #When the OK button is clicked, we try to connect to the database
    def _ok(self):
        self.save()
        username = self.data.get("username", "").strip()
        password = self.data.get("password", "").strip()
        dbname   = self.data.get("dbname", "").strip()
        #If the username or dbname is empty, we display an error message
        if not username or not dbname:
            self._error_label.text = "Error: Username and DB name are required."
            self._error_label.update(None)
            return
        #Try to connect to the database using the given values
        # We do this by creating a DatabaseManager instance and calling connect
        db_manager = DatabaseManager(host="dursley.socs.uoguelph.ca",
                                     database=dbname,
                                     user=username,
                                     password=password)
        #If there is an error connecting to the database, we display an error message
        if not db_manager.connect():
            self._error_label.text = "Login failed. Please check credentials."
            self._error_label.update(None)
            return
        
        # If login succeeded, assign the global variable:
        self._last_arguments = (db_manager,)  # Store the db_manager for the next scene
        raise StopApplication("Login success")  
    
    @staticmethod
    def _cancel():
        raise StopApplication("Login cancelled")

# -------------------------------
# Scene management: Set up and run the UI scenes.
def demo(screen, scene):
    # First, we login
    login_frame = LoginView(screen) #The login frame is created and displayed
    login_scene = Scene([login_frame], -1, name="Login") #Play the login scene
    screen.play([login_scene], stop_on_resize=True, start_scene=scene, allow_int=True)

    # If login was cancelled or failed
    if not login_frame._last_arguments:
        return

    # If login was successful, then our database manager is set to the last arguments
    #DB manager is the first argument in the last arguments tuple, and it holds the database manager instance
    db_manager = login_frame._last_arguments[0]  # Use the actual DB manager returned
    #Then create the VCardModel instance, using the db_manager
    model = VCardModel(db_manager)

    #Create the scenes for the main view, create view, edit view, and db queries view
    scenes = [
        Scene([VCardListView(screen, model)], -1, name="Main"),
        Scene([VCardCreateView(screen, model)], -1, name="Create"),
        Scene([VCardEditView(screen, model)], -1, name="Edit"),
        Scene([DBQueriesView(screen, model)], -1, name="DBQueries"),
    ]

    screen.play(scenes, stop_on_resize=True, start_scene=scenes[0], allow_int=True)

if __name__ == "__main__":
    last_scene = None
    while True:
        try:
            Screen.wrapper(demo, catch_interrupt=True, arguments=[last_scene])
            sys.exit(0)
        except ResizeScreenError as e:
            last_scene = e.scene

"""     # Create a DatabaseManager instance.
    # In a real app, you would obtain these credentials via a login view.
    db_manager = DatabaseManager(host="dursley.socs.uoguelph.ca", database="jgapasin", user="jgapasin", password="1266761")
    if not db_manager.connect():
        print("Failed to connect to the database. Exiting.")
        sys.exit(1)
    model = VCardModel(db_manager)
    scenes = [
        Scene([VCardListView(screen, model)], -1, name="Main"),
        Scene([VCardCreateView(screen, model)], -1, name="Create"),
        Scene([VCardEditView(screen, model)], -1, name="Edit"),
        Scene([DBQueriesView(screen, model)], -1, name="DBQueries")
    ]
    screen.play(scenes, stop_on_resize=True, start_scene=scene, allow_int=True)

if __name__ == "__main__":
    last_scene = None
    while True:
        try:
            Screen.wrapper(demo, catch_interrupt=True, arguments=[last_scene])
            sys.exit(0)
        except ResizeScreenError as e:
            last_scene = e.scene """