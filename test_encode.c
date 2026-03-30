/* Project Documentation
Name: Nagaveni Belavadi
Date: 20-1-2026
Project: Image Steganography using BMP Files
Description:
The Image Steganography project is a C program that allows the user to hide (encode)
a secret text file inside an image (BMP format) and retrieve (decode) it later without
visibly altering the image.

The project demonstrates file handling, bit manipulation, and validation in C,
making it an excellent implementation of data hiding using the Least Significant Bit (LSB) technique.

It provides the following functionality:

1. Encoding (Hide Data)
   - Reads a 24-bit BMP image as input.
   - Embeds a secret file (e.g., .txt, .c, .h, .sh) into the image's pixel data.
   - Creates a new stego-image (.bmp) containing both the image and the hidden data.
   - Verifies available capacity before encoding.
   - Allows optional output filename; defaults to "default.bmp" if not given.

2. Decoding (Extract Data)
   - Reads the stego-image.
   - Validates using a predefined magic string "#*" to ensure it contains hidden data.
   - Extracts the hidden file extension, size, and data.
   - Reconstructs and writes the secret file to disk.
   - Automatically renames the output file to match its original extension.

3. Argument Validation
   Encoding:
     • ./a.out -e <source.bmp> <secret.txt> [output.bmp]
       - Ensures proper order and extensions.
       - Rejects invalid argument combinations.

   Decoding:
     • ./a.out -d <stego.bmp> [output_name]
       - Ensures .bmp extension for input.
       - Optionally names output file; extension is derived automatically.

4. Error Handling
   - Handles missing files, invalid formats, and insufficient capacity.
   - Prints clear, beginner-friendly messages instead of system errors.

5. Technical Details
   - Uses Least Significant Bit (LSB) technique for encoding data bits.
   - Works only with uncompressed 24-bit BMP images.
   - The first 54 bytes (header) are preserved during encoding.
   - Ensures data integrity during both encoding and decoding.

Example (Encoding):
   Input:  beautiful.bmp (image), secret.txt (data)
   Command: ./a.out -e beautiful.bmp secret.txt stego.bmp
   Output:  stego.bmp created successfully

Example (Decoding):
   Input:  stego.bmp (stego image)
   Command: ./a.out -d stego.bmp
   Output: decoded_secret.txt extracted successfully

Sample Output:
   width = 1024
   height = 768
   INFO: Encoding completed successfully
   INFO: Decoding finished. Output: decoded_secret.txt

------------------------------------------------------------
Modules:
   1. encode.c / encode.h  – Handles encoding (writing secret into image)
   2. decode.c / decode.h  – Handles decoding (extracting secret from image)
   3. types.h              – Contains user-defined types and enums
   4. test_encode.c        – Main driver program with argument validation

------------------------------------------------------------
Learning Outcomes:
   • Practical understanding of File I/O and bitwise operations in C.
   • Understanding of BMP file structure and metadata.
   • Implementation of data validation and structured programming.
   • Gained experience with modular code organization and documentation.

End of Documentation
*/

#include <stdio.h>
#include <string.h>
#include "encode.h"
#include "decode.h"
#include "types.h"

/* Check operation*/
OperationType check_operation_type(char *argv[])
{
    if (argv == NULL || argv[1] == NULL)
        return e_unsupported;

    if(strcmp(argv[1], "-e") == 0)
    {
        return e_encode;
    }
    else if(strcmp(argv[1], "-d") == 0)
    {
        return e_decode;
    }
    else
    {
        return e_unsupported;
    }
}

int main(int argc, char *argv[]) /* Array of pointer */
{
    EncodeInfo encInfo;
    memset(&encInfo, 0, sizeof(encInfo));

    /* basic argument check */
    if (argc < 2)
    {
        printf("Error: Not enough arguments\n");
        printf("Usage: %s -e <source.bmp> <secret_file> [stego.bmp]\n", argv[0]);
        return 1;
    }

    OperationType ret = check_operation_type(argv);

    if (ret == e_encode)
    {
        /* encode path */
        if (argc >= 4)
        {
            /* validate encode args */
            if (read_and_validate_encode_args(argv, &encInfo) == e_failure)
            {
                printf("Error: Invalid encoding arguments\n");
                return 1;
            }

            /* do encoding */
            if (do_encoding(&encInfo) == e_failure)
            {
                printf("Error: Encoding failed\n");
                return 1;
            }

            printf("Encoding completed successfully\n");
        }
        else
        {
            /* Error message */
            printf("Error: Not enough arguments for encoding\n");
            return 1;
        }
    }
    else if (ret == e_decode)
    {
        /* decode path */
        DecodeInfo decInfo;
        memset(&decInfo, 0, sizeof(decInfo));

        /* validate decode args (this will also capture optional argv[3] and argv[4]) */
        if (read_and_validate_decode_args(argv, &decInfo) == e_failure)
        {
            printf("Error: Invalid decode arguments\n");
            printf("Usage: %s -d <source.bmp> [output_base] [expected_file]\n", argv[0]);
            return 1;
        }

        /* do decoding */
        if (do_decoding(&decInfo) == e_failure)
        {
            printf("Error: Decoding failed\n");
            return 1;
        }

        /* success message already printed by do_decoding */
    }
    else /* e_unsupported */
    {
        printf("Error: Unsupported operation. Use -e for encode or -d for decode\n");
        return 1;
    }

    return 0;
}
