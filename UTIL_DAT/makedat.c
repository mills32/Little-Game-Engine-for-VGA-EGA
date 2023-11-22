#include <stdio.h>
#include <string.h>
#include <malloc.h>

int main(int argc, char **argv) {
	unsigned short filesize = 0x0000;
	unsigned long dat_offset = 32 + 2048;
    unsigned char i, arg;
	unsigned char ssize = 0;
	unsigned char extract = 0;
	unsigned char *extract_file;
	unsigned char number = argc-1;
    FILE *dat;
	FILE *file;
	unsigned char *buffer = (unsigned char*) calloc(1,65535);
	
	if (!argv[1]) {printf("\nusage\n- Create dat: makedat file1 file2 ...\n- Extract dat: makedat -e FILE.DAT"); return 0;}
	if (argc > 64) {printf("\nOnly 32 files are supported\n"); return 0;}
	extract_file = (unsigned char*)argv[1];
	if ((extract_file[0] == '-')&&(extract_file[1] == 'e')) {
		printf("\nextract\n"); 
		extract = 1;
	}
	printf("\n");
	if (!extract){
		//CREATE FILE
		dat = fopen("DATA.DAT","wb");
		
		fprintf(dat,"LDAT  LT-ENGINE FILES:          ");
		fseek(dat, 22, SEEK_SET); fwrite(&number,1,1,dat);
		for (arg=1; arg<argc; arg++) {
			int i = 0;
			int namepos = 0;
			unsigned char *name = (unsigned char*)argv[arg];
			//if dropped file (windows) remove path
			for (i = 0; i < strlen((const char*)name);i++){
				if (name[i] == 0x5C) namepos = i+1;
			}
			memmove(&name[0],&name[namepos],16);
			//Print file name
			fseek(dat, arg*32, SEEK_SET);
			fprintf(dat,"%s",name);
			if ((strlen((const char*)name))>12){printf("\nCan't use names bigger than 8 characters %s\n",name); return 0;}
			//Read file to insert
			file = fopen((const char*)name,"rb");
			if (!file) {printf("\nCan't find %s\n",name); return 0;}
			fseek(file, 0, SEEK_END);
			filesize = ftell(file);
			rewind(file);
			memset(buffer,0,16);
			fread(buffer,filesize,1,file);
			fclose(file);
			
			//Write file offset in data, and file size
			fseek(dat, (arg*32)+16, SEEK_SET);
			fwrite(&dat_offset,sizeof(dat_offset),1,dat);
			fwrite(&filesize,sizeof(filesize),1,dat);
			
			//Write actual file
			fseek(dat,dat_offset, SEEK_SET);
			fwrite(buffer,filesize,1,dat);
			
			dat_offset += filesize;
		}
		fclose(dat);
	} else {
		int i = 0;
		unsigned char *name = (unsigned char*)argv[2];
		if (!argv[2]) {printf("\nSpcify a DAT file: -e FILE.DAT\n"); return 0;}
		dat = fopen((const char*)name,"rb");
		fseek(dat, 22, SEEK_SET); 
		fread(&number,1,1,dat);
		for (i=1; i<number+1; i++) {
			fseek(dat, 32*i, SEEK_SET);
			memset(buffer,0,16);
			//Read file name
			fread(buffer,1,16,dat);
			printf("Extracting: %s\n",buffer);
			//Read data offset
			fread(&dat_offset,4,1,dat);
			//read file size
			fread(&filesize,2,1,dat);
			//Go to data offset
			fseek(dat,dat_offset, SEEK_SET);
			//Open file
			file = fopen((const char*)buffer,"wb");
			//write file
			fread(buffer,filesize,1,dat);
			fwrite(buffer,filesize,1,file);
			fclose(file);
		}
		fclose(dat);
	}
	
	free(buffer);
	buffer = NULL;
    return 0;
}