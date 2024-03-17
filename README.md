# dump_hbcifs

Based on : https://github.com/askac/dumpifs , but modified to support IFS, ImageFS and HBCIFS (Harmon Becker Compressed IFS) files.

dump_hbcifs will loop through the firmware file looking for supported header types, and if found will process the sub-section. At the end of the sub-section, it will continue to process the firmware file until it reaches EOF.

This process is helpful for multi-section firmware files used in BMW (among other) firmware files, which store the pre-boot, boot, and root separately. If the flag to extract files (-x) is passed to the program along with a directory (-d DIR), it will extract all files across all sections to the same DIR.


Examples: "./dump_hbcifs FILENAME.bin" <-- List the files in FILENAME.bin
          "./dump_hbcifs FILENAME.bin -d DIR -x" <-- Extract all files in FILENAME.bin and put them in directory DIR

          [-mvxbzh -u FILE] [-f FILE] image_file_system_file [-d DIR]
 -b       Extract to basenames of files
 -u FILE  Save a copy of the uncompressed image as FILE
 -v       Verbose
 -x       Extract files
 -m       Display MD5 Checksum
 -f FILE  Extract named FILE
 -d DIR   Extract files to directory DIR
 -z       *** NOT RECOMMENDED ***
          Disable the zero check while searching for the startup header.
          This option should be avoided as it makes the search for the
          startup header less reliable.
 -h       Display this help message
