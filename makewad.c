#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <malloc.h>

typedef uint32_t dword;
typedef uint8_t byte;

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
    for (int i=0; s && s[i] && i<n; i++) {
        if (s[i]>='a' && s[i]<='z')
            s[i] -= ('a'-'A');
    }
}

static int dumpwad(const char *wadname, int isExt) {
    fprintf(stderr, "Dumping XWAD: %s\n", wadname);
    FILE *wad = fopen(wadname, "rb");
    if (!wad) {
        perror("opening WAD");
        return 1;
    }
    wad_header_t hdr;
    if (fread(&hdr, sizeof(hdr), 1, wad)!=1) {
        perror("reading header");
        return 2;
    }
    fprintf(stderr, "ident=%.4s, count=%lu, offset=%lu\n", hdr.ident, hdr.count, hdr.offset);
    if (fseek(wad, hdr.offset, SEEK_SET)<0) {
        perror("seeking to index");
        return 3;
    }
    wad_lump_t *idx = calloc(hdr.count, sizeof(wad_lump_t));
    if (fread(idx, sizeof(wad_lump_t), hdr.count, wad)!=hdr.count) {
        perror("reading index");
        return 4;
    }
    for (int i=0; i<hdr.count; i++) {
        fprintf(stderr, "..%d: %.32s offset=%lu, size=%lu\n", i, idx[i].name, idx[i].offset, idx[i].size);
        if (isExt) {
            if (fseek(wad, idx[i].offset, SEEK_SET)<0) {
                perror("seeking content");
                return 5;
            }
            char fout[32];
            for (int c=0; c<32; c++) {
                fout[c] = idx[i].name[c];
                if ('/'==fout[c]) fout[c] = '-';
            }
            FILE *out = fopen(fout, "wb");
            if (!out) {
                perror("opening content for write");
                return 6;
            }
            byte buf[8192];
            size_t n = idx[i].size;
            size_t t = 0;
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

int main(int argc, char **argv) {
    char *wadname = "GAME.WAD";
    int arg, isDump = 0, isExt = 0;
    wad_header_t hdr;
    wad_lump_t *idx;
    FILE *wad;
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
    fprintf(stderr, "Constructing XWAD: %s\n", wadname);
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
    for (int i=0; arg<argc; arg++, i++) {
        fprintf(stderr, "..%d: %s\n", i, argv[arg]);
        FILE *in = fopen(argv[arg], "rb");
        if (!in) {
            perror("opening file for read");
            return 4;
        }
        strncpy(idx[i].name, argv[arg], 32);
        upper(idx[i].name, 32);
        idx[i].offset = hdr.offset;
        idx[i].size = 0;
        byte buf[8192];
        size_t n;
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