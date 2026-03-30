#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "encode.h"
#include "types.h"



/* Function Definitions */

/* Get image size
 * Input: Image file ptr
 * Output: width * height * bytes per pixel (3 in our case)
 * Description: In BMP Image, width is stored in offset 18,
 * and height after that. size is 4 bytes
 */
uint get_image_size_for_bmp(FILE *fptr_image)
{
    if(fptr_image == NULL)
    {
        printf("Error: Image file pointer is NULL\n");
        return 0;
    }

    uint width = 0, height = 0;
    // Seek to 18th byte
    fseek(fptr_image, 18, SEEK_SET);

    // Read the width (an int)
    if(fread(&width, sizeof(uint), 1, fptr_image) != 1)
    {
        printf("Error: Unable to read image width\n");
        return 0;
    }
    printf("width = %u\n",width);

    // Read the height (an int)
    if(fread(&height, sizeof(uint), 1, fptr_image) != 1)
    {
        printf("Error: Unable to read image height\n");
        return 0;
    }
    printf("height = %u\n",height);

    // Return image capacity
    return width * height * 3;
}

/* 
 * Get File pointers for i/p and o/p files
 * Inputs: Src Image file, Secret file and
 * Stego Image file
 * Output: FILE pointer for above files
 * Return Value: e_success or e_failure, on file errors
 */
Status open_files(EncodeInfo *encInfo)
{
    if(encInfo == NULL)
    {
        return e_failure;
    }
    // Src Image file
    encInfo->fptr_src_image = fopen(encInfo->src_image_fname, "r");
    // Do Error handling
    if (encInfo->fptr_src_image == NULL)
    {
    	perror("fopen");
    	fprintf(stderr, "ERROR: Unable to open file %s\n", encInfo->src_image_fname);

    	return e_failure;
    }

    // Secret file
    encInfo->fptr_secret = fopen(encInfo->secret_fname, "r");
    // Do Error handling
    if (encInfo->fptr_secret == NULL)
    {
    	perror("fopen");
    	fprintf(stderr, "ERROR: Unable to open file %s\n", encInfo->secret_fname);
        fclose(encInfo -> fptr_src_image);
    	return e_failure;
    }

    // Stego Image file
    encInfo->fptr_stego_image = fopen(encInfo->stego_image_fname, "w");
    // Do Error handling
    if (encInfo->fptr_stego_image == NULL)
    {
    	perror("fopen");
    	fprintf(stderr, "ERROR: Unable to open file %s\n", encInfo->stego_image_fname);
        fclose(encInfo->fptr_src_image);
        fclose(encInfo->fptr_secret);
    	return e_failure;
    }

    // No failure return e_success
    return e_success;
}

/* Read and validate Encode args from argv
 *
 * Validation rules enforced (matches your list):
 *  - argv[2] must exist and must end exactly with ".bmp" (source image)
 *  - argv[3] must exist and must have an extension (one of .txt, .c, .sh, .h)
 *  - argv[3] must not begin with a dot (hidden/invalid)
 *  - argv[4] optional : if present must end exactly with ".bmp" (stego image)
 *  - if argv[4] not present, default stego name used ("default.bmp")
 *  - extracts secret extension into encInfo->extn_secret_file (safe copy)
 */
Status read_and_validate_encode_args(char *argv[], EncodeInfo *encInfo)
{
    if(argv == NULL || encInfo == NULL)
    {
        return e_failure;
    }

    /* Need source image at argv[2] */
    if(argv[2] == NULL)
    {
        return e_failure;
    }
    /* Check source image ends with ".bmp" exactly */
    {
        const char *dot = strrchr(argv[2], '.');
        if (dot == NULL) return e_failure;
        if (strcmp(dot, ".bmp") != 0) return e_failure;
    }
    encInfo -> src_image_fname = argv[2];

    /* Need secret file at argv[3] */
    if(argv[3] == NULL)
    {
        return e_failure;
    }
    /* secret name must not start with '.' and must have a valid extension */
    if (argv[3][0] == '.')
    {
        return e_failure;
    }

    /* Extract extension of secret and validate it's one of allowed types */
    {
        const char *s_dot = strrchr(argv[3], '.');
        if (s_dot == NULL) return e_failure; /* no extension -> fail */

        const char *ext = s_dot; /* includes leading '.' */
        /* Compare exact extensions */
        if (strcmp(ext, ".txt") != 0 && strcmp(ext, ".c") != 0 &&
            strcmp(ext, ".sh") != 0 && strcmp(ext, ".h") != 0)
        {
            return e_failure;
        }
    }
    encInfo -> secret_fname = argv[3];

    /* argv[4] optional: stego image filename */
    encInfo->stego_image_fname = NULL;
    if (argv[4] == NULL)
    {
        encInfo->stego_image_fname = "default.bmp";
    }
    else
    {
        /* must end exactly with ".bmp" */
        const char *dot4 = strrchr(argv[4], '.');
        if (dot4 == NULL) 
        {
            return e_failure;
        }
        if (strcmp(dot4, ".bmp") != 0) 
        {
            return e_failure;
        }
        encInfo->stego_image_fname = argv[4];
    }

    /* extract and store extension of secret file (without leading dot) */
    {
        const char *dot = strrchr(encInfo->secret_fname, '.');
        if (dot != NULL)
        {
            const char *ext_no_dot = dot + 1;
            size_t len = strlen(ext_no_dot);
            if (len >= MAX_FILE_SUFFIX) /* too long */
            {
                return e_failure;
            }
            /* safe copy, ensure null termination */
            strncpy(encInfo->extn_secret_file, ext_no_dot, MAX_FILE_SUFFIX - 1);
            encInfo->extn_secret_file[MAX_FILE_SUFFIX - 1] = '\0';
        }
        else
        {
            encInfo->extn_secret_file[0] = '\0';
        }
    }

    return e_success;
}

/* check capacity */
Status check_capacity(EncodeInfo *encInfo)
{
    //copy_bmp_header -> 54 byte
    //encode_magic_string -> 16 byte
    //extn_size -> 32 byte
    //extn -> (2 * 8) or (3 * 6) or (4 * 8)
    //file_size -> 32 byte
    //secret_data -> (24 * 8)
    if (encInfo == NULL)
    {
        return e_failure;
    }

    //Get the image capacity (width * height * 3)
    encInfo->image_capacity = get_image_size_for_bmp(encInfo->fptr_src_image);
    if (encInfo->image_capacity == 0)
    {
        return e_failure;
    }

    //Get the size of the secret file (inline logic from get_file_size)
    if (encInfo->fptr_secret == NULL)
    {
        printf("Error: Secret file pointer is NULL\n");
        return e_failure;
    }

    long current_pos = ftell(encInfo->fptr_secret); // Save current position
    if (current_pos < 0)
    {
        current_pos = 0;
    }

    //Move to end to find total size
    if (fseek(encInfo->fptr_secret, 0, SEEK_END) != 0)
    {
        printf("Error: Unable to seek in secret file\n");
        return e_failure;
    }

    long file_size = ftell(encInfo->fptr_secret);
    if (file_size < 0)
    {
        printf("Error: Unable to determine secret file size\n");
        return e_failure;
    }

    //Restore the previous position
    fseek(encInfo->fptr_secret, current_pos, SEEK_SET);

    encInfo->size_secret_file = (long)file_size;

    if (encInfo->size_secret_file == 0)
    {
        printf("Error: Secret file is empty\n");
        return e_failure;
    }

    //Calculate how many bits we need to store everything
    int extn_len = strlen(encInfo->extn_secret_file);
    long required_size = (MAGIC_len * 8) + 32 + (extn_len * 8) + 32 + (encInfo->size_secret_file * 8);

    //Check if image can hold the secret
    if (required_size > encInfo->image_capacity)
    {
        printf("Error: Image capacity (%u) too small for secret file (%ld bytes)\n", encInfo->image_capacity, encInfo->size_secret_file);
        return e_failure;
    }

    return e_success;
}


/* Copy bmp image header */
Status copy_bmp_header(FILE *fptr_src_image, FILE *fptr_dest_image)
{
    //Declare the char array with size is 54
    //Rewind the src file
    //Read the 54 bytes of data from sr file
    //Write the 54 bytes of data in dest file
    
    char header[BMP_HEADER_SIZE];

    rewind(fptr_src_image);
    if (fread(header, 1, BMP_HEADER_SIZE, fptr_src_image) != BMP_HEADER_SIZE)
    {
        return e_failure;
    }

    if (fwrite(header, 1, BMP_HEADER_SIZE, fptr_dest_image) != BMP_HEADER_SIZE)
    {
        return e_failure;
    }

    return e_success;
}

/* Store Magic String */
Status encode_magic_string(const char *magic_string, EncodeInfo *encInfo)
{
    //Declare the arr with size 8
    //Run the 8 byte of data from src file
        //Call the encode_byte_to_lsb(magic_string[i],arr(same name declared array))
        //write the 8 byte of data in dest file
    unsigned char image_buffer[MAX_IMAGE_BUF_SIZE];
    int i;

    for (i = 0; i < (int)strlen(magic_string); i++)
    {
        if (fread(image_buffer, 1, MAX_IMAGE_BUF_SIZE, encInfo->fptr_src_image) != MAX_IMAGE_BUF_SIZE)
        {
            return e_failure;
        }

        if (encode_byte_to_lsb((unsigned char)magic_string[i], image_buffer) != e_success)
        {
            return e_failure;
        }

        if (fwrite(image_buffer, 1, MAX_IMAGE_BUF_SIZE, encInfo->fptr_stego_image) != MAX_IMAGE_BUF_SIZE)
        {
            return e_failure;
        }
    }
    return e_success;
}

//Read the extn size
Status encode_secret_extn_file_size(int size, EncodeInfo *encInfo)
{
    //Declare the arr with size 32
    //Read the 32 byte of data from src file
    //Call encode_int_to_lsb(file_extn[i], arr);
    //Write the 8 bytes of data in dest file
    unsigned char image_buffer[32];

    if (fread(image_buffer, 1, 32, encInfo->fptr_src_image) != 32)
    {
        return e_failure;
    }

    if (encode_int_to_lsb(size, image_buffer) != e_success)
    {
        return e_failure;
    }

    if (fwrite(image_buffer, 1, 32, encInfo->fptr_stego_image) != 32)
    {
        return e_failure;
    }

    return e_success;
}

/* Encode secret file extn */
Status encode_secret_file_extn(const char *file_extn, EncodeInfo *encInfo)
{
    unsigned char image_buffer[MAX_IMAGE_BUF_SIZE];
    int i;

    for (i = 0; i < (int)strlen(file_extn); i++)
    {
        if (fread(image_buffer, 1, MAX_IMAGE_BUF_SIZE, encInfo->fptr_src_image) != MAX_IMAGE_BUF_SIZE)
        {
            return e_failure;
        }

        if (encode_byte_to_lsb((unsigned char)file_extn[i], image_buffer) != e_success)
        {
            return e_failure;
        }

        if (fwrite(image_buffer, 1, MAX_IMAGE_BUF_SIZE, encInfo->fptr_stego_image) != MAX_IMAGE_BUF_SIZE)
        {
            return e_failure;
        }
    }

    return e_success;
}

/* Encode secret file size */
Status encode_secret_file_size(long file_size, EncodeInfo *encInfo)
{
    //Declare the arr with size 32
    //Read the 32 byte of data from src file
    //Call the encode_int_t0_lsb(file_size),arr)
    //  Wrute the 32 byte data in dest file
    unsigned char image_buffer[32];

    if (fread(image_buffer, 1, 32, encInfo->fptr_src_image) != 32)
    {
        return e_failure;
    }

    if (encode_int_to_lsb((int)file_size, image_buffer) != e_success)
    {
        return e_failure;
    }

    if (fwrite(image_buffer, 1, 32, encInfo->fptr_stego_image) != 32)
    {
        return e_failure;
    }

    return e_success;
}

/* Encode secret file data*/
Status encode_secret_file_data(EncodeInfo *encInfo)
{
    //Rewinf for fptr_secret
    //Declare the arr with size 8
        //run the loop encInfo -> size_secret_file
        //fread(encInfo -> secret_data, 1, 1, encInfo -> fptr_secret)
        //Read the 8 byte of data from src file
        //Call the encode_byte_to_lsb(encInfo -> secret_data[0],arr)
        //Write the 8 byte of data in dest file
    unsigned char image_buffer[MAX_IMAGE_BUF_SIZE];
    unsigned char data;
    long i;

    rewind(encInfo->fptr_secret);

    for (i = 0; i < encInfo->size_secret_file; i++)
    {
        if (fread(&data, 1, 1, encInfo->fptr_secret) != 1)
        {
            return e_failure;
        }

        if (fread(image_buffer, 1, MAX_IMAGE_BUF_SIZE, encInfo->fptr_src_image) != MAX_IMAGE_BUF_SIZE)
        {
            return e_failure;
        }

        if (encode_byte_to_lsb(data, image_buffer) != e_success)
        {
            return e_failure;
        }

        if (fwrite(image_buffer, 1, MAX_IMAGE_BUF_SIZE, encInfo->fptr_stego_image) != MAX_IMAGE_BUF_SIZE)
        {
            return e_failure;
        }
    }

    return e_success;
}

/* Encode function, which does the real encoding */
Status encode_int_to_lsb(int size, unsigned char *image_buffer)//32 byte //same as byte to lsb but that is for char but here for int
{
    int i;
    for (i = 31; i >= 0; i--)
    {
        image_buffer[31 - i] = (image_buffer[31 - i] & 0xFE) | ((size >> i) & 1);
    }
    return e_success;
}

/* Encode a byte into LSB of image data array */
Status encode_byte_to_lsb(unsigned char data, unsigned char *image_buffer)//8 byte
{    
    unsigned char bit_data;//(becz it either 0 or 1 in msb bit)                                          
    int i;
    for(i = 7; i >= 0; i--)//(this is for char) //for int i=32
    {
        bit_data = (unsigned char)((data >> i) & 1);
        /* place MSB first into image_buffer[0] ... image_buffer[7] */
        image_buffer[7 - i] = (image_buffer[7 - i] & 0xFE) | bit_data;
    }
    return e_success;
}

Status do_encoding(EncodeInfo *encInfo)
{
    //call the open_files(enInfo);
        //e_failure
            //return e_failure

    //Call the check_capacity(encInfo);
        //e_failure
            //return e_failure

    //call the copy_bmp_header(encInfo -> fptr_src_image, encInfo -> fptr_stego_image);
        //e_failure
            //return e_failure

    //call the encode_magic_string(MAGIC_STRING, encInfo);
        //e_failure
            //return e_failure
    
    if (open_files(encInfo) == e_failure)
        return e_failure;

    if (check_capacity(encInfo) == e_failure)
    {
        /* close files opened by open_files */
        fclose(encInfo->fptr_src_image);
        fclose(encInfo->fptr_secret);
        fclose(encInfo->fptr_stego_image);
        return e_failure;
    }

    if (copy_bmp_header(encInfo->fptr_src_image, encInfo->fptr_stego_image) == e_failure)
    {
        fclose(encInfo->fptr_src_image);
        fclose(encInfo->fptr_secret);
        fclose(encInfo->fptr_stego_image);
        return e_failure;
    }

    if (encode_magic_string(MAGIC_STRING, encInfo) == e_failure)
    {
        fclose(encInfo->fptr_src_image);
        fclose(encInfo->fptr_secret);
        fclose(encInfo->fptr_stego_image);
        return e_failure;
    }

    int extn_len = strlen(encInfo->extn_secret_file);
    if (encode_secret_extn_file_size(extn_len, encInfo) == e_failure)
    {
        fclose(encInfo->fptr_src_image);
        fclose(encInfo->fptr_secret);
        fclose(encInfo->fptr_stego_image);
        return e_failure;
    }

    if (encode_secret_file_extn(encInfo->extn_secret_file, encInfo) == e_failure)
    {
        fclose(encInfo->fptr_src_image);
        fclose(encInfo->fptr_secret);
        fclose(encInfo->fptr_stego_image);
        return e_failure;
    }

    if (encode_secret_file_size(encInfo->size_secret_file, encInfo) == e_failure)
    {
        fclose(encInfo->fptr_src_image);
        fclose(encInfo->fptr_secret);
        fclose(encInfo->fptr_stego_image);
        return e_failure;
    }

    if (encode_secret_file_data(encInfo) == e_failure)
    {
        fclose(encInfo->fptr_src_image);
        fclose(encInfo->fptr_secret);
        fclose(encInfo->fptr_stego_image);
        return e_failure;
    }

    if (encInfo->fptr_src_image == NULL || encInfo->fptr_stego_image == NULL)
    {
        if (encInfo->fptr_src_image) fclose(encInfo->fptr_src_image);
        if (encInfo->fptr_secret) fclose(encInfo->fptr_secret);
        if (encInfo->fptr_stego_image) fclose(encInfo->fptr_stego_image);
        return e_failure;
    }

    {
        unsigned char buffer[4096];
        size_t bytes_read;
        /* copy the rest of source image to stego image */
        while ((bytes_read = fread(buffer, 1, sizeof(buffer), encInfo->fptr_src_image)) > 0)
        {
            if (fwrite(buffer, 1, bytes_read, encInfo->fptr_stego_image) != bytes_read)
            {
                /* write error */
                fclose(encInfo->fptr_src_image);
                fclose(encInfo->fptr_secret);
                fclose(encInfo->fptr_stego_image);
                return e_failure;
            }
        }

        if (ferror(encInfo->fptr_src_image))
        {
            /* read error */
            fclose(encInfo->fptr_src_image);
            fclose(encInfo->fptr_secret);
            fclose(encInfo->fptr_stego_image);
            return e_failure;
        }
    }

    fclose(encInfo->fptr_src_image);
    fclose(encInfo->fptr_secret);
    fclose(encInfo->fptr_stego_image);

    return e_success;
}
