#include <stdio.h>
#include <string.h>
#include <malloc.h>

// TurboC++ doesn't have stdint.h
#ifdef __GNUC__
#include <stdint.h>
typedef uint32_t dword;
typedef uint8_t byte;
#else
typedef unsigned long dword;
typedef unsigned char byte;
// we are also going to be expanding wildcards ourselves..
#include <dirent.h>
#define USE_EXPAND
#endif

typedef struct {
	char ident[4];
	dword count;
	dword offset;
} wad_header_t;
typedef struct {
	dword offset;
	dword size;
	char name[32];
} wad_lump_t;

static void upper(char *s, int n) {
    int i;
    for (i=0; s && s[i] && i<n; i++) {
        if (s[i]>='a' && s[i]<='z')
            s[i] -= ('a'-'A');
    }
}

static int dumpwad(const char *wadname, int isExt) {
    FILE *wad;
    wad_header_t hdr;
    wad_lump_t *idx;
    int i;
    fprintf(stderr, "Dumping XWAD: %s\n", wadname);
    wad = fopen(wadname, "rb");
    if (!wad) {
        perror("opening WAD");
        return 1;
    }
    if (fread(&hdr, sizeof(hdr), 1, wad)!=1) {
        perror("reading header");
        return 2;
    }
    fprintf(stderr, "ident=%.4s, count=%lu, offset=%lu\n", hdr.ident, hdr.count, hdr.offset);
    if (fseek(wad, hdr.offset, SEEK_SET)<0) {
        perror("seeking to index");
        return 3;
    }
    idx = calloc(hdr.count, sizeof(wad_lump_t));
    if (fread(idx, sizeof(wad_lump_t), hdr.count, wad)!=hdr.count) {
        perror("reading index");
        return 4;
    }
    for (i=0; i<hdr.count; i++) {
        fprintf(stderr, "..%d: %.32s offset=%lu, size=%lu\n", i, idx[i].name, idx[i].offset, idx[i].size);
        if (isExt) {
            char fout[32];
            int c;
            FILE *out;
            static byte buf[8192];
            size_t n, t;
            if (fseek(wad, idx[i].offset, SEEK_SET)<0) {
                perror("seeking content");
                return 5;
            }
            for (c=0; c<32; c++) {
                fout[c] = idx[i].name[c];
                if ('/'==fout[c]) fout[c] = '-';
            }
            out = fopen(fout, "wb");
            if (!out) {
                perror("opening content for write");
                return 6;
            }
            n = idx[i].size;
            t = 0;
            if (n>sizeof(buf)) n = sizeof(buf);
            while (t<idx[i].size && (n=fread(buf, 1, n, wad))>0) {
                if (fwrite(buf, 1, n, out)!=n) {
                    perror("writing content file");
                    return 8;
                }
                t += n;
                n = idx[i].size - t;
                if (n>sizeof(buf)) n=sizeof(buf);
            }
            if ((long)n<0) {
                perror("reading WAD content");
                return 7;
            }
            fclose(out);
        }
    }
    fclose(wad);
    return 0;
}

#ifdef USE_EXPAND
static void expand(int *pargc, char ***pargv) {
    int arg, argc = *pargc;
    char **argv = *pargv;
    int nargc = 0;
    char **nargv = NULL;
    // iterate supplied args, building a new list, expanding '*' via file system entries
    for (arg=0; arg<argc; arg++) {
        char *pwild = strchr(argv[arg], '*');
        if (pwild) {
            DIR *dir;
            struct dirent *ent;
            // wildcard found..
            *pwild = 0;
            dir = opendir(argv[arg]);
            while ((ent=readdir(dir))!=NULL) {
                char *path;
                // knowing that _d_reserved holds the DOS DTA, along with
                // http://www.ctyme.com/intr/rb-2977.htm &
                // http://www.ctyme.com/intr/rb-2803.htm,
                // we can skip directories - phew!
                if (dir->_d_reserved[0x15] & 0x10)
                    continue;
                path = malloc(strlen(argv[arg])+strlen(ent->d_name)+1);
                strcpy(path, argv[arg]);
                strcat(path, ent->d_name);
                if (!nargv)
                    nargv = calloc(1, sizeof(char *));
                else
                    nargv = realloc(nargv, (nargc+1)*sizeof(char *));
                nargv[nargc++] = path;
            }
            closedir(dir);
        } else {
            // plain argument
            if (!nargv)
                nargv = calloc(1, sizeof(char *));
            else
                nargv = realloc(nargv, (nargc+1)*sizeof(char *));
            nargv[nargc++] = argv[arg];
        }
    }
    if (nargv) {
        *pargc = nargc;
        *pargv = nargv;
    }
}
#endif

int main(int argc, char **argv) {
    char *wadname = "GAME.WAD";
    int i, arg, isDump = 0, isExt = 0;
    wad_header_t hdr;
    wad_lump_t *idx;
    FILE *wad;
#ifdef USE_EXPAND
    // wildcard expansion, 'cause DOS doesn't :=(
    expand(&argc, &argv);
#endif
    for (arg=1; arg<argc; arg++) {
        if (strncmp(argv[arg], "-w", 2)==0) {
            wadname = argv[++arg];
        } else if (strncmp(argv[arg], "-d", 2)==0) {
            wadname = argv[++arg];
            isDump = 1;
        } else if (strncmp(argv[arg], "-x", 2)==0) {
            isExt = 1;
        } else if (strncmp(argv[arg], "-h", 2)==0) {
            fprintf(stderr, "usage: %s [-h][-w <wad>] <file> ... || %s -d <wad> [-x]\n", argv[0], argv[0]);
            return 0;
        } else {
            break;
        }
    }
    if (isDump)
        return dumpwad(wadname, isExt);
    fprintf(stdout, "Constructing XWAD: %s\n", wadname);
    wad = fopen(wadname, "wb");
    if (!wad) {
        perror("opening WAD for write");
        return 1;
    }
    // XWAD - eXtended WAD, has 32 byte content names for 'folder-like' naming, eg: GFX/0_font.bmp
    hdr.ident[0] = 'X';
    hdr.ident[1] = 'W';
    hdr.ident[2] = 'A';
    hdr.ident[3] = 'D';
    hdr.count = argc-arg;
    hdr.offset = sizeof(hdr);
    if (fwrite(&hdr, sizeof(hdr), 1, wad)!=1) {
        perror("writing header");
        return 2;
    }
    idx = calloc(hdr.count, sizeof(wad_lump_t));
    if (!idx) {
        perror("allocating index memory");
        return 3;
    }
    for (i=0; arg<argc; arg++, i++) {
        FILE *in;
        static byte buf[8192];
        size_t n;
        fprintf(stdout, "..%d: %s\n", i, argv[arg]);
        in = fopen(argv[arg], "rb");
        if (!in) {
            perror("opening file for read");
            return 4;
        }
        strncpy(idx[i].name, argv[arg], 32);
        upper(idx[i].name, 32);
        idx[i].offset = hdr.offset;
        idx[i].size = 0;
        while ((long)(n=fread(buf, 1, sizeof(buf), in))>0) {
            if (fwrite(buf, 1, n, wad)!=n) {
                perror("writing file");
                return 6;
            }
            hdr.offset += n;
            idx[i].size += n;
        }
        if ((long)n<0) {
            perror("reading file");
            return 5;
        }
        fclose(in);
    }
    if (fwrite(idx, sizeof(wad_lump_t), hdr.count, wad)!=hdr.count) {
        perror("writing index");
        return 7;
    }
    if (fseek(wad, 0, SEEK_SET)<0) {
        perror("seeking WAD");
        return 8;
    }
    if (fwrite(&hdr, sizeof(wad_header_t), 1, wad)!=1) {
        perror("re-writing WAD header");
        return 9;
    }
    fclose(wad);
    return 0;
}