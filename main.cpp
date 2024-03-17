#include <iostream>

#include "dump_hbcifs.h"

using namespace std;

void display_usage(void)
{
    cout << "\nHelp" << endl;
    cout << "Examples: \"./dump_hbcifs FILENAME.bin\" <-- List the files in FILENAME.bin" << endl;
    cout << "          \"./dump_hbcifs FILENAME.bin -d DIR -x\" <-- Extract all files in FILENAME.bin and put them in directory DIR" << endl << endl;

    cout << "	  [-mvxbzh -u FILE] [-f FILE] image_file_system_file [-d DIR]" << endl;
    cout << " -b       Extract to basenames of files" << endl;
    cout << " -u FILE  Save a copy of the uncompressed image as FILE" << endl;
    cout << " -v       Verbose" << endl;
    cout << " -x       Extract files" << endl;
    cout << " -m       Display MD5 Checksum" << endl;
    cout << " -f FILE  Extract named FILE" << endl;
    cout << " -d DIR   Extract files to directory DIR" << endl;
    cout << " -z       *** NOT RECOMMENDED ***" << endl;
    cout << "          Disable the zero check while searching for the startup header." << endl;
    cout << "          This option should be avoided as it makes the search for the" << endl;
    cout << "          startup header less reliable." << endl;
    cout << " -h       Display this help message" << endl;
}

int main(int argc, char *argv[])
{   
    int					c;
    char				*image;
    FILE				*fp = NULL;
    char				*dest_dir_path = NULL;
    struct              Dump_HBCIFS::extract_file *ef;

    Dump_HBCIFS *dump_hbcifs = new Dump_HBCIFS();

    cout << "\n\t\t\tDump QNX IFS, ImageFS, and HBCIFS\n" << endl;

    dump_hbcifs->progname = basename(argv[0]);
    while((c = getopt(argc, argv, ":f:d:mvxbu:z:h")) != -1)
    {
        switch(c)
        {

        case 'f':
            ef = (struct Dump_HBCIFS::extract_file *)(malloc((sizeof(*ef))));
            if ( ef == NULL ) {
                perror("recording path");
                break;
            }
            ef->path = strdup(optarg);
            if ( ef->path == NULL ) {
                perror("recording path");
                break;
            }
            ef->next = dump_hbcifs->extract_files;
            dump_hbcifs->extract_files = ef;
            dump_hbcifs->files_to_extract++;
            dump_hbcifs->files_left_to_extract++;
            dump_hbcifs->flags |= FLAG_EXTRACT;
            break;

        case 'd':
            dest_dir_path = strdup(optarg);
            dump_hbcifs->flags |= FLAG_EXTRACT;
            break;

        case 'm':
            dump_hbcifs->flags |= FLAG_MD5;
            break;

        case 'x':
            dump_hbcifs->flags |= FLAG_EXTRACT;
            break;

        case 'b':
            dump_hbcifs->flags |= FLAG_BASENAME;
            break;

        case 'u':
            dump_hbcifs->ucompress_file = optarg;
            break;

        case 'v':
            dump_hbcifs->verbose++;
            break;

        case 'z':
            dump_hbcifs->zero_check_enabled = 0;
            break;

        case '?':
        case 'h':
            display_usage();
            return(0);

        default:
            break;
        }
    }
    if(dump_hbcifs->flags & (FLAG_EXTRACT|FLAG_MD5)) {
        if(dump_hbcifs->verbose > 1) {
            dump_hbcifs->flags |= FLAG_DISPLAY;
        }
    } else {
        dump_hbcifs->flags |= FLAG_DISPLAY;
    }
    if(optind >= argc) {
        cout << "Missing image file system name" << endl;
        display_usage();
        return -1;
    }
    image = argv[optind++];

    if(optind < argc) {
        dump_hbcifs->check_files = &argv[optind];
    }
    if(strcmp(image, "-") != 0) {
        if(!(fp = fopen(image, "rb"))) {
            cout << "Unable to open file " << image << " - " << strerror(errno) << endl;
            return(-1);
        }
    } else if(isatty(fileno(stdin))) {
        cout << "Must have an image file" << endl;
        return(-1);
    } else {
        fp = stdin;
        image = (char *)"-- stdin --";
    }

    dump_hbcifs->scanfile(dest_dir_path, image, fp);

    fclose(fp);

    return 0;
}
