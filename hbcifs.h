#ifndef HBCIFS_H
#define HBCIFS_H

#define HBCIFS_SIGNATURE		"hbcifs"

struct hbcifs_header {
    char signature[7];                          //Null terminated string
    char null;
    unsigned int decompresed_image_size;
    unsigned int compressed_image_size;
    unsigned int unknown01;
    unsigned short crc16_01;
    unsigned short crc16_02;
    char flags01;                               //If 02, then checksum exists
    char flags02;
    char compression01;                         //0x01 = LZO, 0x03 = UCL, 0x04 = UNKNOWN
    char compression02;
    unsigned int unknown02;
    unsigned int padding[8];
} ;

/* We keep the flags as chars so they are endian neutral */
#define HBCIFS_COMPRESS1_LZO                0x01
#define HBCIFS_COMPRESS1_UCL                0x03
#define HBCIFS_COMPRESS1_ZLIB               0x04    //A Guess at this point

#endif // HBCIFS_H
