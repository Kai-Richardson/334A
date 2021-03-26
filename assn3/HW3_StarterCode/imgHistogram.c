/* color histogram extractor
* convert jpg imgs to png
* mogrify -format png *.jpg
* to compile: gcc -o hist imgHistogram.c libpng16.a libz.a -lm 
*/

#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "png.h"
 
int main(int argv, char* argc[]){
    if (argv != 2){
        perror("usage: <executable> <input file>");
        exit(EXIT_FAILURE);
    }
    char* fn_in = argc[1];
    unsigned char* img;
    png_structp png_ptr; // pointer to png struct
    png_infop info_ptr;  //poiner to png header struct
    png_uint_32 width, height, bit_depth, color_type, interlace_type, number_of_passes;
    png_bytep * row_pointers; // pointer to image payload
    int hist[256*3] = { 0 };  // histogram array
    char header[8];           // to read magic number of 8 bytes

    /* open file and test for it being a png */
    FILE *fp = fopen(fn_in, "rb");
    if (!fp)
        perror("fopen");
    fread(header, 1, 8, fp); // read magic number
    if (png_sig_cmp(header, 0, 8))
        perror("not a PNG file");

    /* initialize png structs */
    if((png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL))==0)
        perror("png_create_read_struct failed");

    if((info_ptr = png_create_info_struct(png_ptr))==0)
        perror("png_create_info_struct failed");

    if (setjmp(png_jmpbuf(png_ptr)))
        perror("init_io failed");

    png_init_io(png_ptr, fp);
    png_set_sig_bytes(png_ptr, 8);
    //load structs
    png_read_info(png_ptr, info_ptr);
    //get local variable copies from structs
    width = png_get_image_width(png_ptr, info_ptr);
    height = png_get_image_height(png_ptr, info_ptr);
    color_type = png_get_color_type(png_ptr, info_ptr);
    bit_depth = png_get_bit_depth(png_ptr, info_ptr);

    //for inflating
    number_of_passes = png_set_interlace_handling(png_ptr);
    png_read_update_info(png_ptr, info_ptr);

    // read file
    if (setjmp(png_jmpbuf(png_ptr)))
        perror("png_jmpbuf: error read_image");
    // allocated array of row pointers
    row_pointers = (png_bytep*) malloc(sizeof(png_bytep) * height);
    // allocated each row to read data into
    for (int y=0; y<height; y++)
            row_pointers[y] = (png_byte*) malloc(png_get_rowbytes(png_ptr,info_ptr));
    // read image into the 2D array
    png_read_image(png_ptr, row_pointers);
    // done reading so close file
    fclose(fp);
    //calculate color histogram counts as long as the file format has [][][] of 0-255
    if (png_get_color_type(png_ptr, info_ptr) != PNG_COLOR_TYPE_RGB)
        perror("must be a RGB file");
    for (int y=0; y<height; y++) {
        png_byte* row = row_pointers[y];
        for (int x=0; x<width; x++) {
            png_byte* ptr = &(row[x*3]);

            hist[(int)ptr[0]]++;
            hist[(int)ptr[1]+256]++;
            hist[(int)ptr[2]+256+256]++;
        }
    }
    //print histogram
    for(int i=0; i<256*3; i++){
        printf("%d:%d\t%f\n",i, hist[i], hist[i]/(double)(width*height*3));
    }
    //printf("\ntotal %d\n",width*height*3);

    //memory cleanup
    for (int y=0; y<height; y++)
        free(row_pointers[y]);
    free(row_pointers);
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

    return 0;
}