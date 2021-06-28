#ifndef IMAGE_FORMAT_MKBMPIMAGE_H
#define IMAGE_FORMAT_MKBMPIMAGE_H

errno_t CLIADDCMD_image_format__mkBMPimage();

errno_t image_writeBMP(
    const char *__restrict__ IDnameR,
    const char *__restrict__ IDnameG,
    const char *__restrict__ IDnameB,
    char *__restrict__ outname
);

#endif
