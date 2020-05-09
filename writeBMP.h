/** @file writeBMP.h
 */

errno_t writeBMP_addCLIcmd();


errno_t image_writeBMP_auto(
    const char *__restrict__ IDnameR,
    const char *__restrict__ IDnameG,
    const char *__restrict__ IDnameB,
    const char *__restrict__ outname
);


errno_t image_writeBMP(
    const char *__restrict__ IDnameR,
    const char *__restrict__ IDnameG,
    const char *__restrict__ IDnameB,
    const char *__restrict__ outname
);
