# LSB Image Steganography

## Overview
The LSB (Least Significant Bit) Image Steganography project is a command-line application developed in C that enables secure hiding and extraction of secret data within digital images. The project utilizes the Least Significant Bit technique to embed information in image pixels without causing noticeable changes to the original image.

This project demonstrates key concepts of data security, file handling, and low-level data manipulation.

## Features
- Encode (hide) secret message into an image  
- Decode (extract) hidden message from an image  
- Supports BMP image format  
- Maintains minimal visual distortion in the image  
- File-based input and output operations  

## Technologies Used
- Programming Language: C  
- Concepts:  
  - File Handling  
  - Bit Manipulation  
  - Structures  
  - Modular Programming  
  - Binary Data Processing  

## Project Structure
LSB-Image-Steganography/  
│── encode.c  
│── decode.c  
│── common.h  
│── encode.h  
│── decode.h  
│── main.c  
│── Makefile (optional)  
│── sample.bmp  
│── secret.txt  

## How to Run
Compile the program:
gcc main.c encode.c decode.c -o steganography  

Run Encoding:
./steganography -e sample.bmp secret.txt output.bmp  

Run Decoding:
./steganography -d output.bmp  

## Sample Functionality
The application provides options to encode a secret message into an image and decode it back when required, ensuring data confidentiality using steganographic techniques.

## Learning Outcomes
- Understanding of image-based data hiding techniques  
- Practical experience with bit-level operations  
- Implementation of file handling for binary data  
- Exposure to basic cybersecurity concepts  

## Future Enhancements
- Support for multiple image formats (PNG, JPEG)  
- Encryption of secret data before embedding  
- GUI-based interface  
- Improved encoding capacity and efficiency  

## Author
Nagaveni Belavadi  

## License
This project is intended for educational and learning purposes. 
