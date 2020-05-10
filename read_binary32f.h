/** @file read_binary32f.h
 */


errno_t read_binary32f_addCLIcmd();


imageID IMAGE_FORMAT_read_binary32f(
    const char *__restrict__ fname,
    long        xsize,
    long        ysize,
    const char *__restrict__ IDname
);
