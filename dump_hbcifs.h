#ifndef DUMP_HBCIFS_H
#define DUMP_HBCIFS_H

#include <iostream>
#include <cstdlib>
#include <cstddef>
#include <cstdarg>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#include <cstring>
#include <utime.h>
#include <sys/stat.h>
#include <libgen.h>

#include <zlib.h>
#include <lzo/lzo1x.h>
#include <ucl/ucl.h>

#include <string>

#include "compat.h"
#include "elf.h"
#include "image.h"
#include "startup.h"
#include "hbcifs.h"

using namespace std;

class Dump_HBCIFS
{

#define min(a,b)	((a)<=(b)?(a):(b))
#define FLAG_EXTRACT		0x00000001
#define FLAG_DISPLAY		0x00000002
#define FLAG_BASENAME		0x00000004
#define FLAG_MD5			0x00000008
#if 1
#define ENDIAN_RET32(x)		((((x) >> 24) & 0xff) | \
                            (((x) >> 8) & 0xff00) | \
                            (((x) & 0xff00) << 8) | \
                            (((x) & 0xff) << 24))

#define ENDIAN_RET16(x)		((((x) >> 8) & 0xff) | \
                            (((x) & 0xff) << 8))
#else
#define ENDIAN_RET32(x)	(x)
#define ENDIAN_RET16(x) (x)
#endif
#define	CROSSENDIAN(big)	(big)
#define MD5_LENGTH            16

public:
    Dump_HBCIFS();
    ~Dump_HBCIFS();

    int scanfile(char* dest_dir_path, char* image, FILE *fp);

    char *progname;
    char *ucompress_file;
    unsigned flags;
    int verbose;
    int files_to_extract;
    int files_left_to_extract;
    int processing_done;
    int zero_check_enabled = 1;
    char **check_files;

    struct extract_file {
        struct extract_file *next;
        char *path;
    } *extract_files = NULL;

private:
    void process_ifs(const char *file, FILE *fp);
    void process_image(const char *file, FILE *fp, bool called_direct);
    void process_hbcifs(const char *file, FILE *fp);

    void error(int level, char *p, ...);
    int find(FILE *fp, const unsigned char *p, int len, int count);
    int check(const char *name);
    char *associate(int ix, char *value);
    int copy(FILE *from, FILE *to, int nbytes);
    void display_script(FILE *fp, int pos, int len);
    void display_shdr(FILE *fp, int spos, struct startup_header *hdr);
    void display_ihdr(FILE *fp, int ipos, struct image_header *hdr);
    void display_attr(struct image_dirent::image_attr *attr);
    void display_dir(FILE *fp, int ipos, struct image_dirent::image_dir *ent);
    void process_dir(FILE *fp, int ipos, struct image_dirent::image_dir *ent);
    void display_symlink(FILE *fp, int ipos, struct image_dirent::image_symlink *ent);
    void process_symlink(FILE *fp, int ipos, struct image_dirent::image_symlink *ent);
    void display_device(FILE *fp, int ipos, struct image_dirent::image_device *ent);
    void process_device(FILE *fp, int ipos, struct image_dirent::image_device *ent);
    int mkdir_p(const char *path);
    void extract_file(FILE *fp, int ipos, struct image_dirent::image_file *ent);
    void process_file(FILE *fp, int ipos, struct image_dirent::image_file *ent);
    void display_file(FILE *fp, int ipos, struct image_dirent::image_file *ent);
    void dsp_index(const char *strv[], unsigned max, int index);
    void display_elf(FILE *fp, int pos, int size, char *name);
    void compute_md5(FILE *fp, int ipos, struct image_dirent::image_file *ent, unsigned char md5_result[16]);
    int zero_ok (struct startup_header *shdr);
    int rifs_checksum(void *ptr, long len);

    int start_search_loc;
    int process_loc;
};

#endif // DUMP_HBCIFS_H
