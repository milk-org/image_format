/**
 * @file    cred_ql_and_utr.c
 * @brief   CDS + up-the-ramp image processing loop for CRED streams
 *
 * "CRED" streams because based on NDR counter location in top row pixel
 * Input: raw camera stream
 * Output 1: CDS quicklook
 * Output 2: UTR determination
 */

#include "CommandLineInterface/CLIcore.h"

// Local variables pointers
static char *in_imname;
static char *ql_imname;
static char *out_imname;
static double *sat_value;

static CLICMDARGDEF farg[] =
    {
        {CLIARG_IMG, ".in_name", "input image", "im1",
         CLIARG_VISIBLE_DEFAULT,
         (void **)&in_imname},
        {CLIARG_STR_NOT_IMG, ".ql_name", "double-sampling image", "out1",
         CLIARG_VISIBLE_DEFAULT,
         (void **)&ql_imname},
        {CLIARG_STR_NOT_IMG, ".out_name", "up-the-ramp image", "out2",
         CLIARG_VISIBLE_DEFAULT,
         (void **)&out_imname},
        {CLIARG_FLOAT, ".sat_value", "Saturation threshold", "satval",
         CLIARG_VISIBLE_DEFAULT,
         (void **)&sat_value}};

static CLICMDDATA CLIcmddata =
    {
        "cred_ql_utr",
        "RT compute of CDS and UTR for camera streams",
        CLICMD_FIELDS_DEFAULTS};

static errno_t help_function()
{
    printf("Perform real-time up-the-ramp data reduction on CRED1/2 streams.\n");
    return RETURN_SUCCESS;
}

/*
THE IMPORTANT, CUSTOM PART




*/

static errno_t copy_cast_SI16TOF(float *out, int16_t *in, int n_val)
{
    for (long ii = 0; ii < n_val; ++ii)
    {
        out[ii] = (float)in[ii];
    }

    return RETURN_SUCCESS;
}

static errno_t copy_cast_UI16TOF(float *out, uint16_t *in, int n_val)
{
    for (long ii = 0; ii < n_val; ++ii)
    {
        out[ii] = (float)in[ii];
    }

    return RETURN_SUCCESS;
}

static errno_t simple_desat_iterate(float *last_valid, int *frame_count, u_char *frame_valid, double sat_val, IMGID in_img, int reset)
{

    int n_pixels = in_img.md->size[0] * in_img.md->size[1];

    float in_val_px;
    int k;

    if (reset)
    {
        for (int ii = 8; ii < n_pixels; ++ii) // For all pixels, including the tags [we could skip the 1st row on the CREDs]
        {
            in_val_px = (float)in_img.im->array.UI16[ii];
            k = (in_val_px <= sat_val);
            frame_valid[ii] = k;
            frame_count[ii] = 1;
            
            last_valid[ii] = k ? in_val_px: 0.0f;
        }
    }
    else
    {
        for (int ii = 8; ii < n_pixels; ++ii) // For all pixels, including the tags [we could skip the 1st row on the CREDs]
        {
            in_val_px = (float)in_img.im->array.UI16[ii];
            k = (in_val_px <= sat_val);
            frame_valid[ii] = k;
            frame_count[ii] += k;

            last_valid[ii] = k ? in_val_px: last_valid[ii];
        }
    }

    return RETURN_SUCCESS;
}

static errno_t utr_iterate(float *sum_x, float *sum_y, float *sum_xy, float *sum_xx, float *sum_yy,
                           int *frame_count, u_char *frame_valid, double sat_val, IMGID in_img, int reset)
{

    int subframe_count = in_img.im->array.UI16[2]; // NDR raw counter
    int n_pixels = in_img.md->size[0] * in_img.md->size[1];

    float in_val_px;
    int k;

    if (reset)
    {
        for (int ii = 8; ii < n_pixels; ++ii) // For all pixels, including the tags [we could skip the 1st row on the CREDs]
        {
            in_val_px = (float)in_img.im->array.UI16[ii];

            // Detect saturation - which can have several forms for CRED1 / CRED2 / clipping to some max

            k = (in_val_px <= sat_val);
            frame_valid[ii] = k;

            frame_count[ii] = k; // At reset: 0 or 1

            sum_x[ii] = k * subframe_count;
            sum_y[ii] = k * in_val_px;
            sum_xy[ii] = (k * subframe_count) * in_val_px;
            sum_xx[ii] = (k * subframe_count) * subframe_count;
            sum_yy[ii] = (k * in_val_px) * in_val_px;
        }
    }
    else
    {                                         // not reset
        for (int ii = 8; ii < n_pixels; ++ii) // For all pixels, including the tags [we could skip the 1st row on the CREDs]
        {
            in_val_px = (float)in_img.im->array.UI16[ii];

            // Detect saturation - which can have several forms for CRED1 / CRED2 / clipping to some max
            k = (in_val_px <= sat_val);
            frame_valid[ii] = k;

            frame_count[ii] += k; // At reset: 0 or 1

            // Only perform those accumulations for unsat pixels, but this avoids if statements.
            sum_x[ii] += k * subframe_count;
            sum_y[ii] += k * in_val_px;
            sum_xy[ii] += (k * subframe_count) * in_val_px;
            sum_xx[ii] += (k * subframe_count) * subframe_count;
            sum_yy[ii] += (k * in_val_px) * in_val_px;

            /*
            if (frame_valid[ii]) {
                ++frame_count[ii]; // At reset: 0 or 1

                // Only perform those accumulations for unsat pixels, but this avoids if statements.
                sum_x[ii] += subframe_count;
                sum_y[ii] += in_val_px;
                sum_xy[ii] += subframe_count * in_val_px;
                sum_xx[ii] += subframe_count * subframe_count;
                sum_yy[ii] += in_val_px * in_val_px;
            }
            */
        }
    }

    return RETURN_SUCCESS;
}

static errno_t utr_reset_buffers(float *sum_x, float *sum_y, float *sum_xy, float *sum_xx, float *sum_yy,
                                 int *frame_count, u_char *frame_valid, int n_pixels)
{
    memset(sum_x, 0, n_pixels * SIZEOF_DATATYPE_FLOAT);
    memset(sum_y, 0, n_pixels * SIZEOF_DATATYPE_FLOAT);
    memset(sum_xy, 0, n_pixels * SIZEOF_DATATYPE_FLOAT);
    memset(sum_xx, 0, n_pixels * SIZEOF_DATATYPE_FLOAT);
    memset(sum_yy, 0, n_pixels * SIZEOF_DATATYPE_FLOAT);

    memset(frame_count, 0, n_pixels * SIZEOF_DATATYPE_INT32);
    memset(frame_valid, 1, n_pixels * SIZEOF_DATATYPE_UINT8);

    return RETURN_SUCCESS;
}

static errno_t ql_finalize(float *save_ql, IMGID in_img, IMGID ql_img)
{
    int n_pixels = in_img.md->size[0] * in_img.md->size[1];

    ql_img.im->md->write = TRUE;
    for (int ii = 0; ii < n_pixels; ++ii)
    {
        ql_img.im->array.F[ii] = (float)in_img.im->array.UI16[ii] - save_ql[ii];
    }

    return RETURN_SUCCESS;
}

static errno_t utr_finalize(float *sum_x, float *sum_y, float *sum_xy, float *sum_xx, float *sum_yy,
                            int *frame_count, u_char *frame_valid, int tot_num_frames, IMGID utr_img)
{
    int n_pixels = utr_img.md->size[0] * utr_img.md->size[1];

    utr_img.im->md->write = TRUE;

    // Skip the counters
    for (int ii = 8; ii < n_pixels; ++ii)
    {
        if (frame_count[ii] > 1) // Multiple valid readouts
        {
            // There's a minus because x is the decreasing raw number, thus decreases w/ time.
            utr_img.im->array.F[ii] = -tot_num_frames *
                                      (frame_count[ii] * sum_xy[ii] - sum_x[ii] * sum_y[ii]) /
                                      (frame_count[ii] * sum_xx[ii] - sum_x[ii] * sum_x[ii]);
            if ((frame_count[ii] * sum_xx[ii] - sum_x[ii] * sum_x[ii]) == 0)
            {
                utr_img.im->array.F[ii] = -1;
                //PRINT_WARNING("MADE NANs -- %d, %d, %f, %f", ii, frame_count[ii], sum_xx[ii], sum_x[ii]*sum_x[ii]);
            }
        }
        else if (frame_count[ii] == 1) // One single valid readout
        {
            utr_img.im->array.F[ii] = tot_num_frames * sum_x[ii];
        }
        else
        {
            utr_img.im->array.F[ii] = 0.0f;
        }
    }

    return RETURN_SUCCESS;
}

static errno_t simple_desat_finalize(float *last_valid, int *frame_count, int tot_num_frames, IMGID sds_img)
{

    int n_pixels = sds_img.md->size[0] * sds_img.md->size[1];

    sds_img.im->md->write = TRUE;
    int val;

    // Skip the counters
    for (int ii = 8; ii < n_pixels; ++ii)
    {
        val = frame_count[ii] >= 1; // Avoid no valid frames
        if (val)
        {
            sds_img.im->array.F[ii] = tot_num_frames * last_valid[ii] / frame_count[ii];
        }
        else
        {
            sds_img.im->array.F[ii] = -1.0f;
        }
    }

    return RETURN_SUCCESS;
}

/*
BOILERPLATE
*/

static errno_t compute_function()
{
    DEBUG_TRACE_FSTART();

    IMGID in_img = makeIMGID(in_imname);
    resolveIMGID(&in_img, ERRMODE_ABORT);

    // Set in_img to be the trigger
    strcpy(CLIcmddata.cmdsettings->triggerstreamname, in_imname);
    // for FPS mode:
    if (data.fpsptr != NULL)
    {
        strcpy(data.fpsptr->cmdset.triggerstreamname, in_imname);
    }

    // TODO DATATYPE - WE WANT IN and QL to be INT but UTR to be FLOAT

    // Resolve or create outputs, per need
    IMGID ql_img = makeIMGID(ql_imname);
    if (resolveIMGID(&ql_img, ERRMODE_WARN) == -1)
    {
        PRINT_WARNING("WARNING FOR QL");
        in_img.datatype = _DATATYPE_FLOAT; // To be passed to ql_img
        imcreatelikewiseIMGID(&ql_img, &in_img);
        resolveIMGID(&ql_img, ERRMODE_ABORT);
    }

    IMGID out_img = makeIMGID(out_imname);
    if (resolveIMGID(&out_img, ERRMODE_WARN))
    {
        PRINT_WARNING("WARNING FOR UTR");
        in_img.datatype = _DATATYPE_FLOAT; // To be passed to out_img
        imcreatelikewiseIMGID(&out_img, &in_img);
        resolveIMGID(&out_img, ERRMODE_ABORT);
    }

    /*
     Keyword setup - initialization
    */
    for (int kw = 0; kw < in_img.md->NBkw; ++kw)
    {
        strcpy(ql_img.im->kw[kw].name, in_img.im->kw[kw].name);
        ql_img.im->kw[kw].type = in_img.im->kw[kw].type;
        ql_img.im->kw[kw].value = in_img.im->kw[kw].value;
        strcpy(ql_img.im->kw[kw].comment, in_img.im->kw[kw].comment);

        strcpy(out_img.im->kw[kw].name, in_img.im->kw[kw].name);
        out_img.im->kw[kw].type = in_img.im->kw[kw].type;
        out_img.im->kw[kw].value = in_img.im->kw[kw].value;
        strcpy(out_img.im->kw[kw].comment, in_img.im->kw[kw].comment);
    }

    /*
    SETUP
    */
    // For counting NRD reads
    int cred_counter = 0;
    int prev_cred_counter = 0;
    // For counting frames and avoiding double processing when catching up with the semaphore
    int frame_counter = 0;
    int prev_frame_counter = 0;

    int ndr_value = 0;
    int old_ndr_value = 0;

    int n_pixels = in_img.md->size[0] * in_img.md->size[1];

    float *sum_x = (float *)malloc(n_pixels * SIZEOF_DATATYPE_FLOAT);
    float *sum_xx = (float *)malloc(n_pixels * SIZEOF_DATATYPE_FLOAT);

    float *sum_y = (float *)malloc(n_pixels * SIZEOF_DATATYPE_FLOAT);
    float *sum_xy = (float *)malloc(n_pixels * SIZEOF_DATATYPE_FLOAT);
    float *sum_yy = (float *)malloc(n_pixels * SIZEOF_DATATYPE_FLOAT);

    int *frame_count = (int *)malloc(n_pixels * SIZEOF_DATATYPE_INT32);
    char *frame_valid = (char *)malloc(n_pixels * SIZEOF_DATATYPE_INT8);
    float *last_valid = (float *)malloc(n_pixels * SIZEOF_DATATYPE_FLOAT);
    float *save_ql = (float *)malloc(n_pixels * SIZEOF_DATATYPE_FLOAT);

    // Reset the buffers for utr
    utr_reset_buffers(sum_x, sum_y, sum_xy, sum_xx, sum_yy, frame_count, frame_valid, n_pixels);
    // Reset the buffer for simple_desat
    memset(last_valid, 0, n_pixels * SIZEOF_DATATYPE_FLOAT);

    // TELEMETRY
    int just_init = FALSE;
    int miss_count = 0;

    PRINT_WARNING("Saturation value: %f", *sat_value);

    /*
    LOOP
    */

    INSERT_STD_PROCINFO_COMPUTEFUNC_START

    {
        old_ndr_value = ndr_value;

        prev_frame_counter = frame_counter;
        frame_counter = in_img.im->array.UI16[0];
        if (frame_counter == prev_frame_counter)
        {
            continue; // This applies to the loop started and closed in PROCINFO macros
        }

        // if we hit 0 just before, this is the first image, save it for the CDS
        prev_cred_counter = cred_counter;
        cred_counter = in_img.im->array.UI16[2]; // Counter in px 3

        if (frame_counter > prev_frame_counter + 1)
        {
            PRINT_WARNING("FRAME MISS %d (%d) %d (%d) - fyi NDR is: %d", frame_counter, prev_frame_counter, cred_counter, prev_cred_counter, ndr_value);
            // TODO don't forget you're missing the first frame of the ramp almost all the time.
        }
        /*
        HOUSEKEEPING
        */
        if (prev_cred_counter > 0 && cred_counter > prev_cred_counter)
        {
            PRINT_WARNING("Raw frame 0 missed - a UTR/SDS frame was lost");
        }

        /*
        INITIALIZE ACCS
        */

        if (prev_cred_counter == cred_counter || in_img.im->array.UI16[2] != 0)
        {   // Test: are we maybe in "rawimages off" ? In this case the counter gives the NDR total and never changes
            // Test: did we lose sync or imagetags ? 3rd pixel should alway be zero !
            ndr_value = 1;
            just_init = TRUE;
        }
        else if (prev_cred_counter == 0 || cred_counter > prev_cred_counter)
        { // Test: we are at the first frame of a burst OR we just missed the last frame of the previous burst
            // Counter in px 3, this is the first frame of the burst so it should be the total number
            // Counter not used in non-NDR mode, thus must set ndr_value to 1
            ndr_value = cred_counter + 1;

            if (ndr_value > 1)
            { // Test: no need for prep if we're in copy-through, non-NDR mode

                // Backup the first frame for CDS output
                if (in_img.md->datatype == _DATATYPE_UINT16)
                {
                    copy_cast_UI16TOF(save_ql, in_img.im->array.UI16, n_pixels);
                }
                else
                {
                    copy_cast_SI16TOF(save_ql, in_img.im->array.SI16, n_pixels);
                }
            }
            just_init = TRUE;
        }
        else
        {
            just_init = FALSE;
        }

        // just_init allows to ignore this condition when NDR = 1
        if (!just_init && cred_counter != prev_cred_counter - 1)
        {
            // TELEMETRY
            ++miss_count;
        }

        if (old_ndr_value != ndr_value)
        {
            PRINT_WARNING("NDR meas changed from %d to %d", old_ndr_value, ndr_value);
        }

        /*
        ACCUMULATE
        */
        if (ndr_value > 1 && ndr_value <= 6)
        {
            simple_desat_iterate(last_valid, frame_count, frame_valid, *sat_value, in_img, just_init);
        }
        else if (ndr_value > 6)
        {
            utr_iterate(sum_x, sum_y, sum_xy, sum_xx, sum_yy, frame_count, frame_valid, *sat_value, in_img, just_init);
        }

        /*
        FINALIZE
        */
        if (cred_counter == 0 || ndr_value == 1) // If we are hitting 0, compute the UTR, the QL, and post the outputs
        {
            if (ndr_value == 1)
            { // ndr_value == 1: single reads OR rawimages off passthrough mode
                if (in_img.md->datatype == _DATATYPE_UINT16)
                {
                    copy_cast_UI16TOF(out_img.im->array.F, in_img.im->array.UI16, n_pixels);
                    copy_cast_UI16TOF(ql_img.im->array.F, in_img.im->array.UI16, n_pixels);
                }
                else
                {
                    copy_cast_SI16TOF(out_img.im->array.F, in_img.im->array.SI16, n_pixels);
                    copy_cast_SI16TOF(ql_img.im->array.F, in_img.im->array.SI16, n_pixels);
                }
            }
            else
            {
                if (ndr_value <= 6)
                {
                    simple_desat_finalize(last_valid, frame_count, ndr_value, out_img);
                }
                else
                {
                    utr_finalize(sum_x, sum_y, sum_xy, sum_xx, sum_yy, frame_count, frame_valid, ndr_value, out_img);
                }
                ql_finalize(save_ql, in_img, ql_img); // TODO ql output is gonna become useless memcopies
            }

            /*
            Keyword value carry-over
            */
            for (int kw = 0; kw < in_img.md->NBkw; ++kw)
            {
                ql_img.im->kw[kw].value = in_img.im->kw[kw].value;
                out_img.im->kw[kw].value = in_img.im->kw[kw].value;
            }

            processinfo_update_output_stream(processinfo, ql_img.ID); // FIXME can 2 outputs be used ?
            processinfo_update_output_stream(processinfo, out_img.ID);

            // TODO compute if we missed frames during that ramp.
            // HOUSEKEEPING
            if (miss_count > 0)
            {
                PRINT_WARNING("UTR/SDS ramp - missing %d/%d frames (cnt0 %ld)", miss_count, ndr_value, in_img.md->cnt0);
            }
            miss_count = 0;

            // TODO ??? Weighted stuff.
        }
    }

    INSERT_STD_PROCINFO_COMPUTEFUNC_END

    /*
    TEARDOWN
    */
    free(sum_x);
    free(sum_y);
    free(sum_xy);
    free(sum_xx);
    free(sum_yy);

    free(frame_count);
    free(frame_valid);
    free(save_ql);

    DEBUG_TRACE_FEXIT();
    return RETURN_SUCCESS;
}

INSERT_STD_FPSCLIfunctions

    // Register function in CLI
    errno_t
    CLIADDCMD_uptheramp__cred_ql_utr()
{
    INSERT_STD_CLIREGISTERFUNC

    return RETURN_SUCCESS;
}