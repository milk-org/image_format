/** @file writeBMP.c
 */



#include "CommandLineInterface/CLIcore.h"
#include "COREMOD_memory/COREMOD_memory.h"
#include "COREMOD_arith/COREMOD_arith.h"


#define BMP_BIG_ENDIAN 0



#define BI_RGB 0
#define BM 19778
#define BMP_FALSE 0
#define BMP_TRUE 1


typedef struct
{
    uint16_t bfType;
    uint32_t bfSize;
    uint16_t bfReserved1;
    uint16_t bfReserved2;
    uint32_t bfOffBits;
} BITMAPFILEHEADER;

typedef struct
{
    uint32_t biSize;
    uint32_t biWidth;
    uint32_t biHeight;
    uint16_t biPlanes;
    uint16_t biBitCount;
    uint32_t biCompression;
    uint32_t biSizeImage;
    uint32_t biXPelsPerMeter;
    uint32_t biYPelsPerMeter;
    uint32_t biClrUsed;
    uint32_t biClrImportant;
} BITMAPINFOHEADER;


// ==========================================
// Forward declaration(s)
// ==========================================


errno_t image_writeBMP_auto(
    const char *__restrict__ IDnameR,
    const char *__restrict__ IDnameG,
    const char *__restrict__ IDnameB,
    const char *__restrict__ outname
);



// ==========================================
// Command line interface wrapper function(s)
// ==========================================

static errno_t image_writeBMP_cli()
{
    if(0
            + CLI_checkarg(1, 4)
            + CLI_checkarg(2, 4)
            + CLI_checkarg(3, 4)
            + CLI_checkarg(4, 3)
            == 0)
    {
        image_writeBMP_auto(
            data.cmdargtoken[1].val.string,
            data.cmdargtoken[2].val.string,
            data.cmdargtoken[3].val.string,
            data.cmdargtoken[4].val.string);
        return RETURN_SUCCESS;
    }
    else
    {
        return RETURN_FAILURE;
    }
}




// ==========================================
// Register CLI command(s)
// ==========================================

errno_t writeBMP_addCLIcmd()
{

    RegisterCLIcommand(
        "saveBMP",
        __FILE__,
        image_writeBMP_cli,
        "write RGB image as BMP - auto scaling",
        "<red image> <green image> <blue image> <output BMP file name>",
        "saveBMP imr img imb im.bmp",
        "int image_writeBMP_auto(const char *IDnameR, const char *IDnameG, const char *IDnameB, const char *outname)"
    );

    return RETURN_SUCCESS;
}













/* This function is for byte swapping on big endian systems */
uint16_t setUint16(uint16_t x)
{
    if(BMP_BIG_ENDIAN)
    {
        return (x & 0x00FF) << 8 | (x & 0xFF00) >> 8;
    }
    else
    {
        return x;
    }
}




/* This function is for byte swapping on big endian systems */
uint32_t setUint32(uint32_t x)
{
    if(BMP_BIG_ENDIAN)
    {
        return (x & 0x000000FF) << 24 | (x & 0x0000FF00) << 8 |
               (x & 0x00FF0000) >> 8 | (x & 0xFF000000) >> 24;
    }
    else
    {
        return x;
    }
}







/**
 * ## Purpose
 *
 * This function writes out a 24-bit Windows bitmap file that is readable by Microsoft Paint. \n
 * The image data is a 1D array of (r, g, b) triples, where individual (r, g, b) values can \n
 * each take on values between 0 and 255, inclusive.
 *
 * ## Arguments
 *
 * @param[in]
 * filename		char*
 * 				A string representing the filename that will be written
 *
 * @param[in]
 * width		uint32_t
 * 				The width, in pixels, of the bitmap
 *
 * @param[in[
 * height		uint32_t
 * 				The height, in pixels, of the bitmap
 *
 * @param[in]
 * image		image*
 * 				The image data, where each pixel is 3 unsigned chars (r, g, b)
 *
 * @note Written by Greg Slabaugh (slabaugh@ece.gatech.edu), 10/19/00
*/
static uint32_t write24BitBmpFile(
    const char    *__restrict__ filename,
    uint32_t        width,
    uint32_t        height,
    unsigned char *__restrict__ image
)
{
    BITMAPINFOHEADER bmpInfoHeader;
    BITMAPFILEHEADER bmpFileHeader;
    FILE *filep;
    uint32_t row, column;
    uint32_t extrabytes, bytesize;
    unsigned char *paddedImage = NULL;
    unsigned char *imagePtr;

    extrabytes = (4 - (width * 3) % 4) % 4;

    /* This is the size of the padded bitmap */
    bytesize = (width * 3 + extrabytes) * height;

    /* Fill the bitmap file header structure */
    bmpFileHeader.bfType = setUint16(BM);   /* Bitmap header */
    bmpFileHeader.bfSize = setUint32(0);      /* This can be 0 for BI_RGB bitmaps */
    bmpFileHeader.bfReserved1 = setUint16(0);
    bmpFileHeader.bfReserved2 = setUint16(0);
    bmpFileHeader.bfOffBits = setUint32(sizeof(BITMAPFILEHEADER) + sizeof(
                                            BITMAPINFOHEADER));

    /* Fill the bitmap info structure */
    bmpInfoHeader.biSize = setUint32(sizeof(BITMAPINFOHEADER));
    bmpInfoHeader.biWidth = setUint32(width);
    bmpInfoHeader.biHeight = setUint32(height);
    bmpInfoHeader.biPlanes = setUint16(1);
    bmpInfoHeader.biBitCount = setUint16(24);            /* 24 - bit bitmap */
    bmpInfoHeader.biCompression = setUint32(BI_RGB);
    bmpInfoHeader.biSizeImage = setUint32(
                                    bytesize);     /* includes padding for 4 byte alignment */
    bmpInfoHeader.biXPelsPerMeter = setUint32(0);
    bmpInfoHeader.biYPelsPerMeter = setUint32(0);
    bmpInfoHeader.biClrUsed = setUint32(0);
    bmpInfoHeader.biClrImportant = setUint32(0);


    /* Open file */
    if((filep = fopen(filename, "wb")) == NULL)
    {
        printf("Error opening file %s\n", filename);
        return BMP_FALSE;
    }

    /* Write bmp file header */
    if(fwrite(&bmpFileHeader, 1, sizeof(BITMAPFILEHEADER),
              filep) < sizeof(BITMAPFILEHEADER))
    {
        printf("Error writing bitmap file header\n");
        fclose(filep);
        return BMP_FALSE;
    }

    /* Write bmp info header */
    if(fwrite(&bmpInfoHeader, 1, sizeof(BITMAPINFOHEADER),
              filep) < sizeof(BITMAPINFOHEADER))
    {
        printf("Error writing bitmap info header\n");
        fclose(filep);
        return BMP_FALSE;
    }


    /* Allocate memory for some temporary storage */
    paddedImage = (unsigned char *)calloc(sizeof(unsigned char), bytesize);
    if(paddedImage == NULL)
    {
        printf("Error allocating memory \n");
        fclose(filep);
        return BMP_FALSE;
    }

    /* This code does three things.  First, it flips the image data upside down, as the .bmp
    format requires an upside down image.  Second, it pads the image data with extrabytes
    number of bytes so that the width in bytes of the image data that is written to the
    file is a multiple of 4.  Finally, it swaps (r, g, b) for (b, g, r).  This is another
    quirk of the .bmp file format. */

    for(row = 0; row < height; row++)
    {
        unsigned char *paddedImagePtr;


        imagePtr = image + (height - 1 - row) * width * 3;
        paddedImagePtr = paddedImage + row * (width * 3 + extrabytes);
        for(column = 0; column < width; column++)
        {
            *paddedImagePtr = *(imagePtr + 2);
            *(paddedImagePtr + 1) = *(imagePtr + 1);
            *(paddedImagePtr + 2) = *imagePtr;
            imagePtr += 3;
            paddedImagePtr += 3;
        }


    }

    /* Write bmp data */
    if(fwrite(paddedImage, 1, bytesize, filep) < bytesize)
    {
        printf("Error writing bitmap data\n");
        free(paddedImage);
        fclose(filep);
        return BMP_FALSE;
    }

    /* Close file */
    fclose(filep);
    free(paddedImage);

    return BMP_TRUE;
}





errno_t image_writeBMP_auto(
    const char *__restrict__ IDnameR,
    const char *__restrict__ IDnameG,
    const char *__restrict__ IDnameB,
    const char *__restrict__ outname
)
{
    imageID IDR, IDG, IDB;
    uint32_t width;
    uint32_t height;
    unsigned char *array;
    uint32_t ii, jj;
    double minr, ming, minb, maxr, maxg, maxb;


    minr = arith_image_min(IDnameR);
    ming = arith_image_min(IDnameG);
    minb = arith_image_min(IDnameB);

    maxr = arith_image_max(IDnameR);
    maxg = arith_image_max(IDnameG);
    maxb = arith_image_max(IDnameB);

    IDR = image_ID(IDnameR);
    IDG = image_ID(IDnameG);
    IDB = image_ID(IDnameB);
    width = (uint32_t) data.image[IDR].md[0].size[0];
    height = (uint32_t) data.image[IDR].md[0].size[1];

    array = (unsigned char *) malloc(sizeof(unsigned char) * width * height * 3);
    if(array == NULL) {
        PRINT_ERROR("malloc returns NULL pointer");
        abort();
    }

    for(ii = 0; ii < width; ii++)
        for(jj = 0; jj < height; jj++)
        {
            array[(jj * width + ii) * 3] = (unsigned char)((data.image[IDR].array.F[(height
                                           - jj - 1) * width + ii] - minr) * (255.0 / (maxr - minr)));
            array[(jj * width + ii) * 3 + 1] = (unsigned char)((
                                                   data.image[IDG].array.F[(height - jj - 1) * width + ii] - ming) * (255.0 /
                                                           (maxg - ming)));
            array[(jj * width + ii) * 3 + 2] = (unsigned char)((
                                                   data.image[IDB].array.F[(height - jj - 1) * width + ii] - minb) * (255.0 /
                                                           (maxb - minb)));
        }
    write24BitBmpFile(outname, width, height, array);
    free(array);

    return RETURN_SUCCESS;
}


errno_t image_writeBMP(
    const char *__restrict__ IDnameR,
    const char *__restrict__ IDnameG,
    const char *__restrict__ IDnameB,
    const char *__restrict__ outname
)
{
    imageID IDR, IDG, IDB;
    uint32_t width;
    uint32_t height;
    unsigned char *array;
    uint32_t ii, jj;

    IDR = image_ID(IDnameR);
    IDG = image_ID(IDnameG);
    IDB = image_ID(IDnameB);
    width = (uint32_t) data.image[IDR].md[0].size[0];
    height = (uint32_t) data.image[IDR].md[0].size[1];

    array = (unsigned char *) malloc(sizeof(unsigned char) * width * height * 3);
    if(array == NULL) {
        PRINT_ERROR("malloc returns NULL pointer");
        abort();
    }

    for(ii = 0; ii < width; ii++)
        for(jj = 0; jj < height; jj++)
        {
            array[(jj * width + ii) * 3] = (unsigned char)(data.image[IDR].array.F[(height -
                                           jj - 1) * width + ii]);
            array[(jj * width + ii) * 3 + 1] = (unsigned char)(
                                                   data.image[IDG].array.F[(height - jj - 1) * width + ii]);
            array[(jj * width + ii) * 3 + 2] = (unsigned char)(
                                                   data.image[IDB].array.F[(height - jj - 1) * width + ii]);
        }
    write24BitBmpFile(outname, width, height, array);
    free(array);

    return RETURN_SUCCESS;
}


