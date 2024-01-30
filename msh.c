#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <stdint.h>

#define WHITESPACE " \t\n"      // We want to split our command line up into tokens
                                // so we need to define what delimits our tokens.
                                // In this case  white space
                                // will separate the tokens on our command line

#define MAX_COMMAND_SIZE 255    // The maximum command-line size

#define MAX_NUM_ARGUMENTS 12     // Mav shell only supports eleven arguments

#define MAX_FILENAME_LENGTH 64
#define BLOCK_SIZE 1024
#define NUM_BLOCKS 65536
#define BLOCKS_PER_FILE 1024
#define NUM_FILES 256
#define FIRST_DATA_BLOCK 1001
#define MAX_FILE_SIZE 1048576
#define HIDDEN 0x1
#define READONLY 0x2

uint8_t data[NUM_BLOCKS][BLOCK_SIZE];
uint8_t *free_blocks;
uint8_t *free_inodes;
// directory
struct directory_entry
{
  char filename[64];
  short in_use;
  int32_t inode;
};

struct directory_entry * directory;

//inode
struct inode
{
  int32_t blocks[BLOCKS_PER_FILE];
  short in_use;
  uint8_t attribute;
  uint32_t file_size;
};

struct inode * inodes;

FILE *fp;
char image_name[64];
uint8_t image_open;

int findFreeBlock()
{
  int i;
  for(i = 0; i < NUM_BLOCKS; i++)
  {
    if(free_blocks[i])
    {
        return i + 1001;
    }
  }
  return -1;
}

int findFreeInode()
{
  int i;
  for(i = 0; i < NUM_FILES; i++)
  {
    if(free_inodes[i])
    {
        return i;
    }
  }
  return -1;
}

int findFreeInodeBlock(int32_t inode)
{
  int i;
  for(i = 0; i < BLOCKS_PER_FILE; i++)
  {
    if(inodes[inode].blocks[i] == -1)
    {
        return i;
    }
  }
  return -1;
}

void init()
{
	directory = (struct directory_entry*)&data[0][0];
	inodes = (struct inode*)&data[20][0];
  free_blocks = (uint8_t *) &data[1000][0];
  free_inodes = (uint8_t *) &data[19][0];
  memset(image_name, 0, 64);
  image_open = 0;
	int i;
	for(i = 0; i < NUM_FILES; i++)
	{
		directory[i].in_use = 0;
    directory[i].inode = -1;
    free_inodes[i] = 1;
    memset(directory[i].filename, 0, 64);

		int j;
		for(j = 0; j < NUM_BLOCKS; j++)
		{
			inodes[i].blocks[j] = -1;
			inodes[i].in_use = 0;
      inodes[i].attribute = 0;
      inodes[i].file_size = 0;
		}
	} 
  int j;
  for(j = 0; j < NUM_BLOCKS; j++)
  {
    free_blocks[j] = 1;
  }
}

uint32_t df()
{
  int j;
  int count = 0;
  for(j = FIRST_DATA_BLOCK; j < NUM_BLOCKS; j++)
  {
    if(free_blocks[j])
    {
      count++;
    }
  }
  return count * BLOCK_SIZE;
}

void createfs(char * filename)
{
	fp = fopen(filename, "w");
  strncpy(image_name, filename, strlen(filename));
	memset( data, 0, NUM_BLOCKS*BLOCK_SIZE);
  image_open = 1;
  int i;
	for(i = 0; i < NUM_FILES; i++)
	{
		directory[i].in_use = 0;
    directory[i].inode = -1;
    free_inodes[i] = 1;
    memset(directory[i].filename, 0, 64);

		int j;
		for(j = 0; j < NUM_BLOCKS; j++)
		{
			inodes[i].blocks[j] = -1;
			inodes[i].in_use = 0;
      inodes[i].attribute = 0;
      inodes[i].file_size = 0;
		}
	} 
  int j;
  for(j = 0; j < NUM_BLOCKS; j++)
  {
    free_blocks[j] = 1;
  }
	fclose(fp);
}

void savefs()
{
  if(image_open == 0)
  {
    printf("Disk image is not open!\n");
    return;
  }
	fp = fopen(image_name, "w");
	fwrite(&data[0][0], BLOCK_SIZE, NUM_BLOCKS,fp);
  memset(image_name, 0, 64);
	fclose(fp);
}

void openfs(char * filename)
{
  fp = fopen(filename, "r");
  strncpy(image_name, filename, strlen(filename));
	fread(&data[0][0], BLOCK_SIZE, NUM_BLOCKS, fp);
  image_open = 1;
	//fclose(fp);
}

void closefs()
{
  if(image_open == 0)
  {
    printf("ERROR: Disk Image is not Open!\n");
    return;
  }
  fclose(fp);
  image_open = 0;
  memset(image_name, 0, 64);
}

void list()
{
  int i;
  int not_found = 1;
  for (i = 0; i < NUM_FILES; i++)
  {
    if(directory[i].in_use)
    {
      if(!(inodes[i].attribute & HIDDEN))
      {
        not_found = 0;
        char filename[64];
        memset(filename, 0, 65);
        strncpy(filename, directory[i].filename, strlen(directory[i].filename));
        printf("%s\n", filename);
      }
    }
  }
  if(not_found)
  {
    printf("ERROR: No Files Found!\n");
  }
}
void listarg(char *param)
{
  int i;
  int none = 1;
  int not_found = 1;
  for (i = 0; i < NUM_FILES; i++)
  {
    if(directory[i].in_use)
    {
      if (param && strcmp(param, "-h") == 0)
      {
        not_found = 0;
        char filename[64];
        memset(filename, 0, 65);
        strncpy(filename, directory[i].filename, strlen(directory[i].filename));
        printf("%s\n", filename);
      }
      else if (param && strcmp(param, "-a") == 0)
      {
        not_found = 0;
        char filename[64];
        memset(filename, 0, 65);
        strncpy(filename, directory[i].filename, strlen(directory[i].filename));
        printf("%s has attribute(s):", filename);
        if(inodes[i].attribute & HIDDEN)
        {
          printf(" Hidden");
          none = 0;
        }
        if(inodes[i].attribute & READONLY)
        {
          printf(" READ ONLY");
          none = 0;
        }       

        if(none)
        {
          printf(" NONE");
        }
        printf("\n");
      }
    }
  }
  if(not_found)
  {
    printf("ERROR: No Files Found!\n");
  }
}

void insert(char *filename)
{
  // verify not null, then if file exist, then if file is not too large
  // then if there is enough space, then find empty directory entry 
  if(filename == NULL)
  {
    printf("ERROR: Filename is NULL!\n");
    return;
  }
  if(strlen(filename) > 64)
  {
    printf("ERROR: Filename is too long!\n");
    return;
  }
  struct stat buf; 
  int ret = stat(filename, &buf);
  if(ret == -1)
  {
    printf("ERROR: File does not Exist!\n");
    return;
  } 
  if(buf.st_size > MAX_FILE_SIZE)
  {
    printf("ERROR: File is too Large!\n ");
    return;
  }
  if(buf.st_size > df())
  {
    printf("ERROR: There is not Enough Free Disk Space!\n ");
    return;
  }
  int i;
  int directory_entry = -1;
  for(i = 0; i < NUM_FILES; i++)
  {
    if(directory[i].in_use == 0)
    {
      directory_entry = i;
      break;
    }
  }
  if(directory_entry == -1)
  {
    printf("ERROR: Could not Find a Free Directory Entry!\n");
    return;
  }
  // Open the input file read-only 
  FILE *ifp = fopen ( filename, "r" ); 
  printf("Reading %d bytes from %s\n", (int) buf . st_size, filename );

  // Save off the size of the input file since we'll use it in a couple of places and 
  // also initialize our index variables to zero. 
  int32_t copy_size   = buf . st_size;

  // We want to copy and write in chunks of BLOCK_SIZE. So to do this 
  // we are going to use fseek to move along our file stream in chunks of BLOCK_SIZE.
  // We will copy bytes, increment our file pointer by BLOCK_SIZE and repeat.
  int32_t offset      = 0;               

  // We are going to copy and store our file in BLOCK_SIZE chunks instead of one big 
  // memory pool. Why? We are simulating the way the file system stores file data in
  // blocks of space on the disk. block_index will keep us pointing to the area of
  // the area that we will read from or write to.
  int32_t block_index = -1;

  //find free inode
    int32_t inode_index = findFreeInode();
    if(inode_index == -1)
    {
      printf("ERROR: Could not Find a Free Inode!\n");
      return;
    }

    //place file info in the directory
    directory[directory_entry].in_use = 1;
    directory[directory_entry].inode = inode_index;
    strncpy(directory[directory_entry].filename, filename, strlen(filename));
    inodes[inode_index].file_size = buf.st_size;

  // copy_size is initialized to the size of the input file so each loop iteration we
  // will copy BLOCK_SIZE bytes from the file then reduce our copy_size counter by
  // BLOCK_SIZE number of bytes. When copy_size is less than or equal to zero we know
  // we have copied all the data from the input file.
  while( copy_size > 0 )
  {
    // Index into the input file by offset number of bytes.  Initially offset is set to
    // zero so we copy BLOCK_SIZE number of bytes from the front of the file.  We 
    // then increase the offset by BLOCK_SIZE and continue the process.  This will
    // make us copy from offsets 0, BLOCK_SIZE, 2*BLOCK_SIZE, 3*BLOCK_SIZE, etc.
    fseek( ifp, offset, SEEK_SET ); 

    //find a free block
    block_index = findFreeBlock();
    if(block_index == -1)
    {
      printf("ERROR: Could not Find a Free Block!\n");
      return;
    }

  

    // Read BLOCK_SIZE number of bytes from the input file and store them in our
    // data array.
    int32_t bytes  = fread( data[block_index], BLOCK_SIZE, 1, ifp );

    //save the block in the inode
    int32_t inode_block = findFreeInodeBlock(inode_index);
    inodes[inode_index].blocks[inode_block] = block_index;

    // If bytes == 0 and we haven't reached the end of the file then something is 
    // wrong. If 0 is returned and we also have the EOF flag set then that is OK.
    // It means we've reached the end of our input file.
    if( bytes == 0 && !feof( ifp ) )
    {
      printf("ERROR: An error occured reading from the input file.\n");
      return;
    }

    // Clear the EOF file flag.
    clearerr( ifp );

    // Reduce copy_size by the BLOCK_SIZE bytes.
    copy_size -= BLOCK_SIZE;
    
    // Increase the offset into our input file by BLOCK_SIZE.  This will allow
    // the fseek at the top of the loop to position us to the correct spot.
    offset    += BLOCK_SIZE;
    if(block_index != -1)
    {
      for(int i = block_index; i < (block_index+1); i++)
      {
        int j;
        for(j = i; j < BLOCKS_PER_FILE; j++)
        {
          free_blocks[j] = 0;
        }
      }
    }
    block_index = findFreeBlock();
  }

  // We are done copying from the input file so close it out.
  fclose( ifp );
}

void attribute(char *attrib, char *filename)
{
  struct stat buf; 
  int ret = stat(filename, &buf);
  if(ret == -1)
  {
    printf("ERROR: File not Found!\n");
    return;
  }
  for(int i = 0; i < NUM_FILES; i++)
  {
    if(strcmp(directory[i].filename, filename) == 0)
    {
      if(strcmp(attrib, "+h") == 0 || strcmp(attrib, "-h") == 0)
      {
        inodes[i].attribute |= HIDDEN;
        if(inodes[i].attribute & HIDDEN)
        {
          printf("File: %s was hidden\n", directory[i].filename);
        }
        return;
      }
      else if(strcmp(attrib, "+r") == 0 || strcmp(attrib, "-r") == 0)
      {
        inodes[i].attribute |= READONLY;
        return;
      }
    }
  }
}

void _crypt(char *filename, char* cipher)
{
  if(filename == NULL)
  {
    printf("ERROR: Filename is NULL!\n");
    return;
  }
  struct stat buf; 
  int ret = stat(filename, &buf);
  if(ret == -1)
  {
    printf("ERROR: File not Found!\n");
    return;
  }
  int curr;
  fp = fopen(filename, "rb+");
  if(fp == NULL)
  {
    printf("ERROR: Unable to open File!\n");
    return;
  }
  while((curr = fgetc(fp)) != EOF)
  {
    curr ^= atoi(cipher);
    fseek(fp, -1, SEEK_CUR);
    fputc(curr, fp);
    fseek(fp, 0, SEEK_CUR);
  }
	fclose(fp);
}
void _read(char *filename, char* start_byte, char* num_byte)
{
  if(filename == NULL)
  {
    printf("ERROR: Filename is NULL!\n");
    return;
  }
  struct stat buf; 
  int ret = stat(filename, &buf);
  if(ret == -1)
  {
    printf("ERROR: File not Found!\n");
    return;
  }
  int curr;
  int i = 0;
  fp = fopen(filename, "rb");
  if(fp == NULL)
  {
    printf("ERROR: Unable to open File!\n");
    return;
  }
  printf("From file: %s, starting from byte %d\n", filename, atoi(start_byte));
  fseek(fp, atoi(start_byte), SEEK_CUR);
  while(((curr = fgetc(fp)) != EOF) && (i < atoi(num_byte)))
  {
    printf("%x", curr);
    i++;
  }
  printf("\n");
	fclose(fp);
}

void delete(char *filename)
{
  if(filename == NULL)
  {
    printf("ERROR: Filename is NULL!\n");
    return;
  }
  struct stat buf; 
  int ret = stat(filename, &buf);
  if(ret == -1)
  {
    printf("ERROR: File not Found!\n");
    return;
  }
  for(int i = 0; i < NUM_FILES; i++)
  {
    if(strcmp(directory[i].filename, filename) == 0)
    {
      directory[i].in_use = 0;
      inodes[i].in_use = 0;
      int j;
      for(j = i; j < BLOCKS_PER_FILE; j++)
      {
        free_blocks[j] = 1;
      }
    }
  }
}

void undelete(char *filename)
{
  if(filename == NULL)
  {
    printf("ERROR: Filename is NULL!\n");
    return;
  }
  int ret = 1;
  for(int i = 0; i < NUM_FILES; i++)
  {
    if(strcmp(directory[i].filename, filename) != 0)
    {
      ret = 0;
    }
    else
    {
      ret = 1;
      break;
    }
    if(ret == 1)
    {
      break;
    }
  }
  if(ret == 0)
  {
    printf("Error: Can not find the file!\n");
    return;
  }
  for(int i = 0; i < NUM_FILES; i++)
  {
    if(strcmp(directory[i].filename, filename) == 0)
    {
      directory[i].in_use = 1;
      inodes[i].in_use = 1;
      int j;
      for(j = i; j < BLOCKS_PER_FILE; j++)
      {
        free_blocks[j] = 0;
      }
    }
  }
}

void retrieve(char *filename)
{
  if(filename == NULL)
  {
    printf("ERROR: Filename is NULL!\n");
    return;
  }
  struct stat buf; 
  int ret = stat(filename, &buf);
  if(ret == -1)
  {
    printf("ERROR: File not Found!\n");
    return;
  }
  int i;
  int directory_entry = -1;
  for(i = 0; i < NUM_FILES; i++)
  {
    if(directory[i].in_use == 0)
    {
      directory_entry = i;
      break;
    }
  }
  if(directory_entry == -1)
  {
    printf("ERROR: Could not Find a Free Directory Entry!\n");
    return;
  }
  // Now, open the output file that we are going to write the data to.
  FILE *ofp;
  ofp = fopen(filename, "w+");

  if( ofp == NULL )
  {
    printf("Could not open output file: %s\n", filename );
    perror("Opening output file returned");
    return;
  }
  
  // Initialize our offsets and pointers just we did above when reading from the file.
  int block_index = -1;
  int copy_size   = buf . st_size;
  int offset      = 0;

  printf("Writing %d bytes to %s\n", (int) buf . st_size, filename );

  //find free inode
  int32_t inode_index = findFreeInode();
  if(inode_index == -1)
  {
    printf("ERROR: Could not Find a Free Inode!\n");
    return;
  }

  //place file info in the directory
  directory[directory_entry].in_use = 1;
  directory[directory_entry].inode = inode_index;
  strncpy(directory[directory_entry].filename, filename, strlen(filename));
  inodes[inode_index].file_size = buf.st_size;

  // Using copy_size as a count to determine when we've copied enough bytes to the output file.
  // Each time through the loop, except the last time, we will copy BLOCK_SIZE number of bytes from
  // our stored data to the file fp, then we will increment the offset into the file we are writing to.
  // On the last iteration of the loop, instead of copying BLOCK_SIZE number of bytes we just copy
  // how ever much is remaining ( copy_size % BLOCK_SIZE ).  If we just copied BLOCK_SIZE on the
  // last iteration we'd end up with gibberish at the end of our file. 
  while( copy_size > 0 )
  { 

    int num_bytes;

    // If the remaining number of bytes we need to copy is less than BLOCK_SIZE then
    // only copy the amount that remains. If we copied BLOCK_SIZE number of bytes we'd
    // end up with garbage at the end of the file.
    if( copy_size < BLOCK_SIZE )
    {
      num_bytes = copy_size;
    }
    else 
    {
      num_bytes = BLOCK_SIZE;
    }

    block_index = findFreeBlock();
    if(block_index == -1)
    {
      printf("ERROR: Could not Find a Free Block!\n");
      return;
    }

    // Write num_bytes number of bytes from our data array into our output file.
    fwrite( data[block_index], num_bytes, 1, ofp ); 

    // Reduce the amount of bytes remaining to copy, increase the offset into the file
    // and increment the block_index to move us to the next data block.
    copy_size -= BLOCK_SIZE;
    offset    += BLOCK_SIZE;
    block_index = findFreeBlock();

    // Since we've copied from the point pointed to by our current file pointer, increment
    // offset number of bytes so we will be ready to copy to the next area of our output file.
    fseek( ofp, offset, SEEK_SET );
  }

  // Close the output file, we're done. 
  fclose( ofp );
}

void retrieve_new(char *filename1, char *filename2)
{
  if(filename1 && filename2 == NULL)
  {
    printf("ERROR: Filename is NULL!\n");
    return;
  }
  struct stat buf; 
  int ret = stat(filename1, &buf);
  if(ret == -1)
  {
    printf("ERROR: File not Found!\n");
    return;
  }

  int i;
  int directory_entry = -1;
  for(i = 0; i < NUM_FILES; i++)
  {
    if(directory[i].in_use == 0)
    {
      directory_entry = i;
      break;
    }
  }
  if(directory_entry == -1)
  {
    printf("ERROR: Could not Find a Free Directory Entry!\n");
    return;
  }
  FILE *ifp = fopen ( filename1, "r" );
  if( ifp == NULL )
    {
      printf("Could not open input file: %s\n", filename1 );
      perror("Opening input file returned");
      return;
    } 
  // Now, open the output file that we are going to write the data to.
    FILE *ofp;
    ofp = fopen(filename2, "w+");

    if( ofp == NULL )
    {
      printf("Could not open output file: %s\n", filename2 );
      perror("Opening output file returned");
      return;
    }
    printf("Writing %d bytes to %s\n", (int) buf . st_size, filename2 );
    
    // Initialize our offsets and pointers just we did above when reading from the file.
    int block_index = -1;
    int copy_size   = buf . st_size;
    int offset      = 0;
    //find free inode
    int32_t inode_index = findFreeInode();
    if(inode_index == -1)
    {
      printf("ERROR: Could not Find a Free Inode!\n");
      return;
    }

    //place file info in the directory
    directory[directory_entry].in_use = 1;
    directory[directory_entry].inode = inode_index;
    strncpy(directory[directory_entry].filename, filename2, strlen(filename2));
    inodes[inode_index].file_size = buf.st_size;
    

    // Using copy_size as a count to determine when we've copied enough bytes to the output file.
    // Each time through the loop, except the last time, we will copy BLOCK_SIZE number of bytes from
    // our stored data to the file fp, then we will increment the offset into the file we are writing to.
    // On the last iteration of the loop, instead of copying BLOCK_SIZE number of bytes we just copy
    // how ever much is remaining ( copy_size % BLOCK_SIZE ).  If we just copied BLOCK_SIZE on the
    // last iteration we'd end up with gibberish at the end of our file. 
    while( copy_size > 0 )
    { 

      int num_bytes;

      // If the remaining number of bytes we need to copy is less than BLOCK_SIZE then
      // only copy the amount that remains. If we copied BLOCK_SIZE number of bytes we'd
      // end up with garbage at the end of the file.
      if( copy_size < BLOCK_SIZE )
      {
        num_bytes = copy_size;
      }
      else 
      {
        num_bytes = BLOCK_SIZE;
      }

      block_index = findFreeBlock();
      if(block_index == -1)
      {
        printf("ERROR: Could not Find a Free Block!\n");
        return;
      }

      int32_t bytes  = fread( data[block_index], BLOCK_SIZE, 1, ifp );
      // Write num_bytes number of bytes from our data array into our output file.
      fwrite( data[block_index], num_bytes, 1, ofp ); 
      //save the block in the inode
      int32_t inode_block = findFreeInodeBlock(inode_index);
      inodes[inode_index].blocks[inode_block] = block_index;

      if( bytes == 0 && !feof( ifp ) )
      {
        printf("ERROR: An error occured reading from the input file.\n");
        return;
      }

      // Clear the EOF file flag.
      clearerr( ifp );

      // Reduce the amount of bytes remaining to copy, increase the offset into the file
      // and increment the block_index to move us to the next data block.
      copy_size -= BLOCK_SIZE;
      offset    += BLOCK_SIZE;
      block_index = findFreeBlock();

      // Since we've copied from the point pointed to by our current file pointer, increment
      // offset number of bytes so we will be ready to copy to the next area of our output file.
      fseek( ofp, offset, SEEK_SET );
    }

    // Close the output file, we're done. 
    fclose( ofp );
    fclose( ifp );
}

int main()
{

  char * command_string = (char*) malloc( MAX_COMMAND_SIZE );
	fp = NULL;
  init();
  // History and pid array max size
  int hn = 15;
  // Index variable for history and pid array 
  int n = 0;
  // Declare an array to store history and pids 
  char *hist[hn];
  int pidarr[hn];

  // Empty both arrays
  for( int i = 0; i < hn; i++ )
  {
    hist[i] = NULL;
    pidarr[i] = 0;
  }

  while( 1 )
  {
    // Print out the msh prompt
    printf ("msh> ");

    // Read the command from the commandline.  The
    // maximum command that will be read is MAX_COMMAND_SIZE
    // This while command will wait here until the user
    // inputs something since fgets returns NULL when there
    // is no input
    while( !fgets (command_string, MAX_COMMAND_SIZE, stdin) );

    // New prompt printed if no string input is entered 
    if (command_string[0] == '\n') {
      continue;
    }

    /* Parse input */
    char *token[MAX_NUM_ARGUMENTS];

    for( int i = 0; i < MAX_NUM_ARGUMENTS; i++ )
    {
      token[i] = NULL;
    }

    int   token_count = 0;                                 
                                                           
    // Pointer to point to the token
    // parsed by strsep
    char *argument_ptr = NULL;                                         

    // If command_string[0] == '!'
    // then declare an integer index
    // set index to atoi|( &command_string[1] )
    // strncpy from hist[index] to command_string
    if(command_string[0] == '!')
    {
      int exIndex = atoi(&command_string[1]);
      strcpy(command_string, hist[exIndex]);
    }

    char *working_string  = strdup( command_string );

    // Store entire input string in history
    // move history up so last 15 inputs are always displayed
    hist[n] = strdup(command_string);
    if( n == 15)
    {
      n = n - 1;
      for(int i = 0; i < n; i++)
      {
        hist[i] = hist[i+1];
      }
      hist[n] = strdup(command_string);
    } 

    // we are going to move the working_string pointer so
    // keep track of its original value so we can deallocate
    // the correct amount at the end
    char *head_ptr = working_string;

    // Tokenize the input strings with whitespace used as the delimiter
    while ( ( (argument_ptr = strsep(&working_string, WHITESPACE ) ) != NULL) && 
              (token_count<MAX_NUM_ARGUMENTS))
    {
      token[token_count] = strndup( argument_ptr, MAX_COMMAND_SIZE );
      if( strlen( token[token_count] ) == 0 )
      {
        token[token_count] = NULL;
      }
      token_count++;
    }
    // Store pid in array
    pid_t pid;
    pidarr[n] = pid;

    // Implement built-ins
    // fork pid
    // Call execvp to handle other commands
    if(strcmp(token[0], "q") == 0)
    {
      exit(0);
    }
    else if (strcmp(token[0], "quit") == 0)
    {
      exit(0);
    }
    else if (strcmp(token[0], "exit") == 0) 
    {
      exit(0);
    }
    else if (strcmp(token[0], "cd") == 0) 
    {
      chdir(token[1]);
    }
    else if (strcmp(token[0], "history") == 0)
    {
      if (token[1] && strcmp(token[1], "-p") == 0)
      {
        for(int i = 0; i < hn; i++)
          {
            if(hist[i] != NULL) 
            {
              printf("%d: %s", i, hist[i]);
              printf("Command %d has pid %d\n", i, pidarr[i]);
            }
          }
      }
      else
      {
          for(int i = 0; i < hn; i++)
          {
            if(hist[i] != NULL) 
            {
              printf("%d: %s", i, hist[i]);
            }
          }
      }
    }
    else if (strcmp(token[0], "createfs") == 0)
    {
			if(token[1] == NULL)
			{
				printf("ERROR: No Filename!\n");
				continue;
			}
      if(strlen(token[1]) > MAX_FILENAME_LENGTH)
      {
        printf("ERROR: File name too long!\n");
        continue;
      }
			createfs(token[1]);
    }
    else if (strcmp(token[0], "savefs") == 0)
    {
			savefs();
    }
    else if (strcmp(token[0], "open") == 0)
    {
			if(token[1] == NULL)
			{
				printf("ERROR: No Filename!\n");
				continue;
			}
      if(strlen(token[1]) > MAX_FILENAME_LENGTH)
      {
        printf("ERROR: File name too long!\n");
        continue;
      }
			openfs(token[1]);
    }
    else if (strcmp(token[0], "close") == 0)
    {
			closefs(token[1]);
    }
    else if (strcmp(token[0], "list") == 0)
    {
			if(!image_open)
			{
				printf("ERROR: Disk Image is not Open!\n");
				continue;
			}
      if(token[1] == NULL)
      {
        list();
        continue;
      }
      if(token[1] != NULL)
			{
				if(strcmp(token[1], "-h") != 0 && strcmp(token[1], "-a") != 0)
        {
          printf("ERROR: Invalid parameter!\n");
        }
        listarg(token[1]);
				continue;
			}
    }
    else if (strcmp(token[0], "df") == 0)
    {
      if(!image_open)
      {
        printf("ERROR: Disk Image is not Open!\n");
        continue; 
      }
			printf("%d bytes free.\n", df());
    }
    else if (strcmp(token[0], "insert") == 0)
    {
      if(!image_open)
      {
        printf("ERROR: Disk Image is not Open!\n");
        continue; 
      }
      if(token[1] == NULL)
			{
				printf("ERROR: No Filename!\n");
				continue;
			}
      if(strlen(token[1]) > MAX_FILENAME_LENGTH)
      {
        printf("ERROR: File name too long!\n");
        continue;
      }
      insert(token[1]);
    }
    else if (strcmp(token[0], "attrib") == 0)
    {
      if(!image_open)
      {
        printf("ERROR: Disk Image is not Open!\n");
        continue; 
      }
      if((token[1] == NULL) && (token[2] == NULL))
			{
				printf("ERROR: No attribute and filename specified!\n");
				continue;
			}
      if(token[1] == NULL)
			{
				printf("ERROR: No attribute specified!\n");
				continue;
			}
      if(strcmp(token[1], "+h") != 0 && strcmp(token[1], "-h") != 0
      && strcmp(token[1], "+r") != 0 && strcmp(token[1], "-r") != 0)
      {
        printf("ERROR: Invalid Attribute!\n");
        printf("%s\n", token[1]);
        continue;
      }
      if(token[2] == NULL)
			{
				printf("ERROR: No Filename!\n");
				continue;
			}
      if(strlen(token[2]) > MAX_FILENAME_LENGTH)
      {
        printf("ERROR: File name too long!\n");
        continue;
      }
      attribute(token[1], token[2]);
    }
    else if (strcmp(token[0], "encrypt") == 0)
    {
      if(!image_open)
      {
        printf("ERROR: Disk Image is not Open!\n");
        continue; 
      }
      if((token[1] == NULL) && (token[2] == NULL))
			{
				printf("ERROR: No filename and cipher specified!\n");
				continue;
			}
      if(token[1] == NULL)
			{
				printf("ERROR: No filename specified!\n");
				continue;
			}
      if(strlen(token[1]) > MAX_FILENAME_LENGTH)
      {
        printf("ERROR: File name too long!\n");
        continue;
      }
      if(token[2] == NULL)
			{
				printf("ERROR: No cipher specified!\n");
				continue;
			}
      _crypt(token[1], token[2]);
    }
    else if (strcmp(token[0], "decrypt") == 0)
    {
      if(!image_open)
      {
        printf("ERROR: Disk Image is not Open!\n");
        continue; 
      }
      if((token[1] == NULL) && (token[2] == NULL))
			{
				printf("ERROR: No filename and cipher specified!\n");
				continue;
			}
      if(token[1] == NULL)
			{
				printf("ERROR: No filename specified!\n");
				continue;
			}
      if(strlen(token[1]) > MAX_FILENAME_LENGTH)
      {
        printf("ERROR: File name too long!\n");
        continue;
      }
      if(token[2] == NULL)
			{
				printf("ERROR: No cipher specified!\n");
				continue;
			}
      _crypt(token[1], token[2]);
    }
    else if (strcmp(token[0], "read") == 0)
    {
      if(!image_open)
      {
        printf("ERROR: Disk Image is not Open!\n");
        continue; 
      }
      if((token[1] == NULL) && (token[2] == NULL))
			{
				printf("ERROR: No filename, starting byte, and number of bytes specified!\n");
				continue;
			}
      if(token[1] == NULL)
			{
				printf("ERROR: No filename specified!\n");
				continue;
			}
      if(strlen(token[1]) > MAX_FILENAME_LENGTH)
      {
        printf("ERROR: File name too long!\n");
        continue;
      }
      if(token[2] == NULL)
			{
				printf("ERROR: No starting byte specified!\n");
				continue;
			}
      if(token[3] == NULL)
			{
				printf("ERROR: Number of bytes not specified!\n");
				continue;
			}
      _read(token[1], token[2], token[3]);
    }
    else if (strcmp(token[0], "delete") == 0)
    {
      if(!image_open)
      {
        printf("ERROR: Disk Image is not Open!\n");
        continue; 
      }
      if(token[1] == NULL)
			{
				printf("ERROR: No filename specified!\n");
				continue;
			}
      if(strlen(token[1]) > MAX_FILENAME_LENGTH)
      {
        printf("ERROR: File name too long!\n");
        continue;
      }
      delete(token[1]);
    }
    else if (strcmp(token[0], "undelete") == 0)
    {
      if(!image_open)
      {
        printf("ERROR: Disk Image is not Open!\n");
        continue; 
      }
      if(token[1] == NULL)
			{
				printf("ERROR: No filename specified!\n");
				continue;
			}
      if(strlen(token[1]) > MAX_FILENAME_LENGTH)
      {
        printf("ERROR: File name too long!\n");
        continue;
      }
      undelete(token[1]);
    }
    else if (strcmp(token[0], "retrieve") == 0)
    {
      if(!image_open)
      {
        printf("ERROR: Disk Image is not Open!\n");
        continue; 
      }
      if(token[1] == NULL)
			{
				printf("ERROR: No filename specified!\n");
				continue;
			}
      if(strlen(token[1]) > MAX_FILENAME_LENGTH)
      {
        printf("ERROR: File name too long!\n");
        continue;
      }
      if(token[2] != NULL)
			{
        if(strlen(token[2]) > MAX_FILENAME_LENGTH)
        {
          printf("ERROR: File name too long!\n");
          continue;
        }
				retrieve_new(token[1], token[2]);
        continue;
			}
      retrieve(token[1]);
    }
    else
    {
      pid = fork();
      if(pid == 0)
      {
        int ret = execvp( token[0], &token[0] );  
        if( ret == -1 )
        {
          printf("%s: Command not found.\n", token[0]);
          return 0;
        }
      }
      else 
      {
        int status;
        wait( & status );
      }
    }

    // Increment inndex variable 
    n++;

    // Cleanup allocated memory
    for( int i = 0; i < MAX_NUM_ARGUMENTS; i++ )
    {
      if( token[i] != NULL )
      {
        free( token[i] );
      }
    }

    free( head_ptr );

  }

  free( command_string );

  return 0;
  // e2520ca2-76f3-90d6-0242ac120003
}