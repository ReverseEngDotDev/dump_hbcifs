#include "dump_hbcifs.h"
#include "hdr.h"

#define _DISABLE_MD5_

Dump_HBCIFS::Dump_HBCIFS()
{

}

Dump_HBCIFS::~Dump_HBCIFS()
{

}

int Dump_HBCIFS::scanfile(char *dest_dir_path, char *image, FILE *fp)
{
    int                         ifs_loc;
    struct startup_header		shdr = { STARTUP_HDR_SIGNATURE };
    int                         image_loc;
    struct image_header 		ihdr = { IMAGE_SIGNATURE };
    int                         hbcifs_loc;
    struct hbcifs_header        hbchdr = { HBCIFS_SIGNATURE };
    int                         tfd = -1;
    char                        tpath[_POSIX_PATH_MAX];
    char                        pwd[_POSIX_PATH_MAX];

    if ( dest_dir_path != NULL ) {
        /* This temp file is created to address the problems of creating a file in /dev/shmem */
        sprintf( tpath, "%s/.dumpifs.%d", dest_dir_path, getpid() );

        tfd = open( tpath, O_CREAT|O_RDONLY, 0644 );
        if ( tfd == -1 ) {
            if ( (access( dest_dir_path, W_OK ) != 0 ) && (mkdir( dest_dir_path, 0755) != 0)) {
                error(0, (char *)("Cannot create directory %s to extract %s"), dest_dir_path, image);
            }
        }
        getcwd(pwd, _POSIX_PATH_MAX - 1);
        cout << "Get CWD: " << pwd << endl;
        if (chdir(dest_dir_path) != 0) {
            error(0, (char *)("Cannot cd to directory %s to extract %s"), dest_dir_path, image);
        }
    }

    start_search_loc = 0;
    process_loc = -1;
    while((fgetc(fp)) != EOF)
    {
        fseek(fp, start_search_loc, SEEK_SET);
        ifs_loc = find(fp, (const unsigned char *)&shdr.signature, sizeof shdr.signature, -1);
        fseek(fp, start_search_loc, SEEK_SET);
        image_loc = find(fp, (const unsigned char *)&ihdr.signature, sizeof ihdr.signature, -1);
        fseek(fp, start_search_loc, SEEK_SET);
        hbcifs_loc = find(fp, (const unsigned char *)&hbchdr.signature, sizeof hbchdr.signature, -1);

        if ((ifs_loc == -1) && (image_loc == -1) && (hbcifs_loc == -1)) break;

        if ((ifs_loc != -1) && (ifs_loc < image_loc) || (image_loc == -1))
        {
            if ((ifs_loc != -1) && (ifs_loc < hbcifs_loc) || (hbcifs_loc == -1))
            {
                cout << " ********** Process IFS at: " << hex << ifs_loc << endl << endl;
                process_loc = ifs_loc;
                fseek(fp, process_loc, SEEK_SET);
                process_ifs(image, fp);
            }
            else
            {
                cout << " ********** Process HBCIFS at: " << hex << hbcifs_loc << endl << endl;
                process_loc = hbcifs_loc;
                fseek(fp, process_loc, SEEK_SET);
                process_hbcifs(image, fp);
            }
        }
        else
        {
            if ((image_loc != -1) && (image_loc < hbcifs_loc) || (hbcifs_loc == -1))
            {
                cout << " ********** Process ImageFS at: " << hex << image_loc << endl << endl;
                process_loc = image_loc;
                fseek(fp, process_loc, SEEK_SET);
                process_image(image, fp, true);
            }
            else
            {
                cout << " ********** Process HBCIFS at: " << hex << hbcifs_loc << endl << endl;
                process_loc = hbcifs_loc;
                fseek(fp, process_loc, SEEK_SET);
                process_hbcifs(image, fp);
            }
        }

        cout << "\n ********** Continue search at: " << hex << start_search_loc << endl;
    }

    cout << "Nothing left to extract from " << image << endl;

    if ( tfd != -1 ) {
        close(tfd);

        if (chdir(pwd) != 0) {
            error(0, (char *)("Cannot return to directory %s to delete %s"), pwd, tpath);
        }

        int result = unlink(tpath);
        if (result) {
            cout << "Could not delete " << tpath << endl;
        }
    }

    return EXIT_SUCCESS;
}

void Dump_HBCIFS::error(int level, char *p, ...)
{
    va_list				ap;

    fprintf(stderr, "%s: ", progname);
    va_start(ap, p);
    vfprintf(stderr, p, ap);
    va_end(ap);

    fputc('\n', stderr);

    if(level == 0) {
        exit(EXIT_FAILURE);
    }
}

int Dump_HBCIFS::find(FILE *fp, const unsigned char *p, int len, int count)
{
    int					c;
    int					i;
    int					n;

    i = n = 0;
    while(i < len && (c = fgetc(fp)) != EOF) {
        n++;
        if(c == p[i]) {
            i++;
        } else {
            if(count != -1 && n >= count) {
                break;
            }
            i = c == p[0] ? 1 : 0;
        }
    }
    if(i < len) {
        return -1;
    }
    return ftell(fp) - i;
}

int Dump_HBCIFS::check(const char *name)
{
    char			**p;

    if(!(p = check_files)) {
        return 0;
    }
    name = basename((char *)name);
    while(*p) {
        if(strcmp(name, *p) == 0) {
            return 0;
        }
        p++;
    }
    return -1;
}

char* Dump_HBCIFS::associate(int ix, char *value)
{
    static int	num = 0;
    static char	**text = NULL;
    char		**ptr;

    if (value != NULL){
        if (ix >= num) {
            if ((ptr = (char **)(realloc(text, (ix + 1) * sizeof(char *)))) == NULL)
                return(value);
            memset(&ptr[num], 0, (ix - num) * sizeof(char *));
            text = ptr, num = ix + 1;
        }
        text[ix] = strdup(value);
    }
    else {
        value = (char *)((ix < num) ? text[ix] : "");
    }
    return(value);
}

void Dump_HBCIFS::display_script(FILE *fp, int pos, int len)
{
    int								off;
    char							buff[512];
    union script_cmd				*hdr = (union script_cmd *)buff;
    int								size;
    int								ext_sched = SCRIPT_SCHED_EXT_NONE;

    for(off = 0; off < len; off += size) {
        fseek(fp, pos + off, SEEK_SET);
        if(fread(hdr, sizeof *hdr, 1, fp) != 1) {
            break;
        }
        if((size = hdr->hdr.size_lo | (hdr->hdr.size_hi << 8)) == 0) {
            break;
        }

        if(size > sizeof *hdr) {
            int n = min(sizeof buff, size - sizeof *hdr);

            if(fread(hdr + 1, n, 1, fp) != 1) {
                break;
            }
        }
        printf("                       ");
        switch(hdr->hdr.type) {
        case SCRIPT_TYPE_EXTERNAL: {
            char			*cmd, *args, *envs;
            int				i;

            cmd = hdr->external.args;
            envs = args = cmd + strlen(cmd) + 1;
            for(i = 0; i < hdr->external.argc; i++) {
                envs = envs + strlen(envs) + 1;
            }
            i = strcmp(basename(cmd), args);

            if(i || (hdr->external.flags & (SCRIPT_FLAGS_SESSION | SCRIPT_FLAGS_KDEBUG | SCRIPT_FLAGS_SCHED_SET | SCRIPT_FLAGS_CPU_SET | SCRIPT_FLAGS_EXTSCHED))) {
                printf("[ ");
                if(i) {
                    printf("argv0=%s ", args);
                }
                if(hdr->external.flags & SCRIPT_FLAGS_SESSION) {
                    printf("+session ");
                }
                if(hdr->external.flags & SCRIPT_FLAGS_KDEBUG) {
                    printf("+debug ");
                }
                if(hdr->external.flags & SCRIPT_FLAGS_SCHED_SET) {
                    printf("priority=%d", hdr->external.priority);
                    switch(hdr->external.policy) {
                    case SCRIPT_POLICY_NOCHANGE:
                        break;
                    case SCRIPT_POLICY_FIFO:
                        printf("f");
                        break;
                    case SCRIPT_POLICY_RR:
                        printf("r");
                        break;
                    case SCRIPT_POLICY_OTHER:
                        printf("o");
                        break;
                    default:
                        printf("?%d?", hdr->external.policy);
                        break;
                    }
                    printf(" ");
                }
                if(hdr->external.flags & SCRIPT_FLAGS_CPU_SET) {
                    printf("cpu=%d ", hdr->external.cpu);
                }
                if(hdr->external.flags & SCRIPT_FLAGS_EXTSCHED) {
                    printf("sched_aps=%s ", associate(hdr->external.extsched.aps.id, NULL));
                }
                printf("] ");
            }
            for(i = 0; i < hdr->external.envc; i++) {
                printf("%s ", envs);
                envs = envs + strlen(envs) + 1;
            }
            printf("%s", cmd);

            args = args + strlen(args) + 1;
            for(i = 1; i < hdr->external.argc; i++) {
                printf(" %s", args);
                args = args + strlen(args) + 1;
            }

            if(hdr->external.flags & SCRIPT_FLAGS_BACKGROUND) {
                printf(" &");
            }
            printf("\n");
            break;
        }

        case SCRIPT_TYPE_WAITFOR:
            printf("waitfor");
            goto wait;
        case SCRIPT_TYPE_REOPEN:
            printf("reopen");
        wait:
            if(hdr->waitfor_reopen.checks_lo || hdr->waitfor_reopen.checks_hi) {
                int					checks = hdr->waitfor_reopen.checks_lo | hdr->waitfor_reopen.checks_hi << 8;

                printf(" %d.%d", checks / 10, checks % 10);
            }
            printf(" %s\n", hdr->waitfor_reopen.fname);
            break;

        case SCRIPT_TYPE_DISPLAY_MSG:
            printf("display_msg '%s'\n", hdr->display_msg.msg);
            break;

        case SCRIPT_TYPE_PROCMGR_SYMLINK:
            printf("procmgr_symlink '%s' '%s'\n", hdr->procmgr_symlink.src_dest,
                   &hdr->procmgr_symlink.src_dest[strlen(hdr->procmgr_symlink.src_dest)+1]);
            break;

        case SCRIPT_TYPE_EXTSCHED_APS:
            if (ext_sched == SCRIPT_SCHED_EXT_NONE)
                associate(SCRIPT_APS_SYSTEM_PARTITION_ID, (char *)(SCRIPT_APS_SYSTEM_PARTITION_NAME));
            else if (ext_sched != SCRIPT_SCHED_EXT_APS)
                printf("Invalid combination of SCHED_EXT features\n");
            ext_sched = SCRIPT_SCHED_EXT_APS;
            printf("sched_aps %s %d %d\n", associate(hdr->extsched_aps.id, hdr->extsched_aps.pname), hdr->extsched_aps.budget, hdr->extsched_aps.critical_hi << 8 | hdr->extsched_aps.critical_lo);
            break;

        default:
            printf("Unknown type %d\n", hdr->hdr.type);
        }
    }
}

void Dump_HBCIFS::process_ifs(const char *file, FILE *fp)
{
    struct startup_header		shdr = { STARTUP_HDR_SIGNATURE };
    int							spos;
    static unsigned char					buf[0x10000];
    static unsigned char 				out_buf[0x10000];

    //find startup signature and verify its validity
    while(1){
        if((spos = find(fp, (const unsigned char *)&shdr.signature, sizeof shdr.signature, -1)) == -1) {

            shdr.signature = ENDIAN_RET32(shdr.signature);
            rewind(fp);
            if((spos = find(fp, (const unsigned char *)&shdr.signature, sizeof shdr.signature, -1)) == -1) {
                error(1, (char *)("Unable to find startup header in %s"), file);
                return;
            }
        }
        if(fread((char *)&shdr + sizeof shdr.signature, sizeof shdr - sizeof shdr.signature, 1, fp) != 1) {
            error(1, (char *)("Unable to read image %s"), file);
            return;
        }
        // if we are cross endian, flip the shdr members
        {
            // flip shdr members
        }

        //check the zero member of startup, one more check to confirm
        //we found the correct signature
        if (!zero_ok(&shdr)) {
            if (zero_check_enabled) {
                error(1, (char *)("Zero check failed, not a valid IFS header. Continuing search"));
                continue;
            }
            if (verbose)
                printf("Warning: Non zero data in zero fields ignored\n");
            break;
        } else {
            break;
        }
    }
    printf(" Image startup_size: %d (0x%x)\n", shdr.startup_size, shdr.startup_size);
    printf(" Image stored_size: %d (0x%x)\n", shdr.stored_size, shdr.stored_size);
    printf(" Compressed size: %lu (0x%lx)\n", shdr.stored_size - shdr.startup_size - sizeof(struct image_trailer), shdr.stored_size - shdr.startup_size - sizeof(struct image_trailer));

    start_search_loc = process_loc + shdr.stored_size;

    // If the image is compressed we need to uncompress it into a
    // tempfile and restart.
    if((shdr.flags1 & STARTUP_HDR_FLAGS1_COMPRESS_MASK) != 0) {
        FILE	*fp2;
        int		n;

        // Create a file to hold uncompressed image.
        if(ucompress_file) {
            fp2 = fopen(ucompress_file, "w+");
        } else {
            fp2 = tmpfile();
        }

        if(fp2 == NULL) {
            error(1, (char *)("Unable to create a file to uncompress image."));
            return;
        }

        // Copy non-compressed part.
        rewind(fp);
        if(CROSSENDIAN(shdr.flags1 & STARTUP_HDR_FLAGS1_BIGENDIAN))
            for(n = 0 ; n < spos + ENDIAN_RET32(shdr.startup_size); ++n) {
                putc(getc(fp), fp2);
            }
        else
            for(n = 0 ; n < spos + shdr.startup_size; ++n) {
                putc(getc(fp), fp2);
            }
        fflush(fp2);

        // Uncompress compressed part
        switch(shdr.flags1 & STARTUP_HDR_FLAGS1_COMPRESS_MASK) {
        case STARTUP_HDR_FLAGS1_COMPRESS_ZLIB:
        {
            int			fd;
            gzFile		zin;

            // We need an fd for zlib, and we cannot count on the fd
            // position being the same as the FILE *'s, so fileno()
            // and lseek.
            fd = fileno(fp);
            lseek(fd, ftell(fp), SEEK_SET);
            if((zin = gzdopen(fd, "rb")) == NULL) {
                error(1, (char *)("Unable to open decompression stream."));
                return;
            }
            while((n = gzread(zin, buf, sizeof(buf))) > 0) {
                printf("zlib Decompress\n");
                fwrite(buf, n, 1, fp2);
            }
        }
        break;
        case STARTUP_HDR_FLAGS1_COMPRESS_LZO:
        {
            unsigned	len, til=0, tol=0;
            lzo_uint	out_len;
            int			status;

            if(lzo_init() != LZO_E_OK) {
                error(1,(char *)("decompression init failure"));
                return;
            }
            printf("LZO Decompress @0x%0lx\n", ftell(fp));
            for(;;) {
                unsigned int nowPtr=ftell(fp);
                len = getc(fp) << 8;
                len += getc(fp);

                if(len == 0) break;
                fread(buf, len, 1, fp);
                status = lzo1x_decompress(buf, len, out_buf, &out_len, NULL);
                if(status != LZO_E_OK) {
                    error(1, (char *)("decompression failure"));
                    printf("decompression not success with code %d. out_len=%lu.\n", status, out_len);
                    return;
                }
                til+=len;tol+=out_len;
                printf("LZO Decompress rd=%d, wr=%lu @ 0x%x\n", len, out_len, nowPtr);
                fwrite(out_buf, out_len, 1, fp2);
            }
            printf("Decompressed %d bytes-> %d bytes\n", til, tol);
        }
        break;
        case STARTUP_HDR_FLAGS1_COMPRESS_UCL:
        {
            unsigned	len, til=0, tol=0;
            ucl_uint	out_len;
            int			status;
            printf("UCL Decompress @0x%0lx\n", ftell(fp));
            for(;;) {
                unsigned int nowPtr = ftell(fp);
                len = getc(fp) << 8;
                len += getc(fp);
                if(len == 0) break;
                fread(buf, len, 1, fp);
                status = ucl_nrv2b_decompress_8(buf, len, out_buf, &out_len, NULL);
                if(status != 0) {
                    error(1, (char *)("decompression failure"));
                    return;
                }
                til+=len;tol+=out_len;
                printf("UCL Decompress rd=%d (0x%x), wr=%d @ 0x%x\n", len, len, out_len, nowPtr);
                fwrite(out_buf, out_len, 1, fp2);
            }
            printf("Decompressed %d bytes-> %d bytes\n", til, tol);
        }
        break;
        default:
            error(1, (char *)("Unsupported compression type."));
            return;
        }

        fp = fp2;
        rewind(fp2);
    }

    if(CROSSENDIAN(shdr.flags1 & STARTUP_HDR_FLAGS1_BIGENDIAN)) {
        unsigned long			*p;

        shdr.version = ENDIAN_RET16(shdr.version);
        shdr.machine = ENDIAN_RET16(shdr.machine);
        shdr.header_size = ENDIAN_RET16(shdr.header_size);
        shdr.preboot_size = ENDIAN_RET16(shdr.preboot_size);
        for(p = (unsigned long *)&shdr.startup_vaddr; (unsigned char *)p < (unsigned char *)&shdr + sizeof shdr; p++) {
            if(p != (unsigned long *)&shdr.preboot_size) {
                *p = ENDIAN_RET32(*p);
            }
        }
    }

    if(flags & FLAG_DISPLAY) {
        printf("   Offset     Size  Name\n");
        if(spos != -1) {
            if(spos) {
                if(check("*.boot") == 0) {
                    printf(" %8x %8x  %s\n", 0, spos, "*.boot");
                }
            }
            display_shdr(fp, spos, &shdr);
            if(check("startup.*") == 0) {
                printf(" %8lx %8lx  %s\n", spos + sizeof shdr, shdr.startup_size - sizeof shdr, "startup.*");
            }
        }
    }

    fseek(fp, spos + shdr.startup_size, SEEK_SET);

    process_image(file, fp, false);

    printf(" Image startup_size: %d (0x%x)\n", shdr.startup_size, shdr.startup_size);
    printf(" Image stored_size: %d (0x%x)\n", shdr.stored_size, shdr.stored_size);
    printf(" Compressed size: %lu (0x%lx)\n", shdr.stored_size - shdr.startup_size - sizeof(image_trailer), shdr.stored_size - shdr.startup_size - sizeof(image_trailer));
}

void Dump_HBCIFS::process_image(const char *file, FILE *fp, bool called_direct)
{
    struct image_header			ihdr = { IMAGE_SIGNATURE };
    int ipos = 0;
    int dpos = 0;

    if((ipos = find(fp, (const unsigned char*)(ihdr.signature), sizeof ihdr.signature, -1)) == -1) {
        error(1, (char *)("Unable to find image header in %s"), file);
        return;
    }

    if(fread((char *)&ihdr + sizeof ihdr.signature, sizeof ihdr - sizeof ihdr.signature, 1, fp) != 1) {
        error(1, (char *)("Unable to read image %s"), file);
        return;
    }
    if(CROSSENDIAN(ihdr.flags & IMAGE_FLAGS_BIGENDIAN)) {
        unsigned long			*p;

        for(p = (unsigned long *)&ihdr.image_size; (unsigned char *)p < (unsigned char *)&ihdr + offsetof(struct image_header, mountpoint); p++) {
            *p = ENDIAN_RET32(*p);
        }
    }

    if (called_direct) {
        start_search_loc = process_loc + ihdr.image_size;
    }

    dpos = ipos + ihdr.dir_offset;

    if(flags & FLAG_DISPLAY) {
        display_ihdr(fp, ipos, &ihdr);
        if(check("Image-directory") == 0) {
            printf(" %8x %8x  %s\n", dpos, ihdr.hdr_dir_size - ihdr.dir_offset, "Image-directory");
        }
    }

    while(!processing_done) {
        char						buff[1024];
        union image_dirent			*dir = (union image_dirent *)buff;

        fseek(fp, dpos, SEEK_SET);
        if(fread(&dir->attr, sizeof dir->attr, 1, fp) != 1) {
            error(1, (char *)("Early end reading directory"));
            break;
        }
        if(CROSSENDIAN(ihdr.flags & IMAGE_FLAGS_BIGENDIAN)) {
            unsigned long			*p;

            dir->attr.size = ENDIAN_RET16(dir->attr.size);
            dir->attr.extattr_offset = ENDIAN_RET16(dir->attr.extattr_offset);
            for(p = (unsigned long *)&dir->attr.ino; (unsigned char *)p < (unsigned char *)dir + sizeof dir->attr; p++) {
                *p = ENDIAN_RET32(*p);
            }
        }
        if(dir->attr.size < sizeof dir->attr) {
            if(dir->attr.size != 0) {
                error(1, (char *)("Invalid dir entry"));
            }
            break;
        }
        if(dir->attr.size > sizeof dir->attr) {
            if(fread((char *)dir + sizeof dir->attr, min(sizeof buff, dir->attr.size) - sizeof dir->attr, 1, fp) != 1) {
                error(1, (char *)("Error reading directory"));
                break;
            }
        }
        dpos += dir->attr.size;

        switch(dir->attr.mode & S_IFMT) {
        case S_IFREG:
            if(CROSSENDIAN(ihdr.flags & IMAGE_FLAGS_BIGENDIAN)) {
                dir->file.offset = ENDIAN_RET32(dir->file.offset);
                dir->file.size = ENDIAN_RET32(dir->file.size);
            }
            process_file(fp, ipos, &dir->file);
            if(dir->attr.ino == ihdr.script_ino && verbose > 1) {
                display_script(fp, ipos + dir->file.offset, dir->file.size);
            }
            break;
        case S_IFDIR:
            process_dir(fp, ipos, &dir->dir);
            break;
        case S_IFLNK:
            if(CROSSENDIAN(ihdr.flags & IMAGE_FLAGS_BIGENDIAN)) {
                dir->symlink.sym_offset = ENDIAN_RET16(dir->symlink.sym_offset);
                dir->symlink.sym_size = ENDIAN_RET16(dir->symlink.sym_size);
            }
            process_symlink(fp, ipos, &dir->symlink);
            break;
        case S_IFCHR:
        case S_IFBLK:
        case S_IFIFO:
        case S_IFNAM:
            if(CROSSENDIAN(ihdr.flags & IMAGE_FLAGS_BIGENDIAN)) {
                dir->device.dev = ENDIAN_RET32(dir->device.dev);
                dir->device.rdev = ENDIAN_RET32(dir->device.rdev);
            }
            process_device(fp, ipos, &dir->device);
            // dir->device;
            break;
        default:
            error(1, (char *)("Unknown type\n"));
            break;
        }
    }
    if(flags & FLAG_DISPLAY) {
        struct image_trailer	itlr;
        struct startup_trailer	stlr;

        fseek(fp, ipos + ihdr.image_size-sizeof(itlr), SEEK_SET);
        if(fread(&itlr, sizeof(itlr), 1, fp) != 1) {
            error(1, (char *)("Early end reading image trailer"));
            return;
        }
        //if(CROSSENDIAN(shdr.flags1 & STARTUP_HDR_FLAGS1_BIGENDIAN))
        //    printf("Checksums: image=%#x", ENDIAN_RET32(itlr.cksum));
        //else
        //    printf("Checksums: image=%#x", itlr.cksum);


        //if(spos != -1) {
        //    fseek(fp, spos + shdr.startup_size-sizeof(stlr), SEEK_SET);
        //    if(fread(&stlr, sizeof(stlr), 1, fp) != 1) {
        //        printf("\n");
        //        error(1, (char *)("Early end reading startup trailer"));
        //        return;
        //    }
        //    if(CROSSENDIAN(shdr.flags1 & STARTUP_HDR_FLAGS1_BIGENDIAN))
        //        printf(" startup=%#lx", ENDIAN_RET32(stlr.cksum));
        //    else
        //        printf(" startup=%#lx", stlr.cksum);
        //}
        //printf("\n");
        //printf("ipos=%#lx spos=%#lx ihdr.image_size=%#ld - sizeof(stlr) = %#ld\n", ipos, spos, ihdr.image_size, ihdr.image_size-sizeof(itlr));
        {
            int             data;
            int             sum;
            int		counted=0;
            int len = ihdr.image_size;
            unsigned        max;

            fseek(fp, ipos , SEEK_SET);
            sum = 0;
            while(len > 4)
            {
                fread(&data, 4, 1, fp);
                sum += data;
                counted +=4;
                len -= 4;
            }
            sum = -1*sum;
            if(0 != (itlr.cksum - sum))
            {
                printf("\nStored checksum not correct!\n");
                printf("Expected checksum: %#x\n", sum);
                printf("  NG: %02x %02x %02x %02x\n", itlr.cksum & 0xff, (itlr.cksum >> 8) & 0xff, (itlr.cksum >> 16) & 0xff, (itlr.cksum >> 24) & 0xff);
                printf("GOOD: %02x %02x %02x %02x\n", sum & 0xff, (sum >> 8) & 0xff, (sum >> 16) & 0xff, (sum >> 24) & 0xff);
            }
        }

    }
}

void Dump_HBCIFS::process_hbcifs(const char *file, FILE *fp)
{
    struct hbcifs_header		hbchdr = { HBCIFS_SIGNATURE };
    int							hpos;
    static unsigned char           *buf;
    static unsigned char           *out_buf;

    if((hpos = find(fp, (const unsigned char*)(hbchdr.signature), sizeof hbchdr.signature, -1)) == -1) {
        error(1, (char *)("Unable to find hbcifs header in %s"), file);
        return;
    }

    if(fread((char *)&hbchdr + sizeof hbchdr.signature, sizeof hbchdr - sizeof hbchdr.signature, 1, fp) != 1) {
        error(1, (char *)("Unable to read hbcifs %s"), file);
        return;
    }

    start_search_loc = process_loc + hbchdr.compressed_image_size + sizeof(hbchdr);

    buf = (unsigned char *)(malloc(hbchdr.decompresed_image_size));
    out_buf = (unsigned char *)(malloc(hbchdr.decompresed_image_size));

    if((hbchdr.compression01 & 0xFF) != 0) {
        FILE	*fp2;
        int		n;

        // Create a file to hold uncompressed image.
        if(ucompress_file) {
            fp2 = fopen(ucompress_file, "w+");
        } else {
            fp2 = tmpfile();
        }

        if(fp2 == NULL) {
            error(1, (char *)("Unable to create a file to uncompress image."));
            return;
        }

        // Copy non-compressed part.
        //rewind(fp);
        //if(CROSSENDIAN(hbchdr.flags1 & STARTUP_HDR_FLAGS1_BIGENDIAN))
        //    for(n = 0 ; n < spos + ENDIAN_RET32(shdr.startup_size); ++n) {
        //        putc(getc(fp), fp2);
        //    }
        //else
        //    for(n = 0 ; n < spos + shdr.startup_size; ++n) {
        //        putc(getc(fp), fp2);
        //    }
        //fflush(fp2);

        // Uncompress compressed part

        switch(hbchdr.compression01 & 0x0F) {
        case HBCIFS_COMPRESS1_ZLIB:
        {
            int			fd;
            gzFile		zin;

            // We need an fd for zlib, and we cannot count on the fd
            // position being the same as the FILE *'s, so fileno()
            // and lseek.
            fd = fileno(fp);
            lseek(fd, ftell(fp), SEEK_SET);
            if((zin = gzdopen(fd, "rb")) == NULL) {
                error(1, (char *)("Unable to open decompression stream."));
                return;
            }
            while((n = gzread(zin, buf, sizeof(buf))) > 0) {
                printf("zlib Decompress\n");
                fwrite(buf, n, 1, fp2);
            }
        }
        break;
        case HBCIFS_COMPRESS1_LZO:
        {
            unsigned	til=0, tol=0;
            lzo_uint    len;
            lzo_uint	out_len;
            int			status;

            if(lzo_init() != LZO_E_OK) {
                error(1,(char *)("decompression init failure"));
                return;
            }
            printf("LZO Decompress @0x%0lx\n", ftell(fp));
            unsigned int nowPtr=ftell(fp);
            len = hbchdr.compressed_image_size;

            fread(buf, len, 1, fp);

            status = lzo1x_decompress(buf, len, out_buf, &out_len, NULL);
            if(status != LZO_E_OK) {
                error(1, (char *)("decompression failure"));
                printf("decompression not success with code %d. out_len=%lu.\n", status, out_len);
                return;
            }
            til+=len;tol+=out_len;
            printf("LZO Decompress rd=%lu, wr=%lu @ 0x%x\n", len, out_len, nowPtr);
            fwrite(out_buf, out_len, 1, fp2);
            printf("Decompressed %d bytes-> %d bytes\n", til, tol);
        }
        break;
        case HBCIFS_COMPRESS1_UCL:
        {
            unsigned	til=0, tol=0;
            ucl_uint    len;
            ucl_uint	out_len;
            int			status;
            printf("UCL Decompress @0x%0lx\n", ftell(fp));
            unsigned int nowPtr = ftell(fp);
            len = hbchdr.compressed_image_size;
            fread(buf, len, 1, fp);
            status = ucl_nrv2b_decompress_8(buf, len, out_buf, &out_len, NULL);
            if(status != 0) {
                error(1, (char *)("decompression failure"));
                return;
            }
            til+=len;tol+=out_len;
            printf("UCL Decompress rd=%d (0x%x), wr=%d @ 0x%x\n", len, len, out_len, nowPtr);
            fwrite(out_buf, out_len, 1, fp2);
            printf("Decompressed %d bytes-> %d bytes\n", til, tol);
        }
        break;
        default:
            error(1, (char *)("Unsupported compression type."));
            return;
        }

        fp = fp2;
        rewind(fp2);
    }

    //if(CROSSENDIAN(shdr.flags1 & STARTUP_HDR_FLAGS1_BIGENDIAN)) {
    //    unsigned long			*p;

    //    shdr.version = ENDIAN_RET16(shdr.version);
    //    shdr.machine = ENDIAN_RET16(shdr.machine);
    //    shdr.header_size = ENDIAN_RET16(shdr.header_size);
    //    shdr.preboot_size = ENDIAN_RET16(shdr.preboot_size);
    //    for(p = (unsigned long *)&shdr.startup_vaddr; (unsigned char *)p < (unsigned char *)&shdr + sizeof shdr; p++) {
    //        if(p != (unsigned long *)&shdr.preboot_size) {
    //            *p = ENDIAN_RET32(*p);
    //        }
    //    }
    //}

    process_image(file, fp, false);

    free(buf);
    free(out_buf);
}

int Dump_HBCIFS::copy(FILE *from, FILE *to, int nbytes)
{
    char			buff[4096];
    int				n;

    while(nbytes > 0) {
        n = min(sizeof buff, nbytes);
        if(fread(buff, n, 1, from) != 1) {
            return -1;
        }
        if(fwrite(buff, n, 1, to) != 1) {
            return -1;
        }
        nbytes -= n;
    }
    return 0;
}

void Dump_HBCIFS::display_shdr(FILE *fp, int spos, struct startup_header *hdr)
{
    if(check("Startup-header") != 0) {
        return;
    }
    printf(" %8x %8x  %s flags1=%#x flags2=%#x paddr_bias=%#x\n",
           spos, hdr->header_size, "Startup-header",
           hdr->flags1, hdr->flags2,
           hdr->paddr_bias);
    if(verbose) {
        printf("                       preboot_size=%#lx\n", (unsigned long)hdr->preboot_size);
        printf("                       image_paddr=%#lx stored_size=%#lx\n", (unsigned long)hdr->image_paddr, (unsigned long)hdr->stored_size);
        printf("                       startup_size=%#lx imagefs_size=%#lx\n", (unsigned long)hdr->startup_size, (unsigned long)hdr->imagefs_size);
        printf("                       ram_paddr=%#lx ram_size=%#lx\n", (unsigned long)hdr->ram_paddr, (unsigned long)hdr->ram_size);
        printf("                       startup_vaddr=%#lx\n", (unsigned long)hdr->startup_vaddr);
    }
}

void Dump_HBCIFS::display_ihdr(FILE *fp, int ipos, struct image_header *hdr)
{
    if(check("Image-header") != 0) {
        return;
    }
    printf(" %8x %8lx  %s", ipos, sizeof *hdr, "Image-header");
    if(hdr->mountpoint[0]) {
        char				buff[512];

        fseek(fp, ipos + offsetof(struct image_header, mountpoint), SEEK_SET);
        if(fread(buff, min(sizeof buff, hdr->dir_offset - offsetof(struct image_header, mountpoint)), 1, fp) != 1) {
            error(1, (char *)("Unable to read image mountpoint"));
        } else {
            printf(" mountpoint=%s", buff);
        }
    }
    printf("\n");
    if(verbose) {
        int						i;

        printf("                       flags=%#x", hdr->flags);
        if(hdr->chain_paddr) {
            printf(" chain=%#x", hdr->chain_paddr);
        }
        if(hdr->script_ino) {
            printf(" script=%u", hdr->script_ino);
        }
        for(i = 0; i < sizeof hdr->boot_ino / sizeof hdr->boot_ino[0]; i++) {
            if(hdr->boot_ino[i]) {
                printf(" boot=%u", hdr->boot_ino[i]);
                for(i++; i < sizeof hdr->boot_ino / sizeof hdr->boot_ino[0]; i++) {
                    if(hdr->boot_ino[i]) {
                        printf(",%u", hdr->boot_ino[i]);
                    }
                }
                break;
            }
        }
        for(i = 0; i < sizeof hdr->spare / sizeof hdr->spare[0]; i++) {
            if(hdr->spare[i]) {
                printf(" spare=%#x", hdr->spare[0]);
                for(i = 1; i < sizeof hdr->spare / sizeof hdr->spare[0]; i++) {
                    printf(",%u", hdr->spare[i]);
                }
                break;
            }
        }
        printf(" mntflg=%#x\n", hdr->mountflags);
    }
}

void Dump_HBCIFS::display_attr(struct image_dirent::image_attr *attr)
{
    printf("                       gid=%u uid=%u mode=%#o ino=%u mtime=%x\n", attr->gid, attr->uid, attr->mode & ~S_IFMT, attr->ino, attr->mtime);
}

void Dump_HBCIFS::display_dir(FILE *fp, int ipos, struct image_dirent::image_dir *ent)
{
    if(check(ent->path[0] ? ent->path : "Root-dirent") != 0) {
        return;
    }
    printf("     ----     ----  %s\n", ent->path[0] ? ent->path : "Root-dirent");
    if(verbose) {
        display_attr(&ent->attr);
    }
}

void Dump_HBCIFS::process_dir(FILE *fp, int ipos, struct image_dirent::image_dir *ent)
{
    if(flags & FLAG_DISPLAY) {
        display_dir(fp, ipos, ent);
    }
}

void Dump_HBCIFS::display_symlink(FILE *fp, int ipos, struct image_dirent::image_symlink *ent)
{
    if(check(ent->path) != 0) {
        return;
    }
    printf("     ---- %8x  %s -> %s\n", ent->sym_size, ent->path, &ent->path[ent->sym_offset]);
    if(verbose) {
        display_attr(&ent->attr);
    }
}

void Dump_HBCIFS::process_symlink(FILE *fp, int ipos, struct image_dirent::image_symlink *ent)
{
    if(flags & FLAG_DISPLAY) {
        display_symlink(fp, ipos, ent);
    }
}

void Dump_HBCIFS::display_device(FILE *fp, int ipos, struct image_dirent::image_device *ent) {
    if(check(ent->path) != 0) {
        return;
    }
    printf("     ----     ----  %s dev=%u rdev=%u\n", ent->path, ent->dev, ent->rdev);

    if(verbose) {
        display_attr(&ent->attr);
    }
}

void Dump_HBCIFS::process_device(FILE *fp, int ipos, struct image_dirent::image_device *ent) {
    if(flags & FLAG_DISPLAY) {
        display_device(fp, ipos, ent);
    }
}

int Dump_HBCIFS::mkdir_p(const char *path)
{
    /* Adapted from http://stackoverflow.com/a/2336245/119527 */
    const size_t len = strlen(path);
    char _path[PATH_MAX] = "temp/dir";
    char *p;

    errno = 0;

    /* Copy string so its mutable */
    if (len > sizeof(_path)-1) {
        errno = ENAMETOOLONG;
        return -1;
    }

    strcpy(_path, path);

    /* Iterate the string */
    for (p = _path + 1; *p; p++) {
        if (*p == '/') {
            /* Temporarily truncate */
            *p = '\0';

            if (mkdir(_path, S_IRWXU) != 0) {
                if (errno != EEXIST)
                    return -1;
            }

            *p = '/';
        }
    }

    if (mkdir(_path, S_IRWXU) != 0) {
        if (errno != EEXIST)
            return -1;
    }

    return 0;
}

void Dump_HBCIFS::extract_file(FILE *fp, int ipos, struct image_dirent::image_file *ent)
{
    char			*name;
    char            dirnames[PATH_MAX] = "/";   //POSIX dirnames truncates the string
    FILE			*dst;
    struct utimbuf	buff;
    struct extract_file *ef;

    if(check(ent->path) != 0) {
        return;
    }

    name = (flags & FLAG_BASENAME) ? basename(ent->path) : ent->path;

    if ( extract_files != NULL ) {
        for ( ef = extract_files; ef != NULL; ef = ef->next ) {
            if ( !strcmp( ef->path, name ) ) {
                break;
            }
        }
        if ( ef == NULL ) {
            return;
        }
        if ( --files_left_to_extract == 0 ) {
            processing_done = 1;
        }
    }

    strcpy(dirnames, ent->path);
    if(mkdir_p(dirname(dirnames))) {
        printf("unable to mkdir -p for %s\n", name);
        return;
    }

    if(!(dst = fopen(name, "wb"))) {
        error(0, (char *)("Unable to open %s: %s\n"), name, strerror(errno));
    }

    fseek(fp, ipos + ent->offset, SEEK_SET);
    ftruncate(fileno(dst), ent->size); /* pregrow the dst file */
    printf("%s ... size %d\n", name, ent->size);
    if(copy(fp, dst, ent->size) == -1) {
        unlink(name);
        error(0, (char *)("Unable to create file %s: %s\n"), name, strerror(errno));
    }
    fchmod(fileno(dst), ent->attr.mode & 07777);
    fchown(fileno(dst), ent->attr.uid, ent->attr.gid);
    fclose(dst);
    buff.actime = buff.modtime = ent->attr.mtime;
    utime(name, &buff);
    if(verbose) {
        printf("Extracted %s\n", name);
    }
}

void Dump_HBCIFS::process_file(FILE *fp, int ipos, struct image_dirent::image_file *ent)
{
    if(flags & FLAG_EXTRACT) {
        extract_file(fp, ipos, ent);
    }
    if(flags & (FLAG_MD5|FLAG_DISPLAY)) {
        display_file(fp, ipos, ent);
    }
}

void Dump_HBCIFS::dsp_index(const char *strv[], unsigned max, int index)
{
    if(index < 0 || index >= max || !strv[index]) {
        printf("Unknown(%d)", index);
    } else {
        printf("%s", strv[index]);
    }
}

void Dump_HBCIFS::display_elf(FILE *fp, int pos, int size, char *name)
{
    Elf32_Ehdr			elfhdr;

    fseek(fp, pos, SEEK_SET);
    if(fread(&elfhdr, sizeof elfhdr, 1, fp) != 1) {
        return;
    }
    if(memcmp(elfhdr.e_ident, ELFMAG, SELFMAG)) {
        return;
    }
    if(verbose <= 2) {
        printf("                       ");
    }
    printf("----- %s ", name);
    if(elfhdr.e_ident[EI_VERSION] != EV_CURRENT) {
        printf("ELF version %d -----\n", elfhdr.e_ident[EI_VERSION]);
        return;
    }
    switch(elfhdr.e_ident[EI_CLASS]) {
    case ELFCLASS32:
        printf("- ELF32");
        break;
    case ELFCLASS64:
        printf("- ELF64");
        break;
    default:
        printf("ELF executable - invalid class -----\n");
        return;
    }

    switch(elfhdr.e_ident[EI_DATA]) {
    case ELFDATA2LSB:
        printf("LE ");
        break;
    case ELFDATA2MSB:
        printf("BE ");
        break;
    default:
        printf(" executable - unknown endian -----\n");
        return;
    }
    if(elfhdr.e_ident[EI_CLASS] == ELFCLASS64) {
        printf("-----\n");
        return;
    }

    if(CROSSENDIAN(elfhdr.e_ident[EI_DATA] == ELFDATA2MSB)) {
        elfhdr.e_type = ENDIAN_RET16(elfhdr.e_type);
        elfhdr.e_machine = ENDIAN_RET16(elfhdr.e_machine);
        elfhdr.e_version = ENDIAN_RET32(elfhdr.e_version);
        elfhdr.e_entry = ENDIAN_RET32(elfhdr.e_entry);
        elfhdr.e_phoff = ENDIAN_RET32(elfhdr.e_phoff);
        elfhdr.e_shoff = ENDIAN_RET32(elfhdr.e_shoff);
        elfhdr.e_flags = ENDIAN_RET32(elfhdr.e_flags);
        elfhdr.e_ehsize = ENDIAN_RET16(elfhdr.e_ehsize);
        elfhdr.e_phentsize = ENDIAN_RET16(elfhdr.e_phentsize);
        elfhdr.e_phnum = ENDIAN_RET16(elfhdr.e_phnum);
        elfhdr.e_shentsize = ENDIAN_RET16(elfhdr.e_shentsize);
        elfhdr.e_shnum = ENDIAN_RET16(elfhdr.e_shnum);
        elfhdr.e_shstrndx = ENDIAN_RET16(elfhdr.e_shstrndx);
    }

    if(elfhdr.e_ehsize < sizeof elfhdr) {
        printf("-----\n");
        error(1, (char *)("ehdr too small"));
        return;
    }

    dsp_index(ehdr_type, sizeof ehdr_type / sizeof *ehdr_type, elfhdr.e_type);
    printf(" ");
    dsp_index(ehdr_machine, sizeof ehdr_machine / sizeof *ehdr_machine, elfhdr.e_machine);
    printf(" -----\n");
    if(verbose <= 2) {
        return;
    }
    if(elfhdr.e_ehsize != sizeof elfhdr) {
        printf(" e_ehsize             : %d\n", elfhdr.e_ehsize);
    }
    printf(" e_flags              : %#lx\n", (unsigned long)elfhdr.e_flags);
    if(elfhdr.e_entry || elfhdr.e_phoff || elfhdr.e_phnum || elfhdr.e_phentsize) {
        printf(" e_entry              : %#lx\n", (unsigned long)elfhdr.e_entry);
        printf(" e_phoff              : %ld\n", (unsigned long)elfhdr.e_phoff);
        printf(" e_phentsize          : %d\n", elfhdr.e_phentsize);
        printf(" e_phnum              : %d\n", elfhdr.e_phnum);
    } else if(elfhdr.e_entry) {
        printf(" e_entry              : %#lx\n", (unsigned long)elfhdr.e_entry);
    }
    if(elfhdr.e_shoff || elfhdr.e_shnum || elfhdr.e_shentsize || elfhdr.e_shstrndx) {
        printf(" e_shoff              : %ld\n", (unsigned long)elfhdr.e_shoff);
        printf(" e_shentsize          : %d\n", elfhdr.e_shentsize);
        printf(" e_shnum              : %d\n", elfhdr.e_shnum);
    }
    if(elfhdr.e_shstrndx) {
        printf(" e_shstrndx           : %d\n", elfhdr.e_shstrndx);
    }
    if(elfhdr.e_phnum && elfhdr.e_phentsize >= sizeof(Elf32_Phdr)) {
        int				n;
        for(n = 0; n < elfhdr.e_phnum; n++) {
            Elf32_Phdr			phdr;

            fseek(fp, pos + elfhdr.e_phoff + n * elfhdr.e_phentsize, SEEK_SET);
            if(fread(&phdr, sizeof phdr, 1, fp) != 1) {
                error(1, (char *)("Unable to read phdr %d\n"), n);
                break;
            }
            printf(" segment %d\n", n);
            if(CROSSENDIAN(elfhdr.e_ident[EI_DATA] == ELFDATA2MSB)) {
                phdr.p_type = ENDIAN_RET32(phdr.p_type);
                phdr.p_offset = ENDIAN_RET32(phdr.p_offset);
                phdr.p_vaddr = ENDIAN_RET32(phdr.p_vaddr);
                phdr.p_paddr = ENDIAN_RET32(phdr.p_paddr);
                phdr.p_filesz = ENDIAN_RET32(phdr.p_filesz);
                phdr.p_memsz = ENDIAN_RET32(phdr.p_memsz);
                phdr.p_flags = ENDIAN_RET32(phdr.p_flags);
                phdr.p_align = ENDIAN_RET32(phdr.p_align);
            }
            printf("   p_type               : ");
            switch(phdr.p_type) {
#ifndef PT_COMPRESS
#define PT_COMPRESS		0x4000
#endif
            case PT_COMPRESS:
                printf("PT_COMPRESS");
                break;
#ifndef PT_SEGREL
#define PT_SEGREL		0x4001
#endif
            case PT_SEGREL:
                printf("PT_SEGREL");
                break;

            default:
                dsp_index(phdr_type, sizeof phdr_type / sizeof *phdr_type, phdr.p_type);
                break;
            }
            printf("\n");
            printf("   p_offset             : %ld\n", (unsigned long)phdr.p_offset);
            printf("   p_vaddr              : 0x%lX\n", (unsigned long)phdr.p_vaddr);
            printf("   p_paddr              : 0x%lX\n", (unsigned long)phdr.p_paddr);
            printf("   p_filesz             : %ld\n", (unsigned long)phdr.p_filesz);
            printf("   p_memsz              : %ld\n", (unsigned long)phdr.p_memsz);
            printf("   p_flags              : ");
            dsp_index(phdr_flags, sizeof phdr_flags / sizeof *phdr_flags, phdr.p_flags);
            printf("\n");
            printf("   p_align              : %ld\n", (unsigned long)phdr.p_align);
        }
    }
    printf("----------\n");
}

int Dump_HBCIFS::zero_ok (struct startup_header *shdr)
{
    return(shdr->zero[0] == 0 &&
            shdr->zero[1] == 0 &&
            shdr->zero[2] == 0 );
}

void Dump_HBCIFS::display_file(FILE *fp, int ipos, struct image_dirent::image_file *ent)
{
    if(check(ent->path) != 0) {
        return;
    }
    printf(" %8x %8x  %s", ipos + ent->offset, ent->size, ent->path);
    if (flags & FLAG_MD5) {
#ifdef _DISABLE_MD5_
        printf(" MD5 Not available");
#else
        unsigned char	md5_result[MD5_LENGTH];
        int				i;

        compute_md5(fp, ipos, ent, md5_result);
        printf(" ");
        for ( i = 0; i < MD5_LENGTH; i++ ) {
            printf("%02x", md5_result[i] );
        }
#endif
    }
    printf("\n");
    if(verbose) {
        display_attr(&ent->attr);
    }
    if(verbose > 1) {
        display_elf(fp, ipos + ent->offset, ent->size, basename(ent->path));
    }
}

void Dump_HBCIFS::compute_md5(FILE *fp, int ipos, struct image_dirent::image_file *ent, unsigned char *md5_result)
{
#ifndef _DISABLE_MD5_
    unsigned char buff[4096];
    int nbytes, n;
    MD5_CTX md5_ctx;

    if (fseek(fp, ipos + ent->offset, SEEK_SET) != 0) {
        error(0, "fseek on source file for MD5 calculation failed: %s\n", strerror(errno));
    }

    memset((unsigned char*)&md5_ctx, 0, sizeof(md5_ctx));
    MD5Init(&md5_ctx);

    nbytes = ent->size;

    while (nbytes > 0) {
        n = min(sizeof buff, nbytes);
        if(fread(buff, n, 1, fp) != 1) {
            error(0, "Error reading %d bytes for MD5 calculation: %s\n", n, strerror(errno));
        }

        MD5Update(&md5_ctx, buff, n);
        nbytes -= n;
    }

    MD5Final(md5_result, &md5_ctx);
#endif
}

// Calculate the sum of an array of 4byte numbers
int Dump_HBCIFS::rifs_checksum(void *ptr, long len)
{
    int			*data = (int *)ptr;
    int 		sum;
    unsigned	max;

    // The checksum may take a while for large images, so we want to poll the mini-driver
    max=len;//max = (lsp.mdriver.size > 0) ? mdriver_cksum_max : len;

    sum = 0;
    while(len > 0)
    {
        sum += *data++;
        len -= 4;
        /*mdriver_count += 4;
        if(mdriver_count >= max)
        {
            // Poll the mini-driver when we reach the limit
            mdriver_check();
            mdriver_count = 0;
        }*/
    }
    return(sum);
}


#ifdef __QNXNTO__
__SRCVERSION("dumpifs.c $Rev: 207305 $");
#endif
