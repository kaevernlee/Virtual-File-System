#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <math.h>

#include "myfilesystem.h"

#define trans *(Directory**)
#define BLOCK_SIZE 256
#define F_POW (pow(2, 32)-1)
#define START_INDEX(x) (pow(2,x)-1)
#define END_INDEX(x) (pow(2,x+1)-2)

typedef struct directory {
	char name[64];
	uint32_t offset, length;
	struct directory* next;
}Directory;

Directory* init_element(char* name, uint32_t offset, uint32_t length, Directory* next) {
	Directory* new = (Directory*) malloc(sizeof(Directory));
	strcpy(new->name, name);
	new->offset = offset;
	new->length = length;
	return new;
}

// gloabl file pointers for access all throughout file
FILE* dtab;
FILE* fdata;
FILE* hash;

// global file size for directory table
int dtab_size;
// global entry size for directory table (bytes / 72)
int entries;

// global file size of file data
int fdata_size;

// global available disk space (total bytes - lengths)
int disk_space;

// init directory table head
Directory* head;

// init mutex for all functions
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;


/*#############################################
######## SELF FILE SYSTEM FUNCTIONS ###########
#############################################*/
/* ordered add by offset */
void list_add(Directory** head, Directory* new) {
	if ((*head == NULL) || ((*head)->offset >= new->offset)) {
		new->next = *head;
		*head = new;
	}
	else {
		Directory* current = *head;
		while ((current->next != NULL) && ((current->next)->offset < new->offset)) {
			current = current->next;
		}
		new->next = current->next;
		current->next = new;
	}
	return;
}

// check if name exists: returns 1 if exists, returns 0 if doesn't exist
int check_name(void* helper, char* name) {
	Directory* current = trans helper;
	
	int detect_error;
	while (current != NULL) {
		if (strcmp(current->name, name)==0) {
			detect_error = 1;
			break;
		} else {
			detect_error = 0;
		}
		current = current->next;
	}
	if (detect_error == 1) {
		return 1;
	}
	return 0;
}

// initialises necessary size information on files
void assign_sizes(FILE* dir, FILE* file) {
	fseek(dir, 0, SEEK_END);
	dtab_size = ftell(dir);
	entries = ftell(dir)/72;
	fseek(dir, 0, SEEK_SET);
	
	fseek(file, 0, SEEK_END);
	fdata_size = ftell(file);
	fseek(file, 0, SEEK_SET);
	return;
}

// calculates free space in filedata file
void calc_diskspace(void* head) {
	Directory* temp = (Directory*) head;
	int used=0;
	while (temp != NULL) {
		used += temp->length;
		temp = temp->next;
	}
	disk_space = fdata_size - used;
	return;
}

// if a file exists, returns place of file in list. else returns NULL
Directory* check_inplace(void* helper, char* input) {
	Directory* holder = trans helper;
	while (holder != NULL) {
		if (strcmp(holder->name, input)==0) {
			break;
		}
		holder = holder->next;
	}
	return holder;
}

// changes value of specifc item in list
void update_list(void* helper, char* oldname, char* newname, uint32_t newset, uint32_t newlength) {
	return;
}

// writes values to specific name in directory table file
void update_dirtab(char* oldname, char* newname, uint32_t newset, uint32_t newlength) {
	fseek(dtab, 0, SEEK_SET);
	Directory* obj = (Directory*) malloc(sizeof(Directory));
	for (int i=0; i<entries; i++) {
		fread(obj, 72, 1, dtab);
		if (strcmp(obj->name, oldname)==0) {
			strcpy(obj->name, newname);
			obj->offset = newset;
			obj->length = newlength;
			fseek(dtab, -72, SEEK_CUR);
			fwrite(obj, 72, 1, dtab);
			break;
		}
	}
	free(obj);
	fseek(dtab, 0, SEEK_SET);
	fflush(dtab);
	return;
}

void update_filedata() {
	return;
}

// takes out an element in list and holds it
Directory* take_out_element (void* helper, char* filename) {
	// need current and prev for specialised method
	Directory* current = trans helper;
	
	// check if element taking out is head or not
	if (strcmp(current->name, filename)==0) {
		head = head->next;
		return current;
	}
	
	Directory* prev = NULL;
	while(current != NULL) {
		if (strcmp(current->name, filename)==0) {
			break;
		}
		prev = current;
		current = current->next;
	}
	
	// update linked list
	if (prev == NULL) {
		current = NULL;
		return NULL;
	}
	prev->next = current->next;
	current->next = NULL;
	
	return current;
}
/*#############################################
##################### END #####################
#############################################*/

/*#############################################
############ SELF TEST FUNCTIONS ##############
#############################################*/
/* prints dir table */
void print_dir(void* helper) {
	Directory* temp = trans helper;
	if (temp == NULL) {
		printf("Error: head doesn't exist bruh\n");
	}
	
	// global file size for directory table
	printf("directory table file size: %d\n", dtab_size);
	
	// global entry size for directory table (bytes / 72)
	printf("no. of entries in d_tab: %d\n", entries);

	// global file size of file data
	printf("file data file size: %d\n", fdata_size);

	// global available disk space (total bytes - lengths)
	printf("disk space available in file data: %d\n\n", disk_space);
	
	int i=0;
	while(1) {
		printf("Element [%d]\n", i);
		if (temp->next == NULL) {
			printf("name: %s\n", temp->name);
			printf("offset: %d\n", temp->offset);
			printf("length: %d\n\n", temp->length);
			break;
		}
		printf("name: %s\n", temp->name);
		printf("offset: %d\n", temp->offset);
		printf("length: %d\n\n", temp->length);
		temp = temp->next;
		i++;
	}
	return;
}

// create file
void tester_add(void* helper, char* newname, uint32_t newset, uint32_t newlength) {
	Directory* new = (Directory*) malloc(sizeof(Directory));
	strcpy(new->name, newname);
	new->offset = newset;
	new->length = newlength;
	list_add(helper, new);
	return;
}

void tester_remove(void* helper, char* filename) {
	// need current and prev for specialised method
	Directory* current = trans helper;
	Directory* prev = NULL;
	
	while(current != NULL) {
		if (strcmp(current->name, filename)==0) {
			break;
		}
		prev = current;
		current = current->next;
	}
	
	// update linked list
	if (prev == NULL) {
		current = NULL;
		return;
	}
	prev->next = current->next;
	current->next = NULL;
	free(current);
	return;
}

void combine(void* helper, char* filename) {
	if (trans helper == NULL) {
		printf("combine input is null\n");
		return;
	}
	
	Directory* holder = take_out_element(helper, filename);
	
	
	printf("\nELEMENT TAKEN OUT NAME: %s\n", holder->name);
	printf("ELEMENT TAKEN OUT OFFSET: %d\n", holder->offset);
	printf("ELEMENT TAKEN OUT LENGTH: %d\n\n", holder->length);
	
	print_dir(helper);
	
	holder->offset = 370;
	holder->length = 390;
	printf("\nELEMENT PUT IN NAME: %s\n", holder->name);
	printf("ELEMENT PUT IN OFFSET: %d\n", holder->offset);
	printf("ELEMENT PUT IN LENGTH: %d\n\n", holder->length);
	
	list_add(helper, holder);
	
	return;
}
/*#############################################
##################### END #####################
#############################################*/

void * init_fs(char * f1, char * f2, char * f3, int n_processors) {
	// init directory table ptr
	dtab = fopen(f2, "rb+");
	if (dtab == NULL) {
		printf("Error: unable to open file\n");
		exit(1);
	}
	
	// init file data ptr
	fdata = fopen(f1, "rb+");
	if (fdata == NULL) {
		printf("Error: unable to open file\n");
		exit(1);
	}
	
	// init hash data ptr
	hash = fopen(f3, "rb+");
	if (hash == NULL) {
		printf("Error: unable to open file\n");
		exit(1);
	}
	
	// init dir_tab file and entry size
	assign_sizes(dtab, fdata);
	
	// init struct
	int head_check = 0;
	for (int i=0; i<entries; i++) {
		Directory* helper = (Directory*) malloc(sizeof(Directory));
		fread(helper, 72, 1, dtab);
		if (strcmp(helper->name, "")==0) {
			free(helper);
			continue;
		} else {
			head_check++;
			if (head_check == 1) {
				helper->next = NULL;
				head = helper;
			} else {
				helper->next = NULL;
				list_add(&head, helper);
			}
		}
	}
	// init file data size
	calc_diskspace(head);
	return &head;
}

void close_fs(void * helper) {
	if (trans helper == NULL) {
		return;
	}
	Directory* temp;
	while(trans helper != NULL) {
		temp = trans helper;
		trans helper = (trans helper)->next;
		free(temp);
	}
	fclose(fdata);
	fclose(dtab);
	return;
}

int create_file(char * filename, size_t length, void * helper) {
	// check disk space
	int use_len = length;
	int disk_resizable = disk_space + use_len;
	if (length > disk_resizable) {
		return 2;
	}
	// init a new offset for new file
	int newoffset;
	// empty disk
	if (trans helper == NULL) {
		// updating list
		newoffset = 0;
		Directory* new = init_element(filename, newoffset, use_len, NULL);
		list_add(helper, new);
		
		// updating directory table
		update_dirtab("", filename, newoffset, use_len);
		
		// updating file data
		char* null = calloc(1, use_len);
		fseek(fdata, newoffset, SEEK_SET);
		fwrite(null, use_len, 1, fdata);
		fseek(fdata, 0, SEEK_SET);
		fflush(fdata);
		free(null);
		
		// update disk space
		disk_space = disk_space - use_len;
		compute_hash_tree(helper);
		return 0;
	}
	// check if filename exists
	Directory* current = check_inplace(helper, filename);
	if (current != NULL) {
		return 1;
	}
	
	// check if repack is necessary
	int need_re = 0; 
	// check file spaces
	int space_behind;
	current = trans helper;
	Directory* prev = NULL;
	
	while(1) {
		// checking first element
		if (prev == NULL) {
			space_behind = current->offset;
			if (space_behind >= use_len) {
				// updating list
				newoffset = current->offset - space_behind;
				Directory* new = init_element(filename, newoffset, use_len, NULL);
				list_add(helper, new);
				
				// updating file data
				char* null = calloc(1, use_len);
				fseek(fdata, newoffset, SEEK_SET);
				fwrite(null, use_len, 1, fdata);
				fseek(fdata, 0, SEEK_SET);
				fflush(fdata);
				free(null);
				
				// update directory table
				update_dirtab("", filename, newoffset, use_len);
				
				// update disk space
				disk_space = disk_space - use_len;
				need_re = 0;
				break;
			}
			prev = current;
			current = current->next;
			continue;
		}
		// checking last element
		if (current == NULL) {
			space_behind = disk_space - ((prev->offset) + (prev->length));
			if (space_behind >= use_len) {
				//newoffset = current->offset - space_behind;
				newoffset = (prev->offset) + (prev->length);
				Directory* new = init_element(filename, newoffset, use_len, NULL);
				list_add(helper, new);

				// updating file
				char* null = calloc(1, use_len);
				fseek(fdata, newoffset, SEEK_SET);
				fwrite(null, use_len, 1, fdata);
				fseek(fdata, 0, SEEK_SET);
				fflush(fdata);
				free(null);

				// update directory table
				update_dirtab("", filename, newoffset, use_len);
				// update disk space
				disk_space = disk_space - use_len;
				need_re = 0;
			}
			break;
		}
		
		// updating list
		space_behind = (current->offset) - ((prev->offset) + (prev->length));
		if (space_behind >= use_len) {
			newoffset = current->offset - space_behind;
			Directory* new = init_element(filename, newoffset, use_len, NULL);
			list_add(helper, new);
			
			// updating file
			char* null = calloc(1, use_len);
			fseek(fdata, newoffset, SEEK_SET);
			fwrite(null, use_len, 1, fdata);
			fseek(fdata, 0, SEEK_SET);
			fflush(fdata);
			free(null);
			
			// update directory table
			update_dirtab("", filename, newoffset, use_len);
			
			// update disk space
			disk_space = disk_space - use_len;
			need_re = 0;
			break;
		}
		prev = current;
		current = current->next;
		need_re++;
	}
	fseek(fdata, 0, SEEK_SET);
	if (need_re == 0) {
		compute_hash_tree(helper);
		return 0;
	}
	// repack and try again
	repack(helper);
	current = trans helper;
	while(current->next != NULL) {
		current = current->next;
	}
	
	// update list
	newoffset = current->offset + current->length;
	Directory* new = init_element(filename, newoffset, use_len, NULL);
	list_add(helper, new);

	// updating file
	char* null = calloc(1, use_len);
	fseek(fdata, newoffset, SEEK_SET);
	fwrite(null, use_len, 1, fdata);
	fseek(fdata, 0, SEEK_SET);
	fflush(fdata);
	free(null);

	// update directory table
	update_dirtab("", filename, newoffset, use_len);

	// update disk space
	disk_space = disk_space - use_len;
	compute_hash_tree(helper);
	return 0;
}


int resize_file(char * filename, size_t length, void * helper) {
	// check head isn't null
	if (trans helper == NULL) {
		return 1;
	}
	// check if filename exists
	Directory* current = check_inplace(helper, filename);
	if (current == NULL) {
		return 1;
	}
	// do nothing if input length is equal to filename length
	if (length == current->length) {
		return -1;
	}
	// check disk space
	int disk_resizable = disk_space + (current->length);
	if (length > disk_resizable) {
		return 2;
	}
	// check file space
	int file_resizable;
	if ((current->next) == NULL) {
		file_resizable = fdata_size - (current->offset);
	} else {
		file_resizable = ((current->next)->offset) - (current->offset);
	}
	// cast to a signed int for compatibility with disk space
	int old_len;
	int use_len = length;
	// update directory table if enough space in file
	if (length <= file_resizable) {
		// only write to file data if length input is larger than file length
		if (length > current->length) {
			// keep records of resizable file
			char* buff = calloc(1, use_len);
			old_len = current->length;
			read_file(current->name, 0, current->length, buff, helper);
			
			// update file data file
			fseek(fdata, current->offset, SEEK_SET);
			fwrite(buff, use_len, 1, fdata);
			fseek(fdata, 0, SEEK_SET);
			fflush(fdata);
			free(buff);
		}
		
		old_len = current->length;
		use_len = length;
		
		// update list and directory table
		current->length = length;
		update_dirtab(filename, current->name, current->offset, current->length);
		
		// update disk_space
		disk_space = disk_space - (use_len - old_len);
		compute_hash_tree(helper);
		return 0;
	}
	// repack and try size again
	else {
		// keep records of resizable file
		char* buff = calloc(1, use_len);
		old_len = current->length;
		read_file(current->name, 0, current->length, buff, helper);
		
		// take out element in list to hold for now
		Directory* holder = take_out_element(helper, filename);
		
		// repack
		repack(helper);
		
		// init and set new offset for holder
		Directory* counter = trans helper;
		while ((counter->next) != NULL) {
		counter = counter->next;
		}
		int newoffset = counter->offset + counter->length;
		holder->offset = newoffset;
		holder->length = length;
		
		// update list
		list_add(helper, holder);
		
		// update directory table file
		update_dirtab(filename, holder->name, holder->offset, holder->length);		
		
		// update file data file
		fseek(fdata, newoffset, SEEK_SET);
		fwrite(buff, use_len, 1, fdata);
		fseek(fdata, 0, SEEK_SET);
		fflush(fdata);
		free(buff);
		// update disk_space
		disk_space = disk_space - (current->length-old_len);
		compute_hash_tree(helper);
		return 0;
	}
}

void repack(void * helper) {
	// check head isn't NULL
	if (trans helper == NULL) {
		return;
	}
	// do nothing if file only contains head and is already front of list
	if (((trans helper)->next) == NULL && ((trans helper)->offset)==0) {
		return;
	}
	// do nothing if file is already repacked
	Directory* prev = trans helper;
	Directory* current = prev->next;
	while (current != NULL) {
		if ((prev->offset + prev->length) != (current->offset)) {
			break;
		}
		prev = current;
		current = current->next;
	}
	if (current == NULL) {
		return;
	} else {
		current = trans helper;
		prev = NULL;
	}
	// update linked list and filedata in place
	int space_behind;
	while(current != NULL) {
		// repacking first element
		if (prev == NULL) {
			space_behind = current->offset;
			char buff[current->length];
			read_file(current->name, 0, current->length, buff, helper);
			current->offset = current->offset - space_behind;
			fseek(fdata, current->offset, SEEK_SET);
			fwrite(buff, current->length, 1, fdata);
			fflush(fdata);
			
			prev = current;
			current = current->next;
			continue;
		}
		// repacking rest of file
		space_behind = (current->offset) - ((prev->offset) + (prev->length));
		char buff[current->length];
		read_file(current->name, 0, current->length, buff, helper);
		current->offset = current->offset - space_behind;
		fseek(fdata, current->offset, SEEK_SET);
		fwrite(buff, current->length, 1, fdata);
		fflush(fdata);
		
		prev = current;
		current = current->next;
	}	
	fseek(fdata, 0, SEEK_SET);

	// updating directory_table
	current = trans helper;
	while(current != NULL) {
		
		fseek(dtab, 0, SEEK_SET);
		Directory* holder = (Directory*) malloc(sizeof(Directory));

		for (int i=0; i<entries; i++) {
			fread(holder, 72, 1, dtab);
			if (strcmp(current->name, holder->name)==0) {

				// set pointer to offset
				fseek(dtab, -8, SEEK_CUR);
				fwrite(&(current->offset), 4, 1, dtab);
				
				// set pointer to length
				fwrite(&(current->length), 4, 1, dtab);
				break;
			}
		}
		free(holder);
		// reset pointer to start of file and reset current to head
		fseek(dtab, 0, SEEK_SET);
		fflush(dtab);
		current = current->next;
	}
	compute_hash_tree(helper);
	return;
}

int delete_file(char * filename, void * helper) {
	// need current and prev for specialised method
	Directory* current = trans helper;
	Directory* prev = NULL;
	// check if filename exists and init prev
	while(current != NULL) {
		if (strcmp(current->name, filename)==0) {
			break;
		}
		prev = current;
		current = current->next;
	}
	if (current == NULL) {
		return 1;
	}
	// update directory table file
	update_dirtab(filename, "", 0, 0);
	// update linked list
	if (prev == NULL) {
		current = NULL;
	} else {
		prev->next = current->next;
		
		// update diskspace
		disk_space = disk_space + current->length;
		
		free(current);
		current = NULL;
	}
	return 0;
}

int rename_file(char * oldname, char * newname, void * helper) {
	// check head isn't NULL
	if (trans helper == NULL) {
		return 1;
	}
	// check newname not over byte limit
	if (strlen(newname) > 63) {
		return 1;
	}
	// test to see if new name doesn't already exist;
	int detect_error = check_name(helper, newname);
	if (detect_error == 1) {
		return 1;
	}
	// check if oldname exists
	Directory* holder = check_inplace(helper, oldname);
	if (holder == NULL) {
		return 1;
	}
	// rename oldname to newname in list
	strcpy(holder->name, newname);
	update_dirtab(oldname, newname, holder->offset, holder->length);
	return 0;
}

int read_file(char * filename, size_t offset, size_t count, void * buf, void * helper) {
	// check head isn't NULL
	if (trans helper == NULL) {
		return 1;
	}
	// test to see if file exists
	Directory* holder = check_inplace(helper, filename);
	if (holder == NULL) {
		return 1;
	}
	// calculate size of file, where to start reading and when to stop
	int end = (holder->offset) + (holder->length);
	int s_read = (holder->offset) + offset;
	int f_read = s_read + count;
	// return if asking to read more than file size
	if (f_read > end) {
		return 2;
	}
	// load into buffer when successful
	fseek(fdata, s_read, SEEK_SET);
	fread(buf, count, 1, fdata);
	fseek(fdata, 0, SEEK_SET);
	return 0;
}

int write_file(char * filename, size_t offset, size_t count, void * buf, void * helper) {
	// check head isn't NULL
	if (trans helper == NULL) {
		return 1;
	}
	// test to see if file exists
	Directory* current = check_inplace(helper, filename);
	if (current == NULL) {
		return 1;
	}
	// return if offset greater than file size
	if (offset > current->length) {
		return 2;
	}
	// calculate size of file, where to start writing and when to stop
	int end = (current->offset) + (current->length);
	int s_write = (current->offset) + offset;
	int f_write = s_write + count;
	
	// if we have enough file space then write
	if (f_write < end) {
		fseek(fdata, s_write, SEEK_SET);
		fwrite(buf, count, 1, fdata);
		fseek(fdata, 0, SEEK_SET);
		fflush(fdata);
		compute_hash_tree(helper);
		return 0;
	}
	// if extension of write extends past disk space return
	int extension = (offset + count) - (current->length);
	int use_len = current->length + extension;
	if (extension > disk_space) {
		return 2;
	}
	// repack and try
	resize_file(filename, use_len, helper);
	
	// update file data file
	Directory* holder = trans helper;
	while (holder->next != NULL) {
		holder = holder->next;
	}
	fseek(fdata, (holder->offset + offset), SEEK_SET);
	fwrite(buf, count, 1, fdata);
	fseek(fdata, 0, SEEK_SET);
	fflush(fdata);
	
	compute_hash_tree(helper);
	return 0;
}

ssize_t file_size(char * filename, void * helper) {
	// check head isn't NULL
	if (trans helper == NULL) {
		return -1;
	}
	// check if filename exists
	Directory* holder = check_inplace(helper, filename);
	if (holder == NULL) {
		return -1;
	}
	// return file size
	ssize_t size = holder->length;
	return size;
}

void fletcher(uint8_t * buf, size_t length, uint8_t * output) {
	// check if input null
	if (buf == NULL) {
		return;
	}
	// checking if input needs padding
	size_t use_len = length;
	if ((length%4) != 0) {
		use_len = use_len + (4 - (length%4));
	}
	// copy contents into a holder memory address
	uint8_t holder[use_len];
	memcpy(holder, buf, length);
	if (use_len != length) {
		for (int i=length; i<use_len; i++) {
			holder[i] = 0;
		}
	}
	// init fletcher
	uint64_t a = 0;
	uint64_t b = 0;
	uint64_t c = 0;
	uint64_t d = 0;
	uint64_t p = (uint64_t) F_POW;
	// cast holder to 32 byte int
	uint32_t* input = (uint32_t*) holder;
	// compute fletcher
	for (int i=0; i<use_len/4; i++) {
		a = (a + input[i]) % p;
		b = (a + b) % p;
		c = (b + c) % p;
		d = (c + d) % p;
	}
	// copy to output
	memcpy(output, &a, 4);
	memcpy(output+4, &b, 4);
	memcpy(output+8, &c, 4);
	memcpy(output+12, &d, 4);
	return;
}

void compute_hash_tree(void * helper) {
	// check head isn't NULL
	if (trans helper == NULL) {
		return;
	}
	// calculate blocks
	int block_no = fdata_size / BLOCK_SIZE;
	
	// calculate amount of levels -- denoted by n
	int n = log(block_no)/log(2);
	
	// calculate size needed for hash data array
	int HASH_DATA_SIZE = 16*(pow(2, n+1)-1);
	
	// init array uint8_t for hash data
	uint8_t hash_data [HASH_DATA_SIZE];
	
	// block holder, input holder and output holder for fletcher
	uint8_t block_holder[BLOCK_SIZE];
	uint8_t input[32];
	uint8_t output[16];
	
	// inputing into hash_data
	fseek(fdata, 0, SEEK_SET);
	for (int k=n; k>=0; k--) {
		for (int i=START_INDEX(k); i<=END_INDEX(k); i++) {
			// leaf
			if (k==n) {
				// read whole block
				fread(block_holder, BLOCK_SIZE, 1, fdata);
				// compute fletcher for block
				fletcher(block_holder, BLOCK_SIZE, output);
				// copy results into hash_data
				memcpy(hash_data+(i*16), &output, 16);
			} 
			// parent
			else {
				// copy left and right child, concat into input
				int l = (i*2 + 1)*16;
				int r = (i*2 + 2)*16;
				memcpy(input, hash_data+l, 16);
				memcpy(input+16, hash_data+r, 16);
				// compute fletcher
				fletcher(input, 32, output);
				// copy results into hash_data
				memcpy(hash_data+(i*16), &output, 16);
			}
		}
	}
	// write to hash_data file
	fseek(hash, 0, SEEK_SET);
	fwrite(hash_data, 1, HASH_DATA_SIZE, hash);
	fseek(hash, 0, SEEK_SET);
	fflush(hash);
	return;
}
void compute_hash_block(size_t block_offset, void * helper) {
	compute_hash_tree(helper);
	return;
}

