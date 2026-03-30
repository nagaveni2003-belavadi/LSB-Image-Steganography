#ifndef DECODE_H
#define DECODE_H

#include <stdio.h>
#include "types.h"

#define DECODE_MAX_SECRET_BUF_SIZE 1
#define DECODE_MAX_IMAGE_BUF_SIZE (DECODE_MAX_SECRET_BUF_SIZE * 8)
#define DECODE_MAX_FILE_SUFFIX 4
#define DECODE_MAGIC_STRING "#*"
#define DECODE_MAGIC_LEN 2
#define DECODE_BMP_HEADER_SIZE 54

typedef struct _DecodeInfo
{
    /* Source image info */
    char *src_image_fname;        
    FILE *fptr_src_image;         
    uint image_capacity;

    /* Output file info */
    char *output_name;            
    FILE *fptr_secret;           
    char extn_secret_file[DECODE_MAX_FILE_SUFFIX];
    char secret_data[DECODE_MAX_SECRET_BUF_SIZE];
    long size_secret_file;

    char *expected_fname;
} DecodeInfo;

Status read_and_validate_decode_args(char *argv[], DecodeInfo *dncInfo);

Status do_decoding(DecodeInfo *dncInfo);

Status skip_bmp_header(FILE *fptr_src_image);
unsigned char byte_from_lsb(const unsigned char *image_buffer);
int int_from_lsb(const unsigned char *image_buffer);
Status decode_magic_string(FILE *fptr_src_image);
Status decode_extn_size(FILE *fptr_src_image, int *extn_size);
Status decode_extn(FILE *fptr_src_image, char *extn, int extn_size);
Status decode_secret_file_size(FILE *fptr_src_image, long *secret_file_size);

#endif 
