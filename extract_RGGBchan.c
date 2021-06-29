/** @file extract_RGGBchan.c
 */


#include "CommandLineInterface/CLIcore.h"
#include "COREMOD_memory/COREMOD_memory.h"


// ==========================================
// Forward declaration(s)
// ==========================================

errno_t image_format_extract_RGGBchan(
    const char *__restrict__ ID_name,
    const char *__restrict__ IDoutR_name,
    const char *__restrict__ IDoutG1_name,
    const char *__restrict__ IDoutG2_name,
    const char *__restrict__ IDoutB_name
);


// ==========================================
// Command line interface wrapper function(s)
// ==========================================

static errno_t IMAGE_FORMAT_extract_RGGBchan_cli()
{
    if(CLI_checkarg(1, 4) + CLI_checkarg(2, 3) + CLI_checkarg(3,
            3) + CLI_checkarg(4, 3) + CLI_checkarg(5, 3) == 0)
    {
        image_format_extract_RGGBchan(data.cmdargtoken[1].val.string,
                                      data.cmdargtoken[2].val.string, data.cmdargtoken[3].val.string,
                                      data.cmdargtoken[4].val.string, data.cmdargtoken[5].val.string);
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

errno_t extract_RGGBchan_addCLIcmd()
{

    RegisterCLIcommand(
        "extractRGGBchan",
        __FILE__,
        IMAGE_FORMAT_extract_RGGBchan_cli,
        "extract RGGB channels from color image",
        "<input image> <imR> <imG1> <imG2> <imB>",
        "extractRGGBchan im imR imG1 imG2 imB",
        "int image_format_extract_RGGBchan(const char *ID_name, const char *IDoutR_name, const char *IDoutG1_name, const char *IDoutG2_name, const char *IDoutB_name)"
    );


    return RETURN_SUCCESS;
}







//
// separates a single RGB image into its 4 channels
// output written in im_r, im_g1, im_g2 and im_b
//
errno_t image_format_extract_RGGBchan(
    const char *__restrict__ ID_name,
    const char *__restrict__ IDoutR_name,
    const char *__restrict__ IDoutG1_name,
    const char *__restrict__ IDoutG2_name,
    const char *__restrict__ IDoutB_name
)
{
    imageID ID;
    long Xsize, Ysize;
    imageID IDr, IDg1, IDg2, IDb;
    long xsize2, ysize2;
    long ii, jj, ii1, jj1;
    int RGBmode = 0;
    imageID ID00, ID01, ID10, ID11;

    ID = image_ID(ID_name);
    Xsize = data.image[ID].md[0].size[0];
    Ysize = data.image[ID].md[0].size[1];

    printf("ID = %ld\n", ID);
    printf("size = %ld %ld\n", Xsize, Ysize);


    if((Xsize == 4770) && (Ysize == 3178))
    {
        RGBmode = 1;
    }
    if((Xsize == 5202) && (Ysize == 3465))
    {
        RGBmode = 2;
    }



    if(RGBmode == 0)
    {
        //PRINT_ERROR("Unknown RGB image mode\n");
        //exit(0);
        RGBmode = 1;
    }

    xsize2 = Xsize / 2;
    ysize2 = Ysize / 2;

    printf("Creating color channel images, %ld x %ld\n", xsize2, ysize2);
    fflush(stdout);


    create_2Dimage_ID(IDoutR_name, xsize2, ysize2, &IDr);
    create_2Dimage_ID(IDoutG1_name, xsize2, ysize2, &IDg1);
    create_2Dimage_ID(IDoutG2_name, xsize2, ysize2, &IDg2);
    create_2Dimage_ID(IDoutB_name, xsize2, ysize2, &IDb);

    printf("STEP 2\n");
    fflush(stdout);

    if(RGBmode == 1) // GBRG
    {
        ID00 = IDg1;
        ID10 = IDb;
        ID01 = IDr;
        ID11 = IDg2;
    }

    if(RGBmode == 2)
    {
        ID00 = IDr;
        ID10 = IDg1;
        ID01 = IDg2;
        ID11 = IDb;
    }


    for(ii = 0; ii < xsize2; ii++)
        for(jj = 0; jj < ysize2; jj++)
        {
            ii1 = 2 * ii;
            jj1 = 2 * jj;

            data.image[ID01].array.F[jj * xsize2 + ii] =
                data.image[ID].array.F[(jj1 + 1)*Xsize + ii1];

            data.image[ID00].array.F[jj * xsize2 + ii] =
                data.image[ID].array.F[jj1 * Xsize + ii1];

            data.image[ID11].array.F[jj * xsize2 + ii] =
                data.image[ID].array.F[(jj1 + 1) * Xsize + (ii1 + 1)];

            data.image[ID10].array.F[jj * xsize2 + ii] =
                data.image[ID].array.F[jj1 * Xsize + (ii1 + 1)];

        }

    return RETURN_SUCCESS;
}

