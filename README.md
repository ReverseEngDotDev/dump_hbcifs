# dump_hbcifs

Based on : https://github.com/askac/dumpifs , but modified to support IFS, ImageFS and HBCIFS (Harmon Becker Compressed IFS) files.

dump_hbcifs will loop through the firmware file looking for supported header types, and if found will process the sub-section. At the end of the sub-section, it will continue to process the firmware file until it reaches EOF.

This process is helpful for multi-section firmware files used in BMW (among other) firmware files, which store the pre-boot, boot, and root separately. If the flag to extract files (-x) is passed to the program along with a directory (-d DIR), it will extract all files across all sections to the same DIR.

## Dependencies

CMake links against LZO, UCL and OpenSSL (for MD5). If you install this on a typical user workstation, you will need the "-devel" package.

## dump_hbcifs help script
```
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
```
## Example on a BMW Firmware File

[Lines have been removed with "..." to shorten output unnecessary for understanding]

```
./dump_hbcifs 9283251A.0pb -d 9283251A/ -x

                        Dump QNX IFS, ImageFS, and HBCIFS

 ********** Process IFS at: 180004

 Image startup_size: 53512 (0xd108)
 Image stored_size: 5894240 (0x59f060)
 Compressed size: 5840724 (0x591f54)
LZO Decompress @0x18d10c
LZO Decompress rd=35312, wr=65536 @ 0x18d10c
...
LZO Decompress rd=3147, wr=65536 @ 0x71d6ba
LZO Decompress rd=3413, wr=13904 @ 0x71e307
Decompressed 5840314 bytes-> 13252176 bytes
usr/share/wave/PDCBeep.wav ... size 3604
etc/layermanager.cfg ... size 109
...
etc/termcap ... size 6749
etc/config/persistTraceCicMidNavRoot.bin ... size 5926
 Image startup_size: 53512 (0xd108)
 Image stored_size: 5894240 (0x59f060)
 Compressed size: 5840724 (0x591f54)

 ********** Continue search at: 71f064
 ********** Process HBCIFS at: 740004

LZO Decompress @0x740044
LZO Decompress rd=33587046, wr=66207872 @ 0x740044
Decompressed 33587046 bytes-> 66207872 bytes
usr/mme/db/mme_connect.sql ... size 856
usr/bin/inetd ... size 45056
...
usr/lib/terminfo/v/vt52 ... size 905

 ********** Continue search at: 2747faa
Nothing left to extract from 9283251A.0pb
```
