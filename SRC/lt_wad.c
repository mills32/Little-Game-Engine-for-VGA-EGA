#include "lt__eng.h"
#include <stdarg.h>

// XWAD content access
typedef struct {
	char ident[4]; // XWAD
	dword count;
	dword offset;
} wad_header_t;
typedef struct {
	dword offset;
	dword size;
	char name[32];
} wad_lump_t;
typedef struct {
	FILE *fp;
	wad_header_t hdr;
	wad_lump_t *idx;
} wad_file_t;
// structure passed back/forth to caller as FILE object, caches lump info and holds virtual position
typedef struct {
	dword lump;
	dword offset;
	dword size;
	dword pos;
	FILE *mirror;
} wad_FILE;
static wad_file_t gamewad;
static FILE *wadlog;
static void upper(char *s, int n) {
	int i;
    for (i=0; s && s[i] && i<n; i++) {
        if (s[i]>='a' && s[i]<='z')
            s[i] -= ('a'-'A');
    }
}
static FILE *wad_fopen(const char *file, const char *mode) {
	dword n;
	char ufile[32];
	strncpy(ufile, file, 32);
	upper(ufile, 32);
	fprintf(wadlog,"wad_fopen(%s,%s)=", ufile, mode);
	// Lazy load the WAD index
	if (!gamewad.fp) {
		gamewad.fp = fopen("GAME.WAD", "rb");
		if (!gamewad.fp) {
			fprintf(wadlog, "[unopenable GAME.WAD]");
			goto errout;
		}
		if (fread(&gamewad.hdr, sizeof(gamewad.hdr), 1, gamewad.fp)!=1) {
			fclose(gamewad.fp);
			gamewad.fp = NULL;
			fprintf(wadlog, "[unreadable WAD header]");
			goto errout;
		}
		// sanity check
		if (strncmp(gamewad.hdr.ident, "XWAD", 4)!=0) {
			fclose(gamewad.fp);
			gamewad.fp = NULL;
			fprintf(wadlog, "[not an XWAD]");
			goto errout;
		}
		fprintf(wadlog, "[WAD:offset=%lu,count=%lu]", gamewad.hdr.offset, gamewad.hdr.count);
		if (fseek(gamewad.fp, gamewad.hdr.offset, SEEK_SET)<0) {
			fclose(gamewad.fp);
			gamewad.fp = NULL;
			fprintf(wadlog, "[index seek failed]");
			goto errout;
		}
		gamewad.idx = calloc(gamewad.hdr.count, sizeof(wad_lump_t));
		if (!gamewad.idx) {
			fclose(gamewad.fp);
			gamewad.fp = NULL;
			fprintf(wadlog, "[out of memory for index (%lu)]", gamewad.hdr.count);
			goto errout;
		}
		if (fread(gamewad.idx, sizeof(wad_lump_t), gamewad.hdr.count, gamewad.fp) != gamewad.hdr.count) {
			fclose(gamewad.fp);
			gamewad.fp = NULL;
			free(gamewad.idx);
			fprintf(wadlog, "[unreadable index (%lu)]", gamewad.hdr.count);
			goto errout;
		}
	}
	// search for the content name..
	for (n=0; n<gamewad.hdr.count; n++) {
		if (strcmp(ufile, gamewad.idx[n].name)==0) {
			wad_FILE *fp;
			// found it, seek to start & allocate handle type
			if (fseek(gamewad.fp, gamewad.idx[n].offset, SEEK_SET)<0) {
				fprintf(wadlog, "[unseekable lump (%lu)]", gamewad.idx[n].offset);
				goto errout;
			}
			fprintf(wadlog, "[LUMP(%lu):offset=%lu,size=%lu]", n, gamewad.idx[n].offset, gamewad.idx[n].size);
			fp = calloc(1, sizeof(wad_FILE));
			if (!fp) {
				fprintf(wadlog, "out of memory for handle]");
				goto errout;
			}
			fp->lump = n;
			fp->offset = gamewad.idx[n].offset;
			fp->size = gamewad.idx[n].size;
			fp->pos = 0;
#ifdef XWAD_TEST
			// Open the original file as a mirror of WAD content, to enable side-by-side checking
			fp->mirror = fopen(file, mode);
			if (!fp->mirror) {
				fprintf(wadlog, "[NO MIRROR]");
			}
#endif
			fprintf(wadlog,"=%p\n", fp);
			return (FILE *)fp;
		}
	}
	fprintf(wadlog, "[no such lump (%s)]", file);
errout:
	fprintf(wadlog, "=NULL\n");
	return NULL;
}
static int wad_fclose(FILE *stream) {
	wad_FILE *fp = (wad_FILE*)stream;
	fprintf(wadlog, "wad_fclose(%p)\n", fp);
	if (fp->mirror) fclose(fp->mirror);
	free(stream);
	return 0;
}
static int wad_fseek(FILE *stream, long offset, int whence) {
	wad_FILE *fp = (wad_FILE*)stream;
	// check validity of seek
	long pos = (long)fp->pos;
	fprintf(wadlog, "wad_fseek(%p,%li,%i)[%li=>", fp, offset, whence, pos);
	switch (whence) {
	case SEEK_SET:
		pos = offset;
		break;
	case SEEK_CUR:
		pos += offset;
		break;
	case SEEK_END:
		pos = (long)gamewad.idx[fp->lump].size + offset;
	default:
		fprintf(wadlog, "?][unknown whence]\n");
		return -1;
	}
	if (pos<0 || pos>(long)gamewad.idx[fp->lump].size) {
		fprintf(wadlog, "%li][out of range]\n", pos);
		return -1;
	}
	// update virtual position
	fprintf(wadlog, "%li]\n", pos);
	fp->pos = (dword)pos;
	if (fp->mirror) {
		int ms = fseek(fp->mirror, offset, whence);
		if (ms) {
			fprintf(wadlog, "[merror(s:%u)]", ms);
		}
	}
	return 0;
}
static long wad_rseek(wad_FILE *fp) {
	long off = (long)fp->offset + (long)fp->pos;
	long act;
	// check for virtual EOF
	if (fp->pos >= fp->size) {
		fprintf(wadlog, "[rseek:EOF(%lu>=%lu)]\n", fp->pos, fp->size);
		return 0;
	}
	// only seek underlying wad if necessary
	act = ftell(gamewad.fp);
	if (act!=off) {
		if (fseek(gamewad.fp, off, SEEK_SET)<0) {
			fprintf(wadlog, "[rseek(%lu):FAIL]\n", off);
			return -1;
		}
		fprintf(wadlog, "[rseek:%li=>%li]", act, off);
	}
	return off;
}
static int wad_fgetc(FILE *stream) {
	wad_FILE *fp = (wad_FILE*)stream;
	long sk;
	int rv;
	fprintf(wadlog, "wad_fgetc(%p)", fp);
	sk = wad_rseek(fp);
	if (sk<=0)
		return EOF;
	rv = fgetc(gamewad.fp);
	if (rv!=EOF)
		fp->pos += 1;
	else
		fprintf(wadlog, "[WAD EOF]");
	if (fp->mirror) {
		int mc = fgetc(fp->mirror);
		if (rv!=mc) {
			fprintf(wadlog, "[merror(v:%d!=%d)]", rv, mc);
		}
	}
	fprintf(wadlog, "=%i\n", rv);
	return rv;
}
static char *wad_fgets(char *ptr, int size, FILE *stream) {
	wad_FILE *fp = (wad_FILE*)stream;
	long sk;
	fprintf(wadlog, "wad_fgets(%p,%i,%p)", ptr, size, fp);
	sk = wad_rseek(fp);
	if (sk<=0)
		return NULL;
	if (fgets(ptr, size, gamewad.fp)==NULL) {
		fprintf(wadlog, "=NULL\n", fp);
		return NULL;
	}
	fp->pos += strlen(ptr);
	if (fp->mirror) {
		char *test = malloc(size);
		if (fgets(test, size, fp->mirror)==NULL || strncmp(ptr, test, strlen(ptr))) {
			fprintf(wadlog, "[merror(%s!=%s)]", ptr, test);
		}
		free(test);
	}
	fprintf(wadlog, "='%s'\n", ptr);
	return ptr;
}
static int wad_fscanf(FILE *stream, const char *format, ...) {
	wad_FILE *fp = (wad_FILE*)stream;
	int rv = 0;
	va_list ap;
	long sk;
	fprintf(wadlog, "wad_fscanf(%p,\'%s\',...)", fp, format);
	sk = wad_rseek(fp);
	if (sk<=0)
		return (int)sk;
	va_start(ap, format);
	rv = vfscanf(gamewad.fp, format, ap);
	va_end(ap);
	fp->pos = (dword)(ftell(gamewad.fp) - (long)fp->offset);
	if (fp->mirror) {
		// WARNING: implicit knowledge of caller..
		int v, *pv;
		va_list tp;
		int mc = fscanf(fp->mirror, format, &v);
		if (mc!=rv) {
			fprintf(wadlog, "[merror((r:%d!=%d)]", rv, mc);
		}
		va_start(tp, format);
		pv = va_arg(tp, int*);
		if (v!=*pv) {
			fprintf(wadlog, "[merror(v:%d!=%d)]", v, *pv);
		}
		va_end(tp);
	}
	fprintf(wadlog, "=%i\n", rv);
	return rv;
}
// IMPORTANT!! size_t is 16-bit here in TurboC++ land
static size_t wad_fread(void *ptr, size_t size, size_t count, FILE *stream) {
	wad_FILE *fp = (wad_FILE*)stream;
	size_t n = count;
	long sk;
	fprintf(wadlog, "wad_fread(%p,%u,%u,%p)", ptr, size, count, fp);
	sk = wad_rseek(fp);
	if (sk<=0)
		return (size_t)sk;
	// prevent reads past virtual EOF
	if ((fp->pos+n*size)>fp->size) {
		n = (fp->size-fp->pos)/size;
	}
	n = fread(ptr, size, n, gamewad.fp);
	if (n>0) {
		fp->pos += size*n;
	}
	if (fp->mirror) {
		void *test = calloc(count, size);
		size_t mr = fread(test, size, count, fp->mirror);
		if (mr!=n) {
			fprintf(wadlog, "[merror(r:%d!=%d)]", n, mr);
		}
		if (memcmp(ptr, test, n*size)) {
			fprintf(wadlog, "[merror(b:memcmp)]");
		}
		free(test);
	}
	fprintf(wadlog, "=%u\n", n);
	return n;
}

void LT_Use_WAD() {
	//Use our WAD packed content
	LT_fopen = wad_fopen;
	LT_fclose = wad_fclose;
	LT_fseek = wad_fseek;
	LT_fgetc = wad_fgetc;
	LT_fgets = wad_fgets;
	LT_fscanf = wad_fscanf;
	LT_fread = wad_fread;
#ifdef XWAD_LOG
	wadlog = fopen("wad.log", "w");
#endif
}
