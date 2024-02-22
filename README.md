# **Linux File System Implementation with Shell**
### **Overview**
This project is a simple implementation of a Linux file system using shell scripting. 
It provides basic functionalities such as navigating through directories (cd command), 
listing directory contents (ls command), creating filesystem (createfs command), accessing command history (history command) and much more.

### **Features**
* Multiple commands created for user interface with shell, filesystem, and files.
* The filesystem uses an index allocation scheme.
* The filesystem supports files up to 220 bytes in size.
* The filesystem supports up to 256 files.
* The filesystem supports filenames of up to 64 characters.
* Supported file names are alphanumeric with or without “.”.
* The directory structure is a single level hierarchy with no subdirectories.

### **Requirements**
* Linux operating system or a compatible environment (e.g., WSL on Windows)
* Shell environment (bash or compatible)

### **Installation**
* Clone or download the repository to your local machine.
* Ensure that you have a compatible shell environment (e.g., bash).
* Navigate to the directory containing the project file.

### **Usage**
* Open a terminal window.
* Navigate to the directory where the project files are located.
* Compile and run the msh.sh script using the following command:
gcc -o msh msh.c
./msh
* Once executed, you can use the following commands:
  * q / quit / exit: Exit the shell.
  * cd <directory>: Change directory to <directory>.
  * history <-p>: Display a list of 15 or <-p> previously executed commands.
  * createfs <filename>: Create a new filesystem image named <filename>.
  * savefs: Write the currently opened filesystem to its file.
  * open <filename>: Open a filesystem image <filename>.
  * close: Close the opened filesystem.
  * list <-h> <-a>: List the files in the filesystem image. If the <-h> parameter is given it will also list hidden files. If the <-a> parameter is provided the attributes will also be listed with the file and displayed as an 8-bit binary value.
  * df: Display the amount of disk space left in the filesystem image.
  * insert <filename>: Copy file into filesystem.
  * attrib <+attribute> <-attribute> <filename>: Adds or removes selected attribute to file. Attributes include <h> hidden and <r> read-only.
  * encrypt <filename> <cipher>:  XOR encrypt the file <filename> with cipher <cipher>. The cipher is limited to a 1-byte value.
  * decrypt <filename> <cipher>:  XOR Decrypt the file <filename> with cipher <cipher>. The cipher is limited to a 1-byte value.
  * read <filename> <startbyte> <numberofbytes>: Read the file <filename> from <startbyte> and stop after <numberofbytes>.
  * delete <filename>: Delete file <filename>.
  * undelete <filename>: Recover file <filename>.
  * retrieve <filename> <newfilename>: Retrieve the file <filename> from the filesystem image and place it in the current working directory, using name <filename> or <newfilename>.
  
Follow the on-screen prompts to navigate through commands.
