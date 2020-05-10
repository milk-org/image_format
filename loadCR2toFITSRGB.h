/** @file loadCR2toFITSRGB.h
 */


errno_t loadCR2toFITSRGB_addCLIcmd();

errno_t loadCR2toFITSRGB(
    const char *__restrict__ fnameCR2,
    const char *__restrict__ fnameFITSr,
    const char *__restrict__ fnameFITSg,
    const char *__restrict__ fnameFITSb
);
