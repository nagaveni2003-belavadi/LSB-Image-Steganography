#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "decode.h"
#include "types.h"

#define FINAL_OUT_MAX 256  // Max length for output filename

/* Extract 1 byte of data from 8 bytes of image (LSB method) */
unsigned char byte_from_lsb(const unsigned char *buf)
{
    unsigned char value = 0;
    for (int i = 0; i < 8; ++i)
    {
        unsigned char bit = buf[i] & 1;        // Get last bit of each byte
        value = (unsigned char)((value << 1) | bit); // Build one character
    }
    return value;
}

/* Extract 4 bytes (integer) from 32 bytes of image (LSB method) */
int int_from_lsb(const unsigned char *buf)
{
    int value = 0;
    for (int i = 0; i < 32; ++i)
    {
        unsigned char bit = buf[i] & 1;        // Get last bit of each byte
        value = (value << 1) | bit;            // Build integer bit by bit
    }
    return value;
}

/* Skip BMP header (first 54 bytes) */
Status skip_bmp_header(FILE *fptr_src)
{
    if (fptr_src == NULL) 
    {
        return e_failure;
    }
    if (fseek(fptr_src, DECODE_BMP_HEADER_SIZE, SEEK_SET) != 0) 
    {
        return e_failure;
    }
    return e_success;
}

/* Read and verify the magic string "#*" from image */
Status decode_magic_string(FILE *fptr_src)
{
    if (fptr_src == NULL) 
    {
        return e_failure;
    }

    unsigned char img_buf[DECODE_MAX_IMAGE_BUF_SIZE];
    for (int i = 0; i < DECODE_MAGIC_LEN; ++i)
    {
        size_t r = fread(img_buf, 1, DECODE_MAX_IMAGE_BUF_SIZE, fptr_src); // Read 8 bytes
        if (r != DECODE_MAX_IMAGE_BUF_SIZE) 
        {
            return e_failure;
        }

        unsigned char ch = byte_from_lsb(img_buf); // Decode one char
        if (ch != (unsigned char)DECODE_MAGIC_STRING[i]) 
        {
            return e_failure; // Mismatch
        }
    }
    return e_success;
}

/* Decode extension size (32 bits) */
Status decode_extn_size(FILE *fptr_src, int *out_extn_size)
{
    if (fptr_src == NULL || out_extn_size == NULL) 
    {
        return e_failure;
    }
    unsigned char img_buf[32];
    if (fread(img_buf, 1, 32, fptr_src) != 32) 
    {
        return e_failure;
    }
    *out_extn_size = int_from_lsb(img_buf); // Get extension size
    return e_success;
}

/* Decode the extension string (.txt / .c / .h / etc.) */
Status decode_extn(FILE *fptr_src, char *extn, int extn_size)
{
    if (fptr_src == NULL || extn == NULL) 
    {
        return e_failure;
    }
    if (extn_size <= 0 || extn_size >= (int)DECODE_MAX_FILE_SUFFIX) 
    {
        return e_failure;
    }

    unsigned char img_buf[DECODE_MAX_IMAGE_BUF_SIZE];
    for (int i = 0; i < extn_size; ++i)
    {
        if (fread(img_buf, 1, DECODE_MAX_IMAGE_BUF_SIZE, fptr_src) != DECODE_MAX_IMAGE_BUF_SIZE)
        {
            return e_failure;
        }
        extn[i] = (char)byte_from_lsb(img_buf); // Decode each extension char
    }
    extn[extn_size] = '\0'; // Null-terminate extension
    return e_success;
}

/* Decode size of secret file */
Status decode_secret_file_size(FILE *fptr_src, long *out_secret_size)
{
    if (fptr_src == NULL || out_secret_size == NULL) 
    {
        return e_failure;
    }
    unsigned char img_buf[32];
    if (fread(img_buf, 1, 32, fptr_src) != 32) 
    {
        return e_failure;
    }
    *out_secret_size = (long)int_from_lsb(img_buf); // Get secret size
    return e_success;
}

/* Decode the actual secret file data (char-by-char) */
static Status decode_secret_file_data_internal(FILE *fptr_src, FILE *fptr_out, long secret_size, FILE *f_expected, int *mismatch_flag)
{
    if (fptr_src == NULL || fptr_out == NULL) 
    {
        return e_failure;
    }
    if (secret_size < 0) 
    {
        return e_failure;
    }

    unsigned char img_buf[DECODE_MAX_IMAGE_BUF_SIZE];
    for (long i = 0; i < secret_size; ++i)
    {
        if (fread(img_buf, 1, DECODE_MAX_IMAGE_BUF_SIZE, fptr_src) != DECODE_MAX_IMAGE_BUF_SIZE)
        {
            return e_failure;
        }

        unsigned char ch = byte_from_lsb(img_buf); // Decode 1 character

        if (fwrite(&ch, 1, 1, fptr_out) != 1)      // Write to output file
        {
            return e_failure;
        }

        /* Optional: check if output matches expected file */
        if (f_expected != NULL && mismatch_flag != NULL && *mismatch_flag == 0)
        {
            unsigned char exp;
            size_t r = fread(&exp, 1, 1, f_expected);
            if (r == 1)
            {
                if (exp != ch) *mismatch_flag = 1;
            }
            else
            {
                *mismatch_flag = 1;
            }
        }
    }
    return e_success;
}

/* Check if BMP filename is valid (ends with ".bmp") */
static int valid_bmp_name(const char *name)
{
    if (name == NULL) 
    {
        return 0;
    }
    const char *dot = strrchr(name, '.');
    if (dot == NULL) 
    {
        return 0;
    }
    if (strcmp(dot, ".bmp") != 0) 
    {
        return 0;
    }
    return 1;
}

/* Check output name validity (0 or 1 dot only) */
static int valid_output_name(const char *name)
{
    if (name == NULL) 
    {
        return 1;
    }
    int dot_count = 0;
    for (const char *p = name; *p; ++p) 
    {
        if (*p == '.') ++dot_count;
    }
    return (dot_count <= 1);
}

/* Read & validate command line decode arguments */
Status read_and_validate_decode_args(char *argv[], DecodeInfo *dncInfo)
{
    if (argv == NULL || dncInfo == NULL) return e_failure;

    if (argv[1] == NULL) 
    {
        return e_failure;
    }
    if (strcmp(argv[1], "-d") != 0) 
    {
        return e_failure;
    }

    if (argv[2] == NULL) 
    {
        return e_failure;
    }
    if (!valid_bmp_name(argv[2])) 
    {
        return e_failure;
    }

    if (valid_output_name(argv[3]) != 1) 
    {
        return e_failure;
    }

    /* Initialize structure members */
    dncInfo->src_image_fname = argv[2];
    dncInfo->fptr_src_image = NULL;
    dncInfo->image_capacity = 0;
    dncInfo->output_name = NULL;
    dncInfo->fptr_secret = NULL;
    dncInfo->extn_secret_file[0] = '\0';
    dncInfo->secret_data[0] = '\0';
    dncInfo->size_secret_file = 0;
    dncInfo->expected_fname = NULL;

    if (argv[3] != NULL) 
    {
        dncInfo->output_name = argv[3]; // Store output name
    }
    if (argv[4] != NULL) 
    {
        dncInfo->expected_fname = argv[4]; // Optional expected file
    }

    return e_success;
}

/* Perform actual decoding process */
Status do_decoding(DecodeInfo *dncInfo)
{
    if (dncInfo == NULL || dncInfo->src_image_fname == NULL) 
    {
        printf("ERROR: Internal: invalid decode info.\n");
        return e_failure;
    }

    /* Step 1: open source image file */
    dncInfo->fptr_src_image = fopen(dncInfo->src_image_fname, "r");
    if (dncInfo->fptr_src_image == NULL)
    {
        printf("ERROR: Unable to open source image: %s\n", dncInfo->src_image_fname);
        return e_failure;
    }

    /* Step 2: skip BMP header */
    if (skip_bmp_header(dncInfo->fptr_src_image) == e_failure)
    {
        printf("ERROR: Could not skip BMP header.\n");
        fclose(dncInfo->fptr_src_image);
        return e_failure;
    }

    /* Step 3: decode and verify magic string */
    if (decode_magic_string(dncInfo->fptr_src_image) == e_failure)
    {
        printf("ERROR: Magic string missing or mismatched.\n");
        fclose(dncInfo->fptr_src_image);
        return e_failure;
    }

    /* Step 4: decode extension size */
    int extn_size = 0;
    if (decode_extn_size(dncInfo->fptr_src_image, &extn_size) == e_failure)
    {
        printf("ERROR: Could not read extension size.\n");
        fclose(dncInfo->fptr_src_image);
        return e_failure;
    }

    if (extn_size <= 0 || extn_size >= (int)DECODE_MAX_FILE_SUFFIX)
    {
        printf("ERROR: Invalid extension size read: %d\n", extn_size);
        fclose(dncInfo->fptr_src_image);
        return e_failure;
    }

    /* Step 5: decode extension */
    if (decode_extn(dncInfo->fptr_src_image, dncInfo->extn_secret_file, extn_size) == e_failure)
    {
        printf("ERROR: Could not read secret file extension.\n");
        fclose(dncInfo->fptr_src_image);
        return e_failure;
    }

    /* Step 6: clean extension (remove dot, lowercase, replace .c -> .txt) */
    if (dncInfo->extn_secret_file[0] == '.')
    {
        for (int i = 0; dncInfo->extn_secret_file[i] != '\0'; ++i)
            dncInfo->extn_secret_file[i] = dncInfo->extn_secret_file[i + 1];
    }

    for (char *p = dncInfo->extn_secret_file; *p; ++p)
        *p = (char)tolower((unsigned char)*p);

    if (strcmp(dncInfo->extn_secret_file, "c") == 0)
    {
        strncpy(dncInfo->extn_secret_file, "txt", DECODE_MAX_FILE_SUFFIX - 1);
        dncInfo->extn_secret_file[DECODE_MAX_FILE_SUFFIX - 1] = '\0';
    }

    /* Step 7: build final output file name */
    char final_out[FINAL_OUT_MAX];
    final_out[0] = '\0';

    if (dncInfo->output_name != NULL)
    {
        char *dot = strrchr(dncInfo->output_name, '.');
        if (dot != NULL)
        {
            size_t base_len = (size_t)(dot - dncInfo->output_name);
            if (base_len >= sizeof(final_out)) 
                base_len = sizeof(final_out) - 1;
            strncpy(final_out, dncInfo->output_name, base_len);
            final_out[base_len] = '\0';

            size_t rem = sizeof(final_out) - strlen(final_out) - 1;
            if (rem > 0) strncat(final_out, ".", rem);
            rem = sizeof(final_out) - strlen(final_out) - 1;
            if (rem > 0) strncat(final_out, dncInfo->extn_secret_file, rem);
        }
        else
        {
            strncpy(final_out, dncInfo->output_name, sizeof(final_out) - 1);
            final_out[sizeof(final_out) - 1] = '\0';
            size_t rem = sizeof(final_out) - strlen(final_out) - 1;
            if (rem > 0) strncat(final_out, ".", rem);
            rem = sizeof(final_out) - strlen(final_out) - 1;
            if (rem > 0) strncat(final_out, dncInfo->extn_secret_file, rem);
        }
    }
    else
    {
        strncpy(final_out, "decoded_secret", sizeof(final_out) - 1);
        final_out[sizeof(final_out) - 1] = '\0';
        size_t rem = sizeof(final_out) - strlen(final_out) - 1;
        if (rem > 0) strncat(final_out, ".", rem);
        rem = sizeof(final_out) - strlen(final_out) - 1;
        if (rem > 0) strncat(final_out, dncInfo->extn_secret_file, rem);
    }

    /* Step 8: open output file */
    dncInfo->fptr_secret = fopen(final_out, "w");
    if (dncInfo->fptr_secret == NULL)
    {
        printf("ERROR: Could not create output file: %s\n", final_out);
        fclose(dncInfo->fptr_src_image);
        return e_failure;
    }

    /* Step 9: optional expected file open (for checking) */
    FILE *f_expected = NULL;
    int expected_opened = 0;
    if (dncInfo->expected_fname != NULL)
    {
        f_expected = fopen(dncInfo->expected_fname, "r");
        if (f_expected != NULL)
        {
            expected_opened = 1;   // only validate if successfully opened
        }
    }


    /* Step 10: decode secret file size */
    if (decode_secret_file_size(dncInfo->fptr_src_image, &dncInfo->size_secret_file) == e_failure)
    {
        printf("ERROR: Failed to read secret file size.\n");
        fclose(dncInfo->fptr_src_image);
        fclose(dncInfo->fptr_secret);
        if (f_expected) fclose(f_expected);
        return e_failure;
    }

    if (dncInfo->size_secret_file < 0)
    {
        printf("ERROR: Invalid secret file size: %ld\n", dncInfo->size_secret_file);
        fclose(dncInfo->fptr_src_image);
        fclose(dncInfo->fptr_secret);
        if (f_expected) fclose(f_expected);
        return e_failure;
    }

    /* Step 11: decode secret data */
    int mismatch_flag = 0;
    if (decode_secret_file_data_internal(dncInfo->fptr_src_image, dncInfo->fptr_secret, dncInfo->size_secret_file, f_expected, &mismatch_flag) == e_failure)
    {
        printf("ERROR: Failed during decoding secret data (I/O or corrupted image).\n");
        fclose(dncInfo->fptr_src_image);
        fclose(dncInfo->fptr_secret);
        if (f_expected) fclose(f_expected);
        return e_failure;
    }

    /* Step 12: close all files */
    fclose(dncInfo->fptr_secret);
    fclose(dncInfo->fptr_src_image);
    if (f_expected) fclose(f_expected);

    /* Step 13: report results */
    printf("Decoding finished. Output: %s\n", final_out);

    return e_success;
}
