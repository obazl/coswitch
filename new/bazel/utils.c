#include <errno.h>
#include <stdio.h>
#include <unistd.h>

#include "liblogc.h"
#include "utstring.h"

#include "utils.h"

#if defined(PROFILE_fastbuild)
#define DEBUG_LEVEL coswitch_debug
extern int  DEBUG_LEVEL;
#define TRACE_FLAG coswitch_trace
extern bool TRACE_FLAG;
#endif

EXPORT void copy_buildfile(char *src_file, UT_string *to_file)
{
    TRACE_ENTRY;
    /* log_debug("copy_buildfile src: %s, dst: %s", */
    /*           src_file, utstring_body(to_file)); */
    UT_string *src;
    utstring_new(src);

    int rc = access(src_file, F_OK);
    if (rc != 0) {
        perror(utstring_body(src));
        log_error("not found: %s", src_file);
        /* fprintf(stderr, "not found: %s\n", utstring_body(src)); */
        exit(EXIT_FAILURE);
        return;
    }

    /* if (coswitch_debug) { */
    /*     log_debug("copying %s to %s\n", */
    /*               utstring_body(src), */
    /*               utstring_body(to_file)); */
    /* } */
    errno = 0;
    rc = copyfile(src_file,
                  utstring_body(to_file));
    if (rc != 0) {
        log_error("copyfile: %s", strerror(errno));
        fprintf(stderr, "ERROR copyfile: %s", strerror(errno));
        log_error("Exiting");
        fprintf(stderr, "Exiting\n");
        exit(EXIT_FAILURE);
    }
    /* TRACE_EXIT; */
}

