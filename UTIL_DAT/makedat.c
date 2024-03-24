#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <dirent.h>
#include <sys/stat.h>

struct dirent *direntry;
struct stat s;
unsigned char nfiles = 0;
DIR *folder;

int check_ext(unsigned char *name){
	int type = 0;
	int len = strlen((const char*)name);
	unsigned long *ext = (unsigned long*) &name[len-4];
	switch (ext[0]){
		//file extensions
		case 0x504D422E:
		case 0x706D622E: printf("A BMP\n"); type = 1; break;
		case 0x584D542E:
		case 0x786D742E: printf("A TMX\n"); type = 2; break;
		case 0x4D47562E:
		case 0x6D67762E: printf("A VGM\n"); type = 3; break;
	}
	return type;
}


int main(int argc, char **argv) {
	unsigned short filesize = 0x0000;
	unsigned long dat_offset = 32 + 2048;
	unsigned long file_pos = 0;
    unsigned char i, arg;
	unsigned char ssize = 0;
	unsigned char extract = 0;
	unsigned char *extract_file;
	unsigned char number = argc-1;
    FILE *dat;
	FILE *file;
	unsigned char *buffer = (unsigned char*) calloc(1,65535);
	unsigned char is_folder = 0;
	
	if (!argv[1]) {printf("\nusage\n- Create dat: makedat file1 file2 ...\n- Extract dat: makedat -e FILE.DAT"); return 0;}
	if (argc > 64) {printf("\nOnly 64 files are supported\n"); return 0;}
	extract_file = (unsigned char*)argv[1];
	if ((extract_file[0] == '-')&&(extract_file[1] == 'e')) {
		printf("\nextract\n"); 
		extract = 1;
	}
	printf("\n");
	if (!extract){
		//Is it a folder?
		stat(argv[1],&s);
		if (s.st_mode & S_IFDIR) is_folder = 1;
		
		//CREATE FILE
		dat = fopen("DATA.DAT","wb");
		
		fprintf(dat,"LDAT  LT-ENGINE FILES:          ");
		fseek(dat, 22, SEEK_SET); fwrite(&number,1,1,dat);

		if (!is_folder){
			for (arg=1; arg<argc; arg++) {
				int i = 0;
				int namepos = 0;
				int type = 0;
				int index = 0;
				char line[128];
				unsigned char start_map_data = 0;
				unsigned char *name = (unsigned char*)argv[arg];
				//if dropped file (windows) remove path
				for (i = 0; i < strlen((const char*)name);i++){if (name[i] == 0x5C) namepos = i+1;}
				memmove(&name[0],&name[namepos],16);
				//Print file name
				fseek(dat, arg*32, SEEK_SET);
				fprintf(dat,"%s",name);
				if ((strlen((const char*)name))>12){printf("\nCan't use names bigger than 8 characters %s\n",name); return 0;}
				//Check file type
				type = check_ext(name);
				//if (!type){printf("file format not recognized %s\n",name);return 0;}
				//Read file to insert
				file = fopen((const char*)name,"rb");
				if (!file) {printf("\nCan't find %s\n",name); return 0;}
				switch (type){
					case 0:
					case 1:
					case 3://BMP VGM or unknown
						fseek(file, 0, SEEK_END);
						filesize = ftell(file);
						rewind(file);
						fread(buffer,filesize,1,file);
					break;
					case 2://store tmx maps more efficiently
						while(!start_map_data){	//read lines 
							memset(line, 0, 64);
							fgets(line, 64, file);
							if((line[2] == '<')&&(line[3] == 'd')) start_map_data = 1;//<data encoding="csv">
						}
						start_map_data = 0;
						filesize = ftell(file);
						//Read header
						fseek(file, 0, SEEK_SET);
						fread(buffer,filesize,1,file);
						//read map in bytes
						for (index = 0; index < 256*19; index++){
							unsigned char tile = 0;
							fscanf(file, "%d,",&tile);if (!tile) tile++;
							buffer[filesize++] = tile; 
						}
						//advance to bkg map
						while(!start_map_data){	//read lines 
							memset(line, 0, 64);
							fgets(line, 64, file);
							if((line[2] == '<')&&(line[3] == 'd')) start_map_data = 1;//<data encoding="csv">
						}
						//paste lines
						sprintf((char*)&buffer[filesize],"\n</data>\n </layer>\n <layer id=\"2\" name=\"col\" width=\"256\" height=\"19\">\n  <data encoding=\"csv\">\n");
						filesize+= 94;
						//read bkg map in bytes
						for (index = 0; index < 256*19; index++){
							unsigned char tile = 0;
							fscanf(file, "%d,",&tile);if (!tile) tile++;
							buffer[filesize++] = tile; 
						}
						//paste end
						sprintf((char*)&buffer[filesize],"\n</data>\n </layer>\n</map>\n");
						filesize+= 26;
					break;
				}
				//Write file offset in data, and file size
				file_pos = dat_offset;
				fseek(dat, (arg*32)+16, SEEK_SET);
				fwrite(&file_pos,4,1,dat);
				fwrite(&filesize,2,1,dat);
				
				//Write actual file
				fseek(dat,file_pos, SEEK_SET);
				fwrite(buffer,filesize,1,dat);
				
				dat_offset += filesize;
				
				fclose(file);
			}
		} else {
			//READ FOLDER
			chdir(argv[1]);
			folder = opendir(".");
			nfiles = 1;
			while((direntry = readdir(folder)) != NULL){
				stat(direntry->d_name,&s);
				if (!(s.st_mode & S_IFDIR)) {
					int i = 0;
					int namepos = 0;
					int type = 0;
					int index = 0;
					char line[128];
					unsigned char start_map_data = 0;
					unsigned char *name = (unsigned char*)direntry->d_name;
					//if dropped file (windows) remove path
					for (i = 0; i < strlen((const char*)name);i++){
						if (name[i] == 0x5C) namepos = i+1;
					}
					memmove(&name[0],&name[namepos],16);
					//Print file name
					fseek(dat, nfiles*32, SEEK_SET);
					fprintf(dat,"%s",name);
					if ((strlen((const char*)name))>12){printf("\nCan't use names bigger than 8 characters %s\n",name); return 0;}
					//Check file type
					type = check_ext(name);
					//if (!type){printf("file format not recognized %s\n",name);return 0;}
					//Read file to insert
					file = fopen((const char*)name,"rb");
					if (!file) {printf("\nCan't find %s\n",name); return 0;}
					switch (type){
						case 0:
						case 1:
						case 3://BMP VGM or unknown
							fseek(file, 0, SEEK_END);
							filesize = ftell(file);
							rewind(file);
							fread(buffer,filesize,1,file);
						break;
						case 2://store tmx maps more efficiently
							while(!start_map_data){	//read lines 
								memset(line, 0, 64);
								fgets(line, 64, file);
								if((line[2] == '<')&&(line[3] == 'd')) start_map_data = 1;//<data encoding="csv">
							}
							start_map_data = 0;
							filesize = ftell(file);
							//Read header
							fseek(file, 0, SEEK_SET);
							fread(buffer,filesize,1,file);
							//read map in bytes
							for (index = 0; index < 256*19; index++){
								unsigned char tile = 0;
								fscanf(file, "%d,",&tile);if (!tile) tile++;
								buffer[filesize++] = tile; 
							}
							//advance to bkg map
							while(!start_map_data){	//read lines 
								memset(line, 0, 64);
								fgets(line, 64, file);
								if((line[2] == '<')&&(line[3] == 'd')) start_map_data = 1;//<data encoding="csv">
							}
							//paste lines
							sprintf((char*)&buffer[filesize],"\n</data>\n </layer>\n <layer id=\"2\" name=\"col\" width=\"256\" height=\"19\">\n  <data encoding=\"csv\">\n");
							filesize+= 94;
							//read bkg map in bytes
							for (index = 0; index < 256*19; index++){
								unsigned char tile = 0;
								fscanf(file, "%d,",&tile);if (!tile) tile++;
								buffer[filesize++] = tile; 
							}
							//paste end
							sprintf((char*)&buffer[filesize],"\n</data>\n </layer>\n</map>\n");
							filesize+= 26;
						break;
					}
					
					file_pos = dat_offset;
					//Write file offset in data, and file size
					fseek(dat, (nfiles*32)+16, SEEK_SET);
					fwrite(&file_pos,sizeof(dat_offset),1,dat);
					fwrite(&filesize,sizeof(filesize),1,dat);
					
					//Write actual file
					fseek(dat,file_pos, SEEK_SET);
					fwrite(buffer,filesize,1,dat);
					
					dat_offset += filesize;
					nfiles++;
				}
			}
		}
		fclose(dat);
	} else {
		//EXTRACT FILES
		int i = 0;
		unsigned char *name = (unsigned char*)argv[2];
		if (!argv[2]) {printf("\nSpcify a DAT file: -e FILE.DAT\n"); return 0;}
		dat = fopen((const char*)name,"rb");
		fseek(dat, 22, SEEK_SET); 
		fread(&number,1,1,dat);
		for (i=1; i<number+1; i++) {
			int type = 0;
			fseek(dat, 32*i, SEEK_SET);
			memset(buffer,0,16);
			//Read file name
			fread(buffer,1,16,dat);
			printf("Extracting: %s\n",buffer);
			//Read data offset
			fread(&dat_offset,4,1,dat);
			//Check file type
			type = check_ext(buffer);
			if (type != 2){
				//read file size
				fread(&filesize,2,1,dat);
				//Go to data offset
				fseek(dat,dat_offset, SEEK_SET);
				//Open file
				file = fopen((const char*)buffer,"wb");
				//write file
				fread(buffer,filesize,1,dat);
				fwrite(buffer,filesize,1,file);
			} else {//Regenerate tmx csv format
				char line[128];
				int lines = 0;
				int tile = 0;
				unsigned char start_map_data = 0;
				//Go to data offset
				fseek(dat,dat_offset, SEEK_SET);
				//Open file
				file = fopen((const char*)buffer,"wb");
				while(!start_map_data){
					memset(line, 0, 64);
					fgets(line, 64, dat);
					fprintf(file,"%s",line);
					if((line[2] == '<')&&(line[3] == 'd')) start_map_data = 1;//<data encoding="csv">
					line[63] = 0;
				}
				start_map_data = 0;
				for(lines = 0; lines < 19;lines++){
					for (tile = 0; tile < 256; tile++){
						unsigned char c = fgetc(dat);
						if ((lines == 18) && (tile == 255)) fprintf(file,"%i",c);
						else fprintf(file,"%d,",c);
					}
					if (lines != 18)fprintf(file,"\n");
				}
				while(!start_map_data){
					memset(line, 0, 64);
					fgets(line, 64, dat);
					fprintf(file,"%s",line);
					if((line[2] == '<')&&(line[3] == 'd')) start_map_data = 1;//<data encoding="csv">
				}
				for(lines = 0; lines < 19;lines++){
					for (tile = 0; tile < 256; tile++){
						unsigned char c = fgetc(dat);
						if (lines == 18 && tile == 255) fprintf(file,"%i",c);
						else fprintf(file,"%i,",c);
					}
					if (lines != 18)fprintf(file,"\n");
				}
				start_map_data = 0;
				while(!start_map_data){
					memset(line, 0, 64);
					fgets(line, 64, dat);
					fprintf(file,"%s",line);
					if((line[0] == '<')&&(line[2] == 'm')) start_map_data = 1;//</map>
				}
			}
			fclose(file);
		}
		fclose(dat);
	}
	
	free(buffer);
	buffer = NULL;
    return 0;
}