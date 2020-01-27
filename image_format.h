#ifndef _IMAGEFORMATMODULE_H
#define _IMAGEFORMATMODULE_H


void __attribute__ ((constructor)) libinit_image_format();
errno_t init_image_format();


errno_t IMAGE_FORMAT_im_to_ASCII(
    const char *IDname,
    const char *foutname
);

//int IMAGE_FORMAT_FITS_to_ASCII(const char *finname, const char *foutname);


errno_t image_writeBMP_auto(
    const char *IDnameR,
    const char *IDnameG,
    const char *IDnameB,
    const char *outname
);


imageID CR2toFITS(
    const char *fnameCR2,
    const char *fnameFITS
);

imageID IMAGE_FORMAT_FITS_to_ushortintbin_lock(
    const char *IDname,
    const char *fname
);


imageID IMAGE_FORMAT_FITS_to_floatbin_lock(
    const char *IDname,
    const char *fname
);


imageID IMAGE_FORMAT_read_binary32f(
    const char *fname,
    long        xsize,
    long        ysize,
    const char *IDname
);


errno_t loadCR2toFITSRGB(
    const char *fnameCR2,
    const char *fnameFITSr,
    const char *fnameFITSg,
    const char *fnameFITSb
);

errno_t image_format_extract_RGGBchan(
    const char *ID_name,
    const char *IDoutR_name,
    const char *IDoutG1_name,
    const char *IDoutG2_name,
    const char *IDoutB_name
);

#endif

