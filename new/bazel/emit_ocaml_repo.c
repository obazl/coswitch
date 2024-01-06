#include <errno.h>
#include <dirent.h>
#ifdef __linux__
#include <linux/limits.h>
#else
#include <limits.h>
#endif
/* #if EXPORT_INTERFACE */
#include <stdio.h>
/* #endif */
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "liblogc.h"
#include "findlibc.h"
#include "opamc.h"
#include "utarray.h"
#include "utstring.h"

/* #include "cjson/cJSON.h" */
/* #include "mustach-cjson.h" */
/* #include "mustach.h" */

#include "emit_ocaml_repo.h"

extern int verbosity;
extern int log_writes;
extern int log_symlinks;

extern char *default_version;
extern int   default_compat;
extern char *bazel_compat;

/* LOCAL const char *ws_name = "mibl"; */

/* extern UT_string *opam_switch_id; */
/* extern UT_string *opam_switch_prefix; */
extern UT_string *opam_ocaml_version;
/* extern UT_string *opam_switch_bin; */
/* extern UT_string *opam_switch_lib; */
/* extern UT_string *coswitch_runfiles_root; */

#if defined(PROFILE_fastbuild)
#define DEBUG_LEVEL coswitch_debug
extern int  DEBUG_LEVEL;
#define TRACE_FLAG coswitch_trace
extern bool TRACE_FLAG;
bool coswitch_debug_symlinks = false;
#endif

void _emit_toplevel(UT_string *templates,
                    char *template,
                    UT_string *src_file,
                    UT_string *dst_dir,
                    UT_string *dst_file,
                    char *coswitch_lib,
                    char *pkg
                    )
{
    TRACE_ENTRY;
    // create <switch>/lib/str for ocaml >= 5.0.0
    // previous versions already have it
    // step 1: write MODULE.bazel, WORKSPACE.bazel
    // step 2: write lib/str/lib/str/BUILD.bazel
    // step 3: write registry record
    char *content_template = ""
        "## generated file - DO NOT EDIT\n"
        "\n"
        "module(\n"
        "    name = \"%s\", version = \"0.0.0\",\n"
        "    compatibility_level = \"0\"\n"
        "    bazel_compatibility = [\">=%s\"]\n"
        ")\n"
        "\n"
        "bazel_dep(name = \"ocaml\", version = \"0.0.0\")\n";
    UT_string *content;
    utstring_new(content);
    utstring_printf(content, content_template,
                    pkg, bazel_compat);

    utstring_renew(dst_dir);
    utstring_printf(dst_dir,
                    "%s/%s",
                    coswitch_lib,
                    pkg);
    mkdir_r(utstring_body(dst_dir));
    utstring_renew(dst_file);
    utstring_printf(dst_file, "%s/MODULE.bazel",
                    utstring_body(dst_dir));
    // write content to dst_file
    FILE *ostream = fopen(utstring_body(dst_file), "w");
    if (ostream == NULL) {
        perror(utstring_body(dst_file));
        log_error("fopen: %s: %s", strerror(errno),
                  utstring_body(dst_file));
        exit(EXIT_FAILURE);
    }
    fprintf(ostream, "%s", utstring_body(content));
    fclose(ostream);
    if (verbosity > log_writes) {
        LOG_INFO(0, "wrote: %s", utstring_body(dst_file));
    }
    utstring_renew(dst_file);
    utstring_printf(dst_file, "%s/WORKSPACE.bazel",
                    utstring_body(dst_dir));
    // write content to ws file
    ostream = fopen(utstring_body(dst_file), "w");
    if (ostream == NULL) {
        perror(utstring_body(dst_file));
        log_error("fopen: %s: %s", strerror(errno),
                  utstring_body(dst_file));
        exit(EXIT_FAILURE);
    }
    fprintf(ostream, "## generated file - DO NOT EDIT\n");
    fclose(ostream);
    if (verbosity > log_writes)
        fprintf(INFOFD, GRN "INFO" CRESET
                " wrote: %s\n", utstring_body(dst_file));

    utstring_renew(dst_dir);
    utstring_printf(dst_dir,
                    "%s/%s/lib/%s",
                    coswitch_lib,
                    pkg, pkg);
    mkdir_r(utstring_body(dst_dir));
    utstring_renew(dst_file);
    utstring_printf(dst_file, "%s/BUILD.bazel",
                    utstring_body(dst_dir));

    utstring_renew(src_file);
    utstring_printf(src_file, "%s/%s",
                    utstring_body(templates),
                    template);

    LOG_DEBUG(2, "cp src: %s, dst: %s",
              utstring_body(src_file), utstring_body(dst_file));
    copy_buildfile(utstring_body(src_file), dst_file);
    TRACE_EXIT;
}

FILE *_open_buildfile(UT_string *ocaml_file)
{
    FILE *ostream = fopen(utstring_body(ocaml_file), "w");
    /* ostream =       fopen(utstring_body(ocaml_file), "w"); */
    if (ostream == NULL) {
        log_error("fopen: %s: %s", strerror(errno),
                  utstring_body(ocaml_file));
        fprintf(stderr, "fopen: %s: %s", strerror(errno),
                utstring_body(ocaml_file));
        fprintf(stderr, "exiting\n");
        /* perror(utstring_body(ocaml_file)); */
        exit(EXIT_FAILURE);
    }
    return ostream;
}

void emit_ocaml_stdlib_pkg(char *runfiles,
                           char *switch_lib,
                           char *coswitch_lib)
{
    TRACE_ENTRY;
    bool add_toplevel = false;

    UT_string *stdlib_dir;
    utstring_new(stdlib_dir);
    utstring_printf(stdlib_dir,
                    "%s/stdlib",
                    switch_lib);
    int rc = access(utstring_body(stdlib_dir), F_OK);
    if (rc != 0) {
#if defined(PROFILE_fastbuild)
        /* if (coswitch_trace) */
        LOG_WARN(0, YEL "NOT FOUND: %s" CRESET,
                 utstring_body(stdlib_dir));
#endif
        utstring_new(stdlib_dir);
        utstring_printf(stdlib_dir,
                        "%s/ocaml/stdlib",
                        switch_lib);
        int rc = access(utstring_body(stdlib_dir), F_OK);
        if (rc != 0) {
#if defined(PROFILE_fastbuild)
            /* if (coswitch_trace) */
            LOG_WARN(0, YEL "NOT FOUND: %s" CRESET,
                     utstring_body(stdlib_dir));
#endif
            utstring_free(stdlib_dir);
            return;
        } else {
#if defined(PROFILE_fastbuild)
            LOG_WARN(0, YEL "FOUND: %s" CRESET,
                     utstring_body(stdlib_dir));
#endif
            add_toplevel = true;
        }
/* #if defined(PROFILE_fastbuild) */
/*     } else { */
/*         LOG_WARN(0, YEL "FOUND: %s" CRESET, utstring_body(stdlib_dir)); */
/* #endif */
    }

    UT_string *templates;
    utstring_new(templates);
    utstring_printf(templates, "%s/%s",
                    runfiles, "/templates/ocaml");

    UT_string *src_file;
    UT_string *dst_dir;
    UT_string *dst_file;
    utstring_new(src_file);
    utstring_new(dst_dir);
    utstring_new(dst_file);

    // 5.0.0+ has <switch>/lib/ocaml/stdlib/META
    // pre-5.0.0: no <switch>/lib/ocaml/stdlib
    // always emit <coswitch>/lib/ocaml/stdlib
    utstring_printf(src_file, "%s/%s",
                    utstring_body(templates),
                    "ocaml_stdlib.BUILD");
    utstring_printf(dst_dir,
                    "%s/ocaml/lib/stdlib",
                    coswitch_lib);
    mkdir_r(utstring_body(dst_dir));
    utstring_printf(dst_file, "%s/BUILD.bazel",
                    utstring_body(dst_dir));
    LOG_DEBUG(2, "cp src: %s, dst: %s",
              utstring_body(src_file), utstring_body(dst_file));
    copy_buildfile(utstring_body(src_file), dst_file);

    if (add_toplevel) {
        // not found: <switch>/lib/stdlib
        // means >= 5.0.0 (<switch>/lib/ocaml/stdlib)

        // add <coswitch>/lib/stdlib for (bazel) compatibility
        _emit_toplevel(templates, "ocaml_stdlib_alias.BUILD",
                       src_file,
                       dst_dir, dst_file,
                       coswitch_lib, "stdlib");
    } else {
        // if we found <switch>/lib/stdlib (versions < 5.0.0)
        // then we need to populate <coswitch>/lib/ocaml/stdlib
        // (its already there for >= 5.0.0)
        utstring_new(stdlib_dir);
        utstring_printf(stdlib_dir,
                        "%s/ocaml",
                        switch_lib);
        /* _symlink_ocaml_stdlib(stdlib_dir, */
        /*                        utstring_body(dst_dir)); */
    }

    /* stdlib files are always in <switchpfx>/lib/ocaml
       even for v5
     */
    utstring_renew(stdlib_dir);
    utstring_printf(stdlib_dir,
                    "%s/ocaml",
                    switch_lib);
    utstring_renew(dst_dir);
    utstring_printf(dst_dir,
                    "%s/ocaml/lib/stdlib",
                    coswitch_lib);
    _symlink_ocaml_stdlib(stdlib_dir,
                          utstring_body(dst_dir));

    utstring_free(src_file);
    utstring_free(dst_file);
    utstring_free(templates);
    utstring_free(stdlib_dir);
    TRACE_EXIT;

    /* UT_string *ocaml_file; */
    /* utstring_new(ocaml_file); */
    /* utstring_concat(ocaml_file, coswitch_pfx); */
    /* utstring_printf(ocaml_file, "/ocaml/lib/stdlib"); */
    /* mkdir_r(utstring_body(ocaml_file)); */

    /* _symlink_ocaml_stdlib(utstring_body(ocaml_file)); */

    /* utstring_printf(ocaml_file, "/BUILD.bazel"); */

    /* copy_buildfile("ocaml_stdlib.BUILD", ocaml_file); */
    /* utstring_free(ocaml_file); */
}

void _symlink_ocaml_stdlib(UT_string *stdlib_srcdir, char *tgtdir)
{
    TRACE_ENTRY;
    /* LOG_DEBUG(0, "stdlib_srcdir: %s", utstring_body(stdlib_srcdir)); */
    /* LOG_DEBUG(0, "tgtdir: %s", tgtdir); */

    /* UT_string *opamdir; */
    /* utstring_new(opamdir); */
    /* utstring_printf(opamdir, "%s/ocaml", utstring_body(opam_stdlib_srcdir)); */

    UT_string *src;
    utstring_new(src);
    UT_string *dst;
    utstring_new(dst);
    int rc;

    DIR *d = opendir(utstring_body(stdlib_srcdir));
    if (d == NULL) {
        log_error("Unable to opendir for symlinking stdlib: %s\n",
                  utstring_body(stdlib_srcdir));
        /* exit(EXIT_FAILURE); */
        return;
    }

    struct dirent *direntry;
    while ((direntry = readdir(d)) != NULL) {
        if(direntry->d_type==DT_REG){
            if (strncmp("stdlib", direntry->d_name, 6) != 0) {
                if (strncmp("camlinternal",
                            direntry->d_name, 12) != 0) {
                    continue;
                }
            }

            utstring_renew(src);
            utstring_printf(src, "%s/%s",
                            utstring_body(stdlib_srcdir), direntry->d_name);
            utstring_renew(dst);
            utstring_printf(dst, "%s/%s",
                            tgtdir, direntry->d_name);
            /* LOG_DEBUG(0, "symlinking %s to %s", */
            /*           utstring_body(src), */
            /*           utstring_body(dst)); */
            rc = symlink(utstring_body(src),
                         utstring_body(dst));
            if (rc != 0) {
                if (errno != EEXIST) {
                    perror(utstring_body(src));
                    fprintf(stderr, "exiting\n");
                    exit(EXIT_FAILURE);
                }
            }
        }
    }
    closedir(d);
    TRACE_EXIT;
}

void emit_ocaml_runtime_pkg(char *runfiles,
                            char *switch_lib,
                            char *coswitch_lib)  // dest
{
    TRACE_ENTRY;
    UT_string *dst_dir;
    utstring_new(dst_dir);
    utstring_printf(dst_dir,
                    "%s/ocaml/runtime",
                    /* "%s/ocaml/lib/runtime", */
                    coswitch_lib);
    mkdir_r(utstring_body(dst_dir));

    UT_string *templates;
    utstring_new(templates);
    utstring_printf(templates, "%s/templates/ocaml", runfiles);
    /* utstring_printf(templates, "%s/%s", */
    /*                 runfiles, "/new/coswitch/templates"); */

    /* _symlink_ocaml_runtime(utstring_body(dst_file)); */
    _symlink_ocaml_runtime_libs(switch_lib, utstring_body(dst_dir));
    _symlink_ocaml_runtime_compiler_libs(switch_lib, utstring_body(dst_dir));
    /* _symlink_ocaml_runtime(switch_lib, utstring_body(dst_dir)); */

    /* ******************************** */
    UT_string *src_file;
    utstring_new(src_file);
    utstring_printf(src_file, "%s/%s",
                    utstring_body(templates),
                    "ocaml_runtime.BUILD");

    UT_string *dst_file;
    utstring_new(dst_file);
    utstring_printf(dst_file,
                    "%s/BUILD.bazel",
                    utstring_body(dst_dir));

    LOG_DEBUG(2, "cp src: %s, dst: %s",
              utstring_body(src_file), utstring_body(dst_file));
    copy_buildfile(utstring_body(src_file), dst_file);

    /* ******************************** */
    utstring_renew(src_file);
    utstring_printf(src_file, "%s/runtime/%s",
                    utstring_body(templates),
                    "runtime_sys.BUILD");

    utstring_renew(dst_dir);
    utstring_printf(dst_dir,
                    "%s/ocaml/runtime/sys",
                    coswitch_lib);
    mkdir_r(utstring_body(dst_dir));
    utstring_renew(dst_file);
    utstring_printf(dst_file,
                    "%s/BUILD.bazel",
                    utstring_body(dst_dir));
    LOG_DEBUG(2, "cp src: %s, dst: %s",
              utstring_body(src_file), utstring_body(dst_file));
    copy_buildfile(utstring_body(src_file), dst_file);

    /* ******************************** */
    utstring_renew(src_file);
    utstring_printf(src_file, "%s/runtime/%s",
                    utstring_body(templates),
                    "runtime_vm.BUILD");
    utstring_renew(dst_dir);
    utstring_printf(dst_dir,
                    "%s/ocaml/runtime/vm",
                    coswitch_lib);
    mkdir_r(utstring_body(dst_dir));
    utstring_renew(dst_file);
    utstring_printf(dst_file,
                    "%s/BUILD.bazel",
                    utstring_body(dst_dir));
    LOG_DEBUG(2, "cp src: %s, dst: %s",
              utstring_body(src_file), utstring_body(dst_file));
    copy_buildfile(utstring_body(src_file), dst_file);

    utstring_free(src_file);
    utstring_free(dst_dir);
    utstring_free(templates);
    TRACE_EXIT;
}

void _symlink_ocaml_runtime(char *switch_lib, char *tgtdir)
{
    TRACE_ENTRY;
    LOG_DEBUG(0, "rt switchlib: %s", switch_lib);
    LOG_DEBUG(0, "rt tgtdir: %s", tgtdir);
    UT_string *opamdir;
    utstring_new(opamdir);
    utstring_printf(opamdir, "%s/ocaml",
                    switch_lib);
                    /* utstring_body(opam_switch_lib)); */

    UT_string *src;
    utstring_new(src);
    UT_string *dst;
    utstring_new(dst);
    int rc;

    DIR *d = opendir(utstring_body(opamdir));
    if (d == NULL) {
        perror(utstring_body(opamdir));
        log_error(RED "Unable to opendir for symlinking stdlib" CRESET);
        exit(EXIT_FAILURE);
        return;
    }

    struct dirent *direntry;
    while ((direntry = readdir(d)) != NULL) {
        /* LOG_DEBUG(0, "DIRENT"); */
        if(direntry->d_type==DT_REG){
            if (strncmp("stdlib", direntry->d_name, 6) != 0)
                if (strncmp("std_exit", direntry->d_name, 8) != 0)
                    continue;

            utstring_renew(src);
            utstring_printf(src, "%s/%s",
                            utstring_body(opamdir), direntry->d_name);
            utstring_renew(dst);
            utstring_printf(dst, "%s/%s",
                            tgtdir,
                            direntry->d_name);
            if (verbosity > log_symlinks) {
                fprintf(INFOFD, GRN "INFO" CRESET
                        " symlink: %s -> %s\n",
                          utstring_body(src),
                          utstring_body(dst));
            }

            rc = symlink(utstring_body(src),
                         utstring_body(dst));
            symlink_ct++;
            if (rc != 0) {
                if (errno != EEXIST) {
                    perror(utstring_body(src));
                    fprintf(stderr, "exiting\n");
                    exit(EXIT_FAILURE);
                }
            }
        }
    }
    closedir(d);
    TRACE_EXIT;
}

/* ************************************** */
void emit_ocaml_stublibs(char *switch_pfx,
                         char *coswitch_lib)
{
    TRACE_ENTRY;
    UT_string *dst_file;
    utstring_new(dst_file);
    utstring_printf(dst_file,
                    "%s/ocaml/lib/stublibs",
                    coswitch_lib);
    mkdir_r(utstring_body(dst_file));
    utstring_printf(dst_file, "/BUILD.bazel");

    FILE *ostream = fopen(utstring_body(dst_file), "w");
    ostream = fopen(utstring_body(dst_file), "w");
    if (ostream == NULL) {
        perror(utstring_body(dst_file));
        log_error("fopen: %s: %s", strerror(errno),
                  utstring_body(dst_file));
        exit(EXIT_FAILURE);
    }
    fprintf(ostream, "# generated file - DO NOT EDIT\n");
    fprintf(ostream, "exports_files(glob([\"**\"]))\n");
    fprintf(ostream,
            "package(default_visibility=[\"//visibility:public\"])\n\n");

    fprintf(ostream, "filegroup(\n");
    fprintf(ostream, "    name = \"stublibs\",\n");
    fprintf(ostream, "    srcs = glob([\"dll*\"]),\n");
    fprintf(ostream, "    data = glob([\"dll*\"]),\n");
    fprintf(ostream, ")\n");
    fclose(ostream);
    if (verbosity > log_writes)
        fprintf(INFOFD, GRN "INFO" CRESET
                " wrote: %s\n", utstring_body(dst_file));

    utstring_free(dst_file);

    _emit_ocaml_stublibs_symlinks(switch_pfx,
                                  coswitch_lib,
                                  "ocaml/stublibs");
    TRACE_EXIT;
}

void _emit_ocaml_stublibs_symlinks(char *switch_pfx,
                                   char *coswitch_lib,
                                   char *tgtdir)
{
    TRACE_ENTRY;
    UT_string *src_dir;
    utstring_new(src_dir);
    utstring_printf(src_dir,
                    "%s/lib/%s",
                    switch_pfx,
                    tgtdir);

    UT_string *dst_dir;
    utstring_new(dst_dir);
    utstring_printf(dst_dir,
                    "%s/ocaml/lib/stublibs",
                    coswitch_lib);
                    /* tgtdir); */
    mkdir_r(utstring_body(dst_dir));

    LOG_DEBUG(1, "stublibs src_dir: %s", utstring_body(src_dir));
    LOG_DEBUG(1, "stublibs dst_dir: %s", utstring_body(dst_dir));

    UT_string *src_file;
    utstring_new(src_file);
    UT_string *dst_file;
    utstring_new(dst_file);
    int rc;

    LOG_DEBUG(1, "opening src_dir for read: %s",
              utstring_body(src_dir));

    DIR *srcd = opendir(utstring_body(src_dir));
    /* DIR *srcd = opendir(utstring_body(opamdir)); */
    if (srcd == NULL) {
        fprintf(stderr, "Unable to opendir for symlinking stublibs: %s\n",
                utstring_body(src_dir));
        /* exit(EXIT_FAILURE); */
        /* this can happen if a related pkg is not installed */
        /* example, see topkg and topkg-care */
        return;
    }

    struct dirent *direntry;
    while ((direntry = readdir(srcd)) != NULL) {
        //Condition to check regular file.
        LOG_DEBUG(1, "stublib: %s, type %d",
                  direntry->d_name, direntry->d_type);
        if( (direntry->d_type==DT_REG)
            || (direntry->d_type==DT_LNK) ){

            /* do not symlink workspace file */
            if (strncmp(direntry->d_name, "WORKSPACE", 9) == 0) {
                continue;
            }
            utstring_renew(src_file);
            utstring_printf(src_file, "%s/%s",
                            utstring_body(src_dir),
                            direntry->d_name);
            utstring_renew(dst_file);
            utstring_printf(dst_file, "%s/%s",
                            utstring_body(dst_dir),
                            direntry->d_name);

            if (verbosity > log_symlinks) {
                fprintf(INFOFD, GRN "INFO" CRESET
                        " symlink: %s -> %s\n",
                          utstring_body(src_file),
                          utstring_body(dst_file));
            }

            rc = symlink(utstring_body(src_file),
                         utstring_body(dst_file));
            symlink_ct++;
            if (rc != 0) {
                switch (errno) {
                case EEXIST:
                    goto ignore;
                case ENOENT:
                    log_error("symlink ENOENT: %s", strerror(errno));
                    log_error("a component of '%s' does not name an existing file",  utstring_body(dst_dir));
                    fprintf(stderr, "symlink ENOENT: %s\n", strerror(errno));
                    fprintf(stderr, "A component of '%s' does not name an existing file.\n",  utstring_body(dst_dir));
                    break;
                default:
                    log_error("symlink err: %s", strerror(errno));
                    fprintf(stderr, "symlink err: %s", strerror(errno));
                }
                log_error("Exiting");
                fprintf(stderr, "Error, exiting\n");
                exit(EXIT_FAILURE);
            ignore:
                ;
            }
        }
    }
    closedir(srcd);
    TRACE_EXIT;
}

/**
   <switch>/lib/stublibs - no META file, populated by other pkgs
 */
void emit_lib_stublibs_pkg(UT_string *registry,
                           char *switch_stublibs,
                           char *coswitch_lib)
{
    (void)registry;
    TRACE_ENTRY;
    UT_string *switch_stublibs_dir;
    utstring_new(switch_stublibs_dir);
    utstring_printf(switch_stublibs_dir,
                    "%s",
                    switch_stublibs);
    int rc = access(utstring_body(switch_stublibs_dir), F_OK);
    if (rc != 0) {
#if defined(PROFILE_fastbuild)
        /* if (coswitch_trace) */
        LOG_WARN(0, YEL "no stublibs dir found: %s" CRESET,
                 utstring_body(switch_stublibs_dir));
#endif
        return;
    }

    UT_string *dst_dir;
    utstring_new(dst_dir);
    utstring_printf(dst_dir,
                    "%s/stublibs",
                    coswitch_lib);
    mkdir_r(utstring_body(dst_dir));

    UT_string *dst_file;
    utstring_new(dst_file);
    utstring_printf(dst_file,
                    "%s/MODULE.bazel",
                    utstring_body(dst_dir));
    FILE *ostream;
    ostream = fopen(utstring_body(dst_file), "w");
    if (ostream == NULL) {
        log_error("%s", strerror(errno));
        perror(utstring_body(dst_file));
        exit(EXIT_FAILURE);
    }
    fprintf(ostream, "## generated file - DO NOT EDIT\n\n");
    fprintf(ostream, "module(\n");
    fprintf(ostream, "    name = \"stublibs\", version = \"%s\",\n",
            default_version);
    fprintf(ostream, "    compatibility_level = %d,\n",
            default_compat);
    fprintf(ostream, ")\n");
    fprintf(ostream, "\n");

    fprintf(ostream, "bazel_dep(name = \"rules_ocaml\",");
    fprintf(ostream, " version = \"%s\")\n", rules_ocaml_version);

    fprintf(ostream,
            "bazel_dep(name = \"ocaml\", version = \"%s\")\n", "0.0.0");

    fprintf(ostream,
            "bazel_dep(name = \"bazel_skylib\", version = \"%s\")\n", skylib_version);

    fclose(ostream);
    if (verbosity > log_writes)
        fprintf(INFOFD, GRN "INFO" CRESET
                " wrote: %s\n", utstring_body(dst_file));

    utstring_new(dst_file);
    utstring_printf(dst_file, "%s/WORKSPACE.bazel",
                    utstring_body(dst_dir));
    emit_workspace_file(dst_file, "stublibs");

    /* now BUILD.bazel */
    utstring_renew(dst_dir);
    utstring_printf(dst_dir, "%s/stublibs/lib/stublibs",
                    coswitch_lib);
    mkdir_r(utstring_body(dst_dir));

    utstring_renew(dst_file);
    utstring_printf(dst_file,
                    "%s/BUILD.bazel",
                    utstring_body(dst_dir));

    ostream = fopen(utstring_body(dst_file), "w");
    if (ostream == NULL) {
        log_error("fopen: %s: %s", strerror(errno),
                  utstring_body(dst_file));
        fprintf(stderr, "fopen: %s: %s", strerror(errno),
                utstring_body(dst_file));
        fprintf(stderr, "exiting\n");
        /* perror(utstring_body(dst_file)); */
        exit(EXIT_FAILURE);
    }
    fprintf(ostream, "# generated file - DO NOT EDIT\n\n");

    fprintf(ostream,
            "load(\"@bazel_skylib//rules:common_settings.bzl\",\n");
    fprintf(ostream, "     \"string_setting\")\n\n");

    fprintf(ostream,
            "package(default_visibility=[\"//visibility:public\"])\n\n");

    fprintf(ostream, "exports_files(glob([\"**\"]))\n\n");

    fprintf(ostream,
            "string_setting(name = \"path\",\n");
    fprintf(ostream,
            "               build_setting_default = \"%s/stublibs/lib/stublibs\")\n\n",
            coswitch_lib  // use switch_lib?
            );

    fprintf(ostream, "filegroup(\n");
    fprintf(ostream, "    name = \"stublibs\",\n");
    fprintf(ostream, "    srcs = glob([\"**\"]),\n");
    fprintf(ostream, ")\n");
    fclose(ostream);
    if (verbosity > log_writes) {
        fprintf(INFOFD, GRN "INFO" CRESET
                " wrote: %s\n", utstring_body(dst_file));
    }

    utstring_free(dst_file);

    _emit_lib_stublibs_symlinks(switch_stublibs, coswitch_lib);
    /* registry record for stublibs emitted by emit_ocaml_workspace */
    TRACE_EXIT;
}

void _emit_lib_stublibs_symlinks(char *switch_stublibs,
                                 char *coswitch_lib)
{
    TRACE_ENTRY;
    UT_string *dst_dir;
    utstring_new(dst_dir);
    /* utstring_printf(dst_dir, coswitch_pfx); */
    utstring_printf(dst_dir, "%s/stublibs/lib/stublibs",
                    coswitch_lib);
    mkdir_r(utstring_body(dst_dir));

    UT_string *src_dir; // relative to opam_switch_lib
    utstring_new(src_dir);
    utstring_printf(src_dir, "%s", switch_stublibs);
                    /* "%s%s", */
                    /* utstring_body(opam_switch_lib), */
                    /* "/stublibs"); // _dir); */

    /* LOG_DEBUG(0, "libstublibs src_dir: %s\n", utstring_body(src_dir)); */
    /* LOG_DEBUG(0, "libstublibs dst_dir: %s\n", utstring_body(dst_dir)); */

    UT_string *src;
    utstring_new(src);
    UT_string *dst;
    utstring_new(dst);
    int rc;

#if defined(PROFILE_fastbuild)
    LOG_DEBUG(0, "opening src_dir for read: %s",
              utstring_body(src_dir));
#endif
    DIR *srcd = opendir(utstring_body(src_dir));
    /* DIR *srcd = opendir(utstring_body(opamdir)); */
    if (srcd == NULL) {
        log_error("Unable to opendir for symlinking stublibs: %s",
                utstring_body(src_dir));
        fprintf(stderr, "Unable to opendir for symlinking stublibs: %s\n",
                utstring_body(src_dir));
        /* exit(EXIT_FAILURE); */
        /* this can happen if a related pkg is not installed */
        /* example, see topkg and topkg-care */
        return;
    /* } else { */
    /*     LOG_DEBUG(0, "XXXXXXXXXXXXXXXX"); */
    }

    struct dirent *direntry;
    while ((direntry = readdir(srcd)) != NULL) {
        //Condition to check regular file.
        /* if (coswitch_debug_symlinks) { */
        LOG_DEBUG(1, "stublib: %s, type %d",
                  direntry->d_name, direntry->d_type);
        /* } */
        if (strncmp(direntry->d_name, "WORKSPACE", 9) == 0) {
            continue;
        }

        if( (direntry->d_type==DT_REG)
            || (direntry->d_type==DT_LNK) ){
            utstring_renew(src);
            utstring_printf(src, "%s/%s",
                            utstring_body(src_dir),
                            direntry->d_name);
            utstring_renew(dst);
            utstring_printf(dst, "%s/%s",
                            utstring_body(dst_dir), direntry->d_name);
            LOG_DEBUG(1, "stublibs: symlinking %s to %s",
                      utstring_body(src), utstring_body(dst));

            rc = symlink(utstring_body(src),
                         utstring_body(dst));
            symlink_ct++;
            if (rc != 0) {
                switch (errno) {
                case EEXIST:
                    goto ignore;
                case ENOENT:
                    log_error("symlink ENOENT: %s", strerror(errno));
                    log_error("a component of '%s' does not name an existing file",  utstring_body(dst_dir));
                    fprintf(stderr, "symlink ENOENT: %s\n", strerror(errno));
                    fprintf(stderr, "A component of '%s' does not name an existing file.\n",  utstring_body(dst_dir));
                    break;
                default:
                    log_error("symlink err: %s", strerror(errno));
                    fprintf(stderr, "symlink err: %s", strerror(errno));
                }
                log_error("Exiting");
                fprintf(stderr, "Exiting\n");
                exit(EXIT_FAILURE);
            ignore:
                ;
            }
        }
    }
    closedir(srcd);
    TRACE_EXIT;
}

/* *********************************************** */
void emit_ocaml_platform_buildfiles(UT_string *templates,
                                    /* char *runfiles, */
                                    char *coswitch_lib)
{
    /* TRACE_ENTRY; */
    /* UT_string *templates; */
    /* utstring_new(templates); */
    /* utstring_printf(templates, "%s/new/templates", runfiles); */
    /* utstring_printf(templates, "%s/%s", */
    /*                 runfiles, "/new/templates"); */

    UT_string *src_file;
    utstring_new(src_file);
    UT_string *dst_file;
    utstring_new(dst_file);

    /* LOG_DEBUG(0, "CWD: %s", getcwd(NULL,0)); */

    utstring_printf(src_file, "%s/%s",
                    utstring_body(templates),
                    "platform/platform.BUILD");
                    /* "platform/BUILD.bazel"); */
    utstring_printf(dst_file,
                    "%s/ocaml/platform",
                    coswitch_lib);
    mkdir_r(utstring_body(dst_file));
    utstring_printf(dst_file, "/BUILD.bazel");
    LOG_DEBUG(2, "cp src: %s, dst: %s",
              utstring_body(src_file), utstring_body(dst_file));
    copy_buildfile(utstring_body(src_file), dst_file);

    /* **************** */
    utstring_renew(src_file);
    utstring_printf(src_file, "%s/%s",
                    utstring_body(templates),
                    "platform/arch.BUILD");
                    /* "platform/build/BUILD.bazel"); */

    utstring_renew(dst_file);
    utstring_printf(dst_file, "%s/ocaml/platform/arch", coswitch_lib);
    mkdir_r(utstring_body(dst_file));
    utstring_printf(dst_file, "/BUILD.bazel");
    LOG_DEBUG(2, "cp src: %s, dst: %s",
              utstring_body(src_file), utstring_body(dst_file));
    copy_buildfile(utstring_body(src_file), dst_file);

    /* **************** */
    utstring_renew(src_file);
    utstring_printf(src_file, "%s/%s",
                    utstring_body(templates),
                    "platform/executor.BUILD");

    utstring_renew(dst_file);
    utstring_printf(dst_file, "%s/ocaml/platform/executor", coswitch_lib);
    mkdir_r(utstring_body(dst_file));
    utstring_printf(dst_file, "/BUILD.bazel");
    LOG_DEBUG(2, "cp src: %s, dst: %s",
              utstring_body(src_file), utstring_body(dst_file));
    copy_buildfile(utstring_body(src_file), dst_file);

    /* **************** */
    utstring_renew(src_file);
    utstring_printf(src_file, "%s/%s",
                    utstring_body(templates),
                    "platform/emitter.BUILD");

    utstring_renew(dst_file);
    utstring_printf(dst_file, "%s/ocaml/platform/emitter", coswitch_lib);
    mkdir_r(utstring_body(dst_file));
    utstring_printf(dst_file, "/BUILD.bazel");
    LOG_DEBUG(2, "cp src: %s, dst: %s",
              utstring_body(src_file), utstring_body(dst_file));
    copy_buildfile(utstring_body(src_file), dst_file);

    utstring_free(templates);
    utstring_free(src_file);
    utstring_free(dst_file);
    TRACE_EXIT;
}

void emit_ocaml_toolchain_buildfiles(char *runfiles,
                                     char *coswitch_lib)
{
    TRACE_ENTRY;
    UT_string *templates;
    utstring_new(templates);
    utstring_printf(templates, "%s/%s",
                    runfiles, "/templates");

    UT_string *src_file;
    utstring_new(src_file);
    UT_string *dst_file;
    utstring_new(dst_file);

    utstring_printf(src_file, "%s/%s",
                    utstring_body(templates),
                    "toolchain/selectors/local.BUILD");
    utstring_printf(dst_file,
                    "%s/ocaml/toolchain/selectors/local",
                    coswitch_lib);
    mkdir_r(utstring_body(dst_file));
    utstring_printf(dst_file, "/BUILD.bazel");
    LOG_DEBUG(2, "cp src: %s, dst: %s",
              utstring_body(src_file), utstring_body(dst_file));
    copy_buildfile(utstring_body(src_file), dst_file);

    utstring_renew(src_file);
    utstring_printf(src_file, "%s/%s",
                    utstring_body(templates),
                    "toolchain/selectors/macos/x86_64.BUILD");
    utstring_renew(dst_file);
    utstring_printf(dst_file,
                    "%s/ocaml/toolchain/selectors/macos/x86_64",
                    coswitch_lib);
    mkdir_r(utstring_body(dst_file));
    utstring_printf(dst_file, "/BUILD.bazel");
    LOG_DEBUG(2, "cp src: %s, dst: %s",
              utstring_body(src_file), utstring_body(dst_file));
    copy_buildfile(utstring_body(src_file), dst_file);

    utstring_renew(src_file);
    utstring_printf(src_file, "%s/%s",
                    utstring_body(templates),
                    "toolchain/selectors/macos/arm.BUILD");
    utstring_new(dst_file);
    utstring_printf(dst_file,
                    "%s/ocaml/toolchain/selectors/macos/arm",
                    coswitch_lib);
    mkdir_r(utstring_body(dst_file));
    utstring_printf(dst_file, "/BUILD.bazel");
    LOG_DEBUG(2, "cp src: %s, dst: %s",
              utstring_body(src_file), utstring_body(dst_file));
    copy_buildfile(utstring_body(src_file), dst_file);

    utstring_renew(src_file);
    utstring_printf(src_file, "%s/%s",
                    utstring_body(templates),
                    "toolchain/selectors/linux/x86_64.BUILD");
    utstring_new(dst_file);
    utstring_printf(dst_file,
                    "%s/ocaml/toolchain/selectors/linux/x86_64",
                    coswitch_lib);
    mkdir_r(utstring_body(dst_file));
    utstring_printf(dst_file, "/BUILD.bazel");
    LOG_DEBUG(2, "cp src: %s, dst: %s",
              utstring_body(src_file), utstring_body(dst_file));
    copy_buildfile(utstring_body(src_file), dst_file);

    utstring_renew(src_file);
    utstring_printf(src_file, "%s/%s",
                    utstring_body(templates),
                    "toolchain/selectors/linux/arm.BUILD");
    utstring_new(dst_file);
    utstring_printf(dst_file,
                    "%s/ocaml/toolchain/selectors/linux/arm",
                    coswitch_lib);
    mkdir_r(utstring_body(dst_file));
    utstring_printf(dst_file, "/BUILD.bazel");
    LOG_DEBUG(2, "cp src: %s, dst: %s",
              utstring_body(src_file), utstring_body(dst_file));
    copy_buildfile(utstring_body(src_file), dst_file);

    /* toolchain options */
    utstring_renew(src_file);
    utstring_printf(src_file, "%s/%s",
                    utstring_body(templates),
                    "toolchain/profiles/profiles.BUILD");
    utstring_new(dst_file);
    utstring_printf(dst_file,
                    "%s/ocaml/toolchain/profiles",
                    coswitch_lib);
    mkdir_r(utstring_body(dst_file));
    utstring_printf(dst_file, "/BUILD.bazel");
    LOG_DEBUG(2, "cp src: %s, dst: %s",
              utstring_body(src_file), utstring_body(dst_file));
    copy_buildfile(utstring_body(src_file), dst_file);

    /* toolchain adapters */
    utstring_renew(src_file);
    utstring_printf(src_file, "%s/%s",
                    utstring_body(templates),
                    "toolchain/adapters/local.BUILD");
    utstring_new(dst_file);
    utstring_printf(dst_file,
                    "%s/ocaml/toolchain/adapters/local",
                    coswitch_lib);
    mkdir_r(utstring_body(dst_file));
    utstring_printf(dst_file, "/BUILD.bazel");
    LOG_DEBUG(2, "cp src: %s, dst: %s",
              utstring_body(src_file), utstring_body(dst_file));
    copy_buildfile(utstring_body(src_file), dst_file);
    //TODO: mustache support
    /* _process_mustache("toolchain/adapters/local.BUILD.mustache", dst_file); */

    utstring_renew(src_file);
    utstring_printf(src_file, "%s/%s",
                    utstring_body(templates),
                    "toolchain/adapters/linux/x86_64.BUILD");
    utstring_new(dst_file);
    utstring_printf(dst_file,
                    "%s/ocaml/toolchain/adapters/linux/x86_64",
                    coswitch_lib);
    mkdir_r(utstring_body(dst_file));
    utstring_printf(dst_file, "/BUILD.bazel");
    LOG_DEBUG(2, "cp src: %s, dst: %s",
              utstring_body(src_file), utstring_body(dst_file));
    copy_buildfile(utstring_body(src_file), dst_file);

    utstring_renew(src_file);
    utstring_printf(src_file, "%s/%s",
                    utstring_body(templates),
                    "toolchain/adapters/linux/arm.BUILD");
    utstring_new(dst_file);
    utstring_printf(dst_file,
                    "%s/ocaml/toolchain/adapters/linux/arm",
                    coswitch_lib);
    mkdir_r(utstring_body(dst_file));
    utstring_printf(dst_file, "/BUILD.bazel");
    LOG_DEBUG(2, "cp src: %s, dst: %s",
              utstring_body(src_file), utstring_body(dst_file));
    copy_buildfile(utstring_body(src_file), dst_file);

    utstring_renew(src_file);
    utstring_printf(src_file, "%s/%s",
                    utstring_body(templates),
                    "toolchain/adapters/macos/x86_64.BUILD");
    utstring_new(dst_file);
    utstring_printf(dst_file,
                    "%s/ocaml/toolchain/adapters/macos/x86_64",
                    coswitch_lib);
    mkdir_r(utstring_body(dst_file));
    utstring_printf(dst_file, "/BUILD.bazel");
    LOG_DEBUG(2, "cp src: %s, dst: %s",
              utstring_body(src_file), utstring_body(dst_file));
    copy_buildfile(utstring_body(src_file), dst_file);

    utstring_renew(src_file);
    utstring_printf(src_file, "%s/%s",
                    utstring_body(templates),
                    "toolchain/adapters/macos/arm.BUILD");
    utstring_new(dst_file);
    utstring_printf(dst_file,
                    "%s/ocaml/toolchain/adapters/macos/arm",
                    coswitch_lib);
    mkdir_r(utstring_body(dst_file));
    utstring_printf(dst_file, "/BUILD.bazel");
    LOG_DEBUG(2, "cp src: %s, dst: %s",
              utstring_body(src_file), utstring_body(dst_file));
    copy_buildfile(utstring_body(src_file), dst_file);

    utstring_free(templates);
    utstring_free(src_file);
    utstring_free(dst_file);
    TRACE_EXIT;
}

void emit_ocaml_bin_dir(char *switch_pfx, char *coswitch_lib)
{
    TRACE_ENTRY;
    UT_string *dst_file;
    utstring_new(dst_file);
    utstring_printf(dst_file, "%s/ocaml/bin", coswitch_lib);
    mkdir_r(utstring_body(dst_file));
    utstring_printf(dst_file, "/BUILD.bazel");

    FILE *ostream;
    ostream = fopen(utstring_body(dst_file), "w");
    if (ostream == NULL) {
        log_error("fopen: %s: %s", strerror(errno),
                  utstring_body(dst_file));
        fprintf(stderr, "fopen: %s: %s", strerror(errno),
                utstring_body(dst_file));
        fprintf(stderr, "exiting\n");
        /* perror(utstring_body(dst_file)); */
        exit(EXIT_FAILURE);
    }
    fprintf(ostream, "# generated file - DO NOT EDIT\n");
    fprintf(ostream, "exports_files(glob([\"**\"]))\n");
    fclose(ostream);
    if (verbosity > log_writes)
        fprintf(INFOFD, GRN "INFO" CRESET " wrote %s\n", utstring_body(dst_file));
    utstring_free(dst_file);

    /* **************************************************************** */
    _emit_ocaml_bin_symlinks(switch_pfx, coswitch_lib);
    TRACE_EXIT;
}

void _emit_ocaml_bin_symlinks(char *opam_switch_pfx,
                              char *coswitch_lib // dest
                              )
{
    TRACE_ENTRY;
    UT_string *dst_dir;
    utstring_new(dst_dir);
    /* utstring_concat(dst_dir, coswitch_lib); // pfx); */
    utstring_printf(dst_dir, "%s/ocaml/bin", coswitch_lib);

    /* UT_string *src_dir; // relative to opam_switch_lib */
    /* utstring_new(src_dir); */
    /* utstring_printf(src_dir, "bin"); */

    mkdir_r(utstring_body(dst_dir));

/*     LOG_DEBUG(0, "opam pfx: %s", opam_switch_pfx); */
/*         LOG_DEBUG(0, "dst_dir: %s", utstring_body(dst_dir)); */

    UT_string *src;
    utstring_new(src);
    UT_string *dst;
    utstring_new(dst);
    int rc;

    UT_string *opam_switch_bin;
    utstring_new(opam_switch_bin);
    utstring_printf(opam_switch_bin, "%s/bin", opam_switch_pfx);
#if defined(PROFILE_fastbuild)
    if (verbosity > 3)
        LOG_DEBUG(0, "opening src_dir for read: %s",
                  utstring_body(opam_switch_bin));
#endif
    DIR *srcd = opendir(utstring_body(opam_switch_bin));
    /* DIR *srcd = opendir(utstring_body(opamdir)); */
    if (dst == NULL) {
        fprintf(stderr, "Unable to opendir for symlinking toolchain: %s\n",
                utstring_body(opam_switch_bin));
        /* exit(EXIT_FAILURE); */
        /* this can happen if a related pkg is not installed */
        /* example, see topkg and topkg-care */
        return;
    }

    struct dirent *direntry;
    while ((direntry = readdir(srcd)) != NULL) {
        //Condition to check regular file.
        if( (direntry->d_type==DT_REG)
            || (direntry->d_type==DT_LNK) ){

            utstring_renew(src);
            utstring_printf(src, "%s/%s",
                            utstring_body(opam_switch_bin),
                            direntry->d_name);
            utstring_renew(dst);
            utstring_printf(dst, "%s/%s",
                            utstring_body(dst_dir), direntry->d_name);

            if (verbosity > log_symlinks)
                fprintf(INFOFD, GRN "INFO" CRESET
                        " symlink: %s -> %s\n",
                          //direntry->d_name);
                          utstring_body(src),
                          utstring_body(dst));

            rc = symlink(utstring_body(src),
                         utstring_body(dst));
            symlink_ct++;
            if (rc != 0) {
                switch (errno) {
                case EEXIST:
                    goto ignore;
                case ENOENT:
                    log_error("symlink ENOENT: %s", strerror(errno));
                    log_error("a component of '%s' does not name an existing file",  utstring_body(dst_dir));
                    fprintf(stderr, "symlink ENOENT: %s\n", strerror(errno));
                    fprintf(stderr, "A component of '%s' does not name an existing file.\n",  utstring_body(dst_dir));
                    break;
                default:
                    log_error("symlink err: %s", strerror(errno));
                    fprintf(stderr, "symlink err: %s", strerror(errno));
                }
                log_error("Exiting");
                fprintf(stderr, "Exiting\n");
                exit(EXIT_FAILURE);
            ignore:
                ;
            }
        }
    }
    closedir(srcd);
    utstring_free(opam_switch_bin);
    TRACE_EXIT;
}

/* **************************************************************** */
/* obsolete but keep it around in case we decide to use it later */
/* void _symlink_buildfile(char *buildfile, UT_string *to_file) */
/* { */
/*     TRACE_ENTRY; */
/*     UT_string *src; */
/*     utstring_new(src); */
/*     utstring_printf(src, */
/*                     "%s/external/%s/coswitch/templates/%s", */
/*                     utstring_body(coswitch_runfiles_root), */
/*                     ws_name, */
/*                     buildfile); */
/*     int rc = access(utstring_body(src), F_OK); */
/*     if (rc != 0) { */
/*         log_error("not found: %s", utstring_body(src)); */
/*         fprintf(stderr, "not found: %s\n", utstring_body(src)); */
/*         return; */
/*     } */

/* #if defined(PROFILE_fastbuild) */
/*     if (coswitch_debug) { */
/*         LOG_DEBUG(0, "c_libs: symlinking %s to %s\n", */
/*                   utstring_body(src), */
/*                   utstring_body(to_file)); */
/*     } */
/* #endif */
/*     errno = 0; */
/*     rc = symlink(utstring_body(src), */
/*                  utstring_body(to_file)); */
/*     symlink_ct++; */
/*     if (rc != 0) { */
/*         switch (errno) { */
/*         case EEXIST: */
/*             goto ignore; */
/*         case ENOENT: */
/*             log_error("symlink ENOENT: %s", strerror(errno)); */
/*             /\* log_error("a component of '%s' does not name an existing file",  utstring_body(dst_dir)); *\/ */
/*             fprintf(stderr, "symlink ENOENT: %s\n", strerror(errno)); */
/*             /\* fprintf(stderr, "A component of '%s' does not name an existing file.\n",  utstring_body(dst_dir)); *\/ */
/*             break; */
/*         default: */
/*             log_error("symlink err: %s", strerror(errno)); */
/*             fprintf(stderr, "symlink err: %s", strerror(errno)); */
/*         } */
/*         log_error("Exiting"); */
/*         fprintf(stderr, "Exiting\n"); */
/*         exit(EXIT_FAILURE); */
/*     ignore: */
/*         ; */
/*     } */
/*     TRACE_EXIT; */
/* } */

/* ************************************************ */
/*
  pre-v5: <switch-pfx>/lib/bigarray, redirects to lib/ocaml
  v5+: no bigarray subdir anywhere
 */
void emit_ocaml_bigarray_pkg(char *runfiles,
                             char *switch_lib,
                             char *coswitch_lib)
{
    TRACE_ENTRY;


    UT_string *bigarray_dir;
    utstring_new(bigarray_dir);
    utstring_printf(bigarray_dir,
                    "%s/bigarray",
                    switch_lib);
    int rc = access(utstring_body(bigarray_dir), F_OK);
    if (rc != 0) {
#if defined(PROFILE_fastbuild)
        /* if (coswitch_trace) */
        LOG_WARN(0, YEL "NOT FOUND: %s" CRESET,
                 utstring_body(bigarray_dir));
#endif
        utstring_free(bigarray_dir);
        // v >= 5.0.0 does not include any bigarray archive
        return;
/* #if defined(PROFILE_fastbuild) */
/*     } else { */
/*         LOG_WARN(0, YEL "FOUND: %s" CRESET, utstring_body(bigarray_dir)); */
/* #endif */
    }

    UT_string *templates;
    utstring_new(templates);
    utstring_printf(templates, "%s/%s",
                    runfiles, "/templates/ocaml");

    UT_string *src_file;
    UT_string *dst_dir;
    UT_string *dst_file;
    utstring_new(src_file);
    utstring_new(dst_dir);
    utstring_new(dst_file);

    utstring_printf(src_file, "%s/%s",
                    utstring_body(templates),
                    "ocaml_bigarray.BUILD");

    utstring_printf(dst_dir,
                    "%s/ocaml/lib/bigarray",
                    coswitch_lib);
    mkdir_r(utstring_body(dst_dir));
    utstring_printf(dst_file, "%s/BUILD.bazel",
                    utstring_body(dst_dir));
    LOG_DEBUG(2, "cp src: %s, dst: %s",
              utstring_body(src_file), utstring_body(dst_file));
    copy_buildfile(utstring_body(src_file), dst_file);

    // if we found <switch>/lib/bigarray,
    // then the files will be in <switch>/lib/ocaml
    // (not <switch>/lib/ocaml/bigarray)
    utstring_renew(bigarray_dir);
    utstring_printf(bigarray_dir,
                    "%s/ocaml",
                    switch_lib);
    _symlink_ocaml_bigarray(bigarray_dir, utstring_body(dst_dir))
;

    utstring_free(src_file);
    utstring_free(dst_file);
    utstring_free(templates);
    utstring_free(bigarray_dir);
    TRACE_EXIT;
}

void _symlink_ocaml_bigarray(UT_string *bigarray_dir,
                             char *tgtdir)
{
    TRACE_ENTRY;
/* #if defined(PROFILE_fastbuild) */
/*     /\* if (coswitch_debug_symlinks) *\/ */
/*     if (verbosity > 2) { */
/*         LOG_DEBUG(0, "src: %s, dst: %s", */
/*                   utstring_body(bigarray_dir), tgtdir); */
/*     } */
/* #endif */

    /* UT_string *opamdir; */
    /* utstring_new(opamdir); */
    /* utstring_printf(opamdir, "%s/ocaml", switch_lib); */
    /*                 /\* utstring_body(opam_switch_lib)); *\/ */

    UT_string *src;
    utstring_new(src);
    UT_string *dst;
    utstring_new(dst);
    int rc;

    DIR *d = opendir(utstring_body(bigarray_dir));
    if (d == NULL) {
        perror(utstring_body(bigarray_dir));
        log_error(RED "Unable to opendir for symlinking bigarray" CRESET " %s\n",
                utstring_body(bigarray_dir));
        /* exit(EXIT_FAILURE); */
        /* this can happen if a related pkg is not installed */
        /* example, see topkg and topkg-care */
        return;
    }

    struct dirent *direntry;
    while ((direntry = readdir(d)) != NULL) {
        if(direntry->d_type==DT_REG){
            /* link files starting with "bigarray" */
            if (strncmp("bigarray", direntry->d_name, 8) != 0)
                continue;

            utstring_renew(src);
            utstring_printf(src, "%s/%s",
                            utstring_body(bigarray_dir),
                            direntry->d_name);
            utstring_renew(dst);
            utstring_printf(dst, "%s/%s",
                            tgtdir, direntry->d_name);
            /* LOG_DEBUG(0, "symlinking %s to %s", */
            /*        utstring_body(src), */
            /*        utstring_body(dst)); */
            errno = 0;
            rc = symlink(utstring_body(src),
                         utstring_body(dst));
            symlink_ct++;
            if (rc != 0) {
                if (errno != EEXIST) {
                    /* perror(utstring_body(src)); */
                    log_error("ERROR: %s", strerror(errno));
                    log_error("src: %s, dst: %s",
                              utstring_body(src),
                              utstring_body(dst));
                    log_error("exiting");
                    exit(EXIT_FAILURE);
                }
            }
        }
    }
    closedir(d);
    TRACE_EXIT;
}

/*
  for <switch>/lib/compiler-libs.
  all targets aliased to <switch>/lib/ocaml/compiler-libs
 */
void emit_compiler_libs_pkg(char *runfiles, char *coswitch_lib)
{
    TRACE_ENTRY;
    UT_string *templates;
    utstring_new(templates);
    utstring_printf(templates, "%s/%s",
                    runfiles, "/templates/ocaml");

    UT_string *src_file;
    utstring_new(src_file);
    UT_string *dst_file;
    utstring_new(dst_file);

    utstring_printf(src_file, "%s/%s",
                    utstring_body(templates),
                    "compiler_libs/common.BUILD");
    utstring_printf(dst_file,
                    "%s/compiler-libs/lib/compiler-libs",
                    coswitch_lib);
    mkdir_r(utstring_body(dst_file));
    utstring_printf(dst_file, "/BUILD.bazel");
    LOG_DEBUG(2, "cp src: %s, dst: %s",
              utstring_body(src_file), utstring_body(dst_file));
    copy_buildfile(utstring_body(src_file), dst_file);

    utstring_renew(src_file);
    utstring_printf(src_file, "%s/%s",
                    utstring_body(templates),
                    "compiler_libs/common.BUILD");
    utstring_renew(dst_file);
    utstring_printf(dst_file,
                    "%s/compiler-libs/lib/common",
                    coswitch_lib);
    mkdir_r(utstring_body(dst_file));
    utstring_printf(dst_file, "/BUILD.bazel");
    LOG_DEBUG(2, "cp src: %s, dst: %s",
              utstring_body(src_file), utstring_body(dst_file));
    copy_buildfile(utstring_body(src_file), dst_file);

    utstring_renew(src_file);
    utstring_printf(src_file, "%s/%s",
                    utstring_body(templates),
                    "compiler_libs/bytecomp.BUILD");
    utstring_renew(dst_file);
    utstring_printf(dst_file, "%s/compiler-libs/lib/bytecomp",
                    coswitch_lib);
    mkdir_r(utstring_body(dst_file));
    utstring_printf(dst_file, "/BUILD.bazel");
    LOG_DEBUG(2, "cp src: %s, dst: %s",
              utstring_body(src_file), utstring_body(dst_file));
    copy_buildfile(utstring_body(src_file), dst_file);

    utstring_renew(src_file);
    utstring_printf(src_file, "%s/%s",
                    utstring_body(templates),
                    "compiler_libs/optcomp.BUILD");
    utstring_renew(dst_file);
    utstring_printf(dst_file, "%s/compiler-libs/lib/optcomp",
                    coswitch_lib);
    mkdir_r(utstring_body(dst_file));
    utstring_printf(dst_file, "/BUILD.bazel");
    LOG_DEBUG(2, "cp src: %s, dst: %s",
              utstring_body(src_file), utstring_body(dst_file));
    copy_buildfile(utstring_body(src_file), dst_file);

    utstring_renew(src_file);
    utstring_printf(src_file, "%s/%s",
                    utstring_body(templates),
                    "compiler_libs/toplevel.BUILD");
    utstring_renew(dst_file);
    utstring_printf(dst_file, "%s/compiler-libs/lib/toplevel",
                    coswitch_lib);
    mkdir_r(utstring_body(dst_file));
    utstring_printf(dst_file, "/BUILD.bazel");
    LOG_DEBUG(2, "cp src: %s, dst: %s",
              utstring_body(src_file), utstring_body(dst_file));
    copy_buildfile(utstring_body(src_file), dst_file);

    utstring_free(src_file);
    utstring_free(dst_file);
    utstring_free(templates);
    TRACE_EXIT;
}

/* **************************************************************** */
/* for <switch>/lib/ocaml/compiler-libs */
void emit_ocaml_compiler_libs_pkg(char *runfiles,
                                  char *switch_lib,
                                  char *coswitch_lib)
                                  /* char *tgtdir) */
/* (char *runfiles, */
/*  char *coswitch_lib) */
{
    TRACE_ENTRY;
    UT_string *templates;
    utstring_new(templates);
    utstring_printf(templates, "%s/%s",
                    runfiles, "templates/ocaml");

    UT_string *src_file;
    UT_string *dst_file;
    utstring_new(src_file);
    utstring_new(dst_file);

    utstring_printf(src_file, "%s/%s",
                    utstring_body(templates),
                    "ocaml_compiler-libs.BUILD");
    utstring_printf(dst_file,
                    "%s/ocaml/lib/compiler-libs",
                    coswitch_lib);
    mkdir_r(utstring_body(dst_file));
    utstring_printf(dst_file, "/BUILD.bazel");
    /* LOG_DEBUG(2, "cp src: %s, dst: %s", */
    /*           utstring_body(src_file), */
    /*           utstring_body(dst_file)); */
    LOG_DEBUG(2, "cp src: %s, dst: %s",
              utstring_body(src_file), utstring_body(dst_file));
    copy_buildfile(utstring_body(src_file), dst_file);

    utstring_renew(src_file);
    utstring_printf(src_file, "%s/%s",
                    utstring_body(templates),
                    "compiler_libs/common.BUILD");
    utstring_renew(dst_file);
    utstring_printf(dst_file,
                    "%s/ocaml/lib/compiler-libs/common",
                    coswitch_lib);
    mkdir_r(utstring_body(dst_file));
    utstring_printf(dst_file, "/BUILD.bazel");
    LOG_DEBUG(2, "cp src: %s, dst: %s",
              utstring_body(src_file), utstring_body(dst_file));
    copy_buildfile(utstring_body(src_file), dst_file);

    utstring_renew(src_file);
    utstring_printf(src_file, "%s/%s",
                    utstring_body(templates),
                    "compiler_libs/bytecomp.BUILD");
    utstring_renew(dst_file);
    utstring_printf(dst_file,
                    "%s/ocaml/lib/compiler-libs/bytecomp",
                    coswitch_lib);
    mkdir_r(utstring_body(dst_file));
    utstring_printf(dst_file, "/BUILD.bazel");
    LOG_DEBUG(2, "cp src: %s, dst: %s",
              utstring_body(src_file), utstring_body(dst_file));
    copy_buildfile(utstring_body(src_file), dst_file);

    utstring_renew(src_file);
    utstring_printf(src_file, "%s/%s",
                    utstring_body(templates),
                    "compiler_libs/optcomp.BUILD");
    utstring_renew(dst_file);
    utstring_printf(dst_file,
                    "%s/ocaml/lib/compiler-libs/optcomp",
                    coswitch_lib);
    mkdir_r(utstring_body(dst_file));
    utstring_printf(dst_file, "/BUILD.bazel");
    LOG_DEBUG(2, "cp src: %s, dst: %s",
              utstring_body(src_file), utstring_body(dst_file));
    copy_buildfile(utstring_body(src_file), dst_file);

    utstring_renew(src_file);
    utstring_printf(src_file, "%s/%s",
                    utstring_body(templates),
                    "compiler_libs/toplevel.BUILD");
    utstring_renew(dst_file);
    utstring_printf(dst_file, "%s/ocaml/lib/compiler-libs/toplevel",
                    coswitch_lib);
    mkdir_r(utstring_body(dst_file));
    utstring_printf(dst_file, "/BUILD.bazel");
    LOG_DEBUG(2, "cp src: %s, dst: %s",
              utstring_body(src_file), utstring_body(dst_file));
    copy_buildfile(utstring_body(src_file), dst_file);

    utstring_renew(dst_file);
    utstring_printf(dst_file, "%s/ocaml/lib/compiler-libs",
                    coswitch_lib);
    _symlink_ocaml_compiler_libs(switch_lib,
                                 /* coswitch_lib); */
                                 utstring_body(dst_file));

    utstring_free(src_file);
    utstring_free(dst_file);
    utstring_free(templates);
    TRACE_EXIT;
}

void _symlink_ocaml_compiler_libs(char *switch_lib,
                                  char *coswitch_lib)
                                  /* char *tgtdir) */
{
    TRACE_ENTRY;
#if defined(PROFILE_fastbuild)
    /* LOG_DEBUG(0, "src: %s, dst: %s, tgt: %s", */
    /*           switch_lib, coswitch_lib); //, tgtdir); */
#endif
    UT_string *opamdir;
    utstring_new(opamdir);
    utstring_printf(opamdir, "%s/ocaml/compiler-libs",
                    switch_lib);
                    /* utstring_body(opam_switch_lib)); */
    /* LOG_DEBUG(0, "OPAM DIR src: %s", utstring_body(opamdir)); */
    UT_string *src;
    utstring_new(src);
    UT_string *dst;
    utstring_new(dst);
    int rc;

    DIR *d = opendir(utstring_body(opamdir));
    if (d == NULL) {
        fprintf(stderr, "Unable to opendir for symlinking compiler-libs: %s\n",
                utstring_body(opamdir));
        /* exit(EXIT_FAILURE); */
        /* this can happen if a related pkg is not installed */
        /* example, see topkg and topkg-care */
        return;
    }

    struct dirent *direntry;
    while ((direntry = readdir(d)) != NULL) {
        if(direntry->d_type==DT_REG){

            utstring_renew(src);
            utstring_printf(src, "%s/%s",
                            utstring_body(opamdir), direntry->d_name);

            utstring_renew(dst);
            utstring_printf(dst, "%s/%s",
                            //tgtdir,
                            coswitch_lib,
                            direntry->d_name);
            /* LOG_DEBUG(0, "symlinking %s to %s", */
            /*           utstring_body(src), */
            /*           utstring_body(dst)); */
            rc = symlink(utstring_body(src),
                         utstring_body(dst));
            symlink_ct++;
            if (rc != 0) {
                if (errno != EEXIST) {
                    perror(utstring_body(src));
                    log_error("exiting");
                    exit(EXIT_FAILURE);
                }
            }
        }
    }
    closedir(d);
    TRACE_EXIT;
}

/* ***************************************** */
void emit_ocaml_dynlink_pkg(char *runfiles,
                            char *switch_lib,
                            char *coswitch_lib)
{
    TRACE_ENTRY;
    bool add_toplevel = false;

    UT_string *dynlink_dir;
    utstring_new(dynlink_dir);
    utstring_printf(dynlink_dir,
                    "%s/dynlink",
                    switch_lib);
    int rc = access(utstring_body(dynlink_dir), F_OK);
    if (rc != 0) {
#if defined(PROFILE_fastbuild)
        /* if (coswitch_trace) */
        LOG_WARN(0, YEL "NOT FOUND: %s" CRESET,
                 utstring_body(dynlink_dir));
#endif
        utstring_new(dynlink_dir);
        utstring_printf(dynlink_dir,
                        "%s/ocaml/dynlink",
                        switch_lib);
        int rc = access(utstring_body(dynlink_dir), F_OK);
        if (rc != 0) {
#if defined(PROFILE_fastbuild)
            /* if (coswitch_trace) */
            LOG_WARN(0, YEL "NOT FOUND: %s" CRESET,
                     utstring_body(dynlink_dir));
#endif
            utstring_free(dynlink_dir);
            return;
        } else {
#if defined(PROFILE_fastbuild)
            LOG_WARN(0, YEL "FOUND: %s" CRESET,
                     utstring_body(dynlink_dir));
#endif
            add_toplevel = true;
        }
/* #if defined(PROFILE_fastbuild) */
/*     } else { */
/*         LOG_WARN(0, YEL "FOUND: %s" CRESET, utstring_body(dynlink_dir)); */
/* #endif */
    }

    UT_string *templates;
    utstring_new(templates);
    utstring_printf(templates, "%s/%s",
                    runfiles, "templates/ocaml");

    UT_string *src_file;
    UT_string *dst_dir;
    UT_string *dst_file;
    utstring_new(src_file);
    utstring_new(dst_dir);
    utstring_new(dst_file);

    // 5.0.0+ has <switch>/lib/ocaml/dynlink/META
    // pre-5.0.0: no <switch>/lib/ocaml/dynlink
    // always emit <coswitch>/lib/ocaml/dynlink
    utstring_printf(src_file, "%s/%s",
                    utstring_body(templates),
                    "ocaml_dynlink.BUILD");
    utstring_printf(dst_dir,
                    "%s/ocaml/lib/dynlink",
                    coswitch_lib);
    mkdir_r(utstring_body(dst_dir));
    utstring_printf(dst_file, "%s/BUILD.bazel",
                    utstring_body(dst_dir));
    LOG_DEBUG(2, "cp src: %s, dst: %s",
              utstring_body(src_file), utstring_body(dst_file));
    copy_buildfile(utstring_body(src_file), dst_file);

    if (add_toplevel) {
        // not found: <switch>/lib/dynlink
        // means >= 5.0.0
        // add <coswitch>/lib/dynlink for (bazel) compatibility
        _symlink_ocaml_dynlink(dynlink_dir,
                               utstring_body(dst_dir));
        _emit_toplevel(templates, "ocaml_dynlink_alias.BUILD",
                       src_file,
                       dst_dir, dst_file,
                       coswitch_lib, "dynlink");
    } else {
        // if we found <switch>/lib/dynlink (versions < 5.0.0)
        // then we need to populate <coswitch>/lib/ocaml/dynlink
        // (its already there for >= 5.0.0)
        utstring_new(dynlink_dir);
        utstring_printf(dynlink_dir,
                        "%s/ocaml",
                        switch_lib);
        _symlink_ocaml_dynlink(dynlink_dir,
                               utstring_body(dst_dir));
    }

    utstring_free(src_file);
    utstring_free(dst_file);
    utstring_free(templates);
    utstring_free(dynlink_dir);
    TRACE_EXIT;
}

void _symlink_ocaml_dynlink(UT_string *dynlink_dir, char *tgtdir)
{
    TRACE_ENTRY;
#if defined(PROFILE_fastbuild)
    if (verbosity > 2) {
        LOG_DEBUG(0, "src: %s, dst: %s",
                  utstring_body(dynlink_dir), tgtdir);
    }
#endif
    UT_string *src;
    utstring_new(src);
    UT_string *dst;
    utstring_new(dst);
    int rc;

    DIR *d = opendir(utstring_body(dynlink_dir));
    if (d == NULL) {
        fprintf(stderr, "Unable to opendir for symlinking dynlink: %s\n",
                utstring_body(dynlink_dir));
        /* exit(EXIT_FAILURE); */
        /* this can happen if a related pkg is not installed */
        /* example, see topkg and topkg-care */
        return;
    /* } else { */
    /*     if (verbosity > 1) */
    /*         LOG_DEBUG(0, "opened dir %s", utstring_body(dynlink_dir)); */
    }

    struct dirent *direntry;
    while ((direntry = readdir(d)) != NULL) {
        if(direntry->d_type==DT_REG){
            /* link files starting with "dynlink" */
            if (strncmp("dynlink", direntry->d_name, 7) != 0)
                continue;

            utstring_renew(src);
            utstring_printf(src, "%s/%s",
                            utstring_body(dynlink_dir),
                            direntry->d_name);
            utstring_renew(dst);
            utstring_printf(dst, "%s/%s",
                            tgtdir, direntry->d_name);
            /* LOG_DEBUG(0, "symlinking %s to %s", */
            /*           utstring_body(src), */
            /*           utstring_body(dst)); */
            errno = 0;
            rc = symlink(utstring_body(src),
                         utstring_body(dst));
            symlink_ct++;
            if (rc != 0) {
                if (errno != EEXIST) {
                    log_error("ERROR: %s", strerror(errno));
                    log_error("src: %s, dst: %s",
                              utstring_body(src),
                              utstring_body(dst));
                    log_error("exiting");
                    exit(EXIT_FAILURE);
                }
            }
        }
    }
    closedir(d);
    TRACE_EXIT;
}

/* ***************************************** */
void emit_ocaml_num_pkg(char *runfiles,
                        char *switch_lib,
                        char *coswitch_lib)
{ /* only if opam 'nums' pseudo-pkg was installed */
    TRACE_ENTRY;

    UT_string *num_dir;
    utstring_new(num_dir);
    utstring_printf(num_dir,
                    "%s/num",
                    switch_lib);
    int rc = access(utstring_body(num_dir), F_OK);
    if (rc != 0) {
#if defined(PROFILE_fastbuild)
        /* if (coswitch_trace) */
        LOG_WARN(0, YEL "NOT FOUND: %s" CRESET,
                 utstring_body(num_dir));
#endif
        return;
/* #if defined(PROFILE_fastbuild) */
/*     } else { */
/*         LOG_WARN(0, YEL "FOUND: %s" CRESET, utstring_body(num_dir)); */
/* #endif */
    }

    UT_string *dst_file;
/*     utstring_new(dst_file); */
/*     utstring_printf(dst_file, "%s/num/META", switch_lib); */
/*     rc = access(utstring_body(dst_file), F_OK); */
/*     if (rc != 0) { */
/* #if defined(PROFILE_fastbuild) */
/*         if (coswitch_trace) log_trace(YEL "num pkg not installed" CRESET); */
/* #endif */
/*         return; */
/*     } */

    utstring_new(dst_file);
    /* utstring_concat(dst_file, coswitch_pfx); */
    utstring_printf(dst_file, "%s/ocaml/num/core", coswitch_lib);
    mkdir_r(utstring_body(dst_file));
    _symlink_ocaml_num(switch_lib, utstring_body(dst_file));

    UT_string *templates;
    utstring_new(templates);
    utstring_printf(templates, "%s/%s",
                    runfiles, "/templates/ocaml");

    UT_string *src_file;
    utstring_new(src_file);
    utstring_printf(src_file, "%s/%s",
                    utstring_body(templates),
                    "ocaml_num.BUILD");
    utstring_printf(dst_file, "/BUILD.bazel");

    LOG_DEBUG(2, "cp src: %s, dst: %s",
              utstring_body(src_file), utstring_body(dst_file));
    copy_buildfile(utstring_body(src_file), dst_file);

    utstring_free(src_file);
    utstring_free(dst_file);
    utstring_free(templates);
    TRACE_EXIT;
}

void _symlink_ocaml_num(char *switch_lib, char *tgtdir)
{
    TRACE_ENTRY;

    UT_string *opamdir;
    utstring_new(opamdir);
    utstring_printf(opamdir, "%s/ocaml", switch_lib);
    /* utstring_body(opam_switch_lib)); */

    UT_string *src;
    utstring_new(src);
    UT_string *dst;
    utstring_new(dst);
    int rc;

    DIR *d = opendir(utstring_body(opamdir));
    if (d == NULL) {
        perror(utstring_body(opamdir));
        log_error("Unable to opendir for symlinking num: %s\n",
                  utstring_body(opamdir));
        /* exit(EXIT_FAILURE); */
        /* this can happen if a related pkg is not installed */
        /* example, see topkg and topkg-care */
        return;
    }
    LOG_DEBUG(0, "reading num dir %s", utstring_body(opamdir));

    struct dirent *direntry;
    while ((direntry = readdir(d)) != NULL) {
        if(direntry->d_type==DT_REG){
            /* if (strcasestr(direntry->d_name, "num") == NULL) */
            if (strncmp("nums.", direntry->d_name, 5) != 0)
                if (strncmp("libnums.", direntry->d_name, 8) != 0)
                    continue;

            utstring_renew(src);
            utstring_printf(src, "%s/%s",
                            utstring_body(opamdir), direntry->d_name);
            utstring_renew(dst);
            utstring_printf(dst, "%s/%s",
                            tgtdir, direntry->d_name);
#if defined(PROFILE_fastbuild)
            /* if (coswitch_debug_symlinks) */
                /* LOG_DEBUG(0, "symlinking %s to %s\n", */
                /*           utstring_body(src), */
                /*           utstring_body(dst)); */
#endif
            rc = symlink(utstring_body(src),
                         utstring_body(dst));
            symlink_ct++;
            if (rc != 0) {
                if (errno != EEXIST) {
                    perror(utstring_body(src));
                    fprintf(stderr, "exiting\n");
                    exit(EXIT_FAILURE);
                }
            }
        }
    }
    closedir(d);
    TRACE_EXIT;
}

/* ***************************************** */
void emit_ocaml_rtevents_pkg(char *runfiles,
                            char *switch_lib,
                            char *coswitch_lib)
{
    TRACE_ENTRY;
    bool add_toplevel = false;

    UT_string *rtevents_dir;
    utstring_new(rtevents_dir);
    utstring_printf(rtevents_dir,
                    "%s/runtime_events",
                    switch_lib);
    int rc = access(utstring_body(rtevents_dir), F_OK);
    if (rc != 0) {
#if defined(PROFILE_fastbuild)
        /* if (coswitch_trace) */
        LOG_WARN(0, YEL "NOT FOUND: %s" CRESET,
                 utstring_body(rtevents_dir));
#endif
        utstring_new(rtevents_dir);
        utstring_printf(rtevents_dir,
                        "%s/ocaml/runtime_events",
                        switch_lib);
        int rc = access(utstring_body(rtevents_dir), F_OK);
        if (rc != 0) {
#if defined(PROFILE_fastbuild)
            /* if (coswitch_trace) */
            LOG_WARN(0, YEL "NOT FOUND: %s" CRESET,
                     utstring_body(rtevents_dir));
#endif
            utstring_free(rtevents_dir);
            return;
        } else {
#if defined(PROFILE_fastbuild)
            LOG_WARN(0, YEL "FOUND: %s" CRESET, utstring_body(rtevents_dir));
#endif
            add_toplevel = true;
        }
/* #if defined(PROFILE_fastbuild) */
/*     } else { */
/*         LOG_WARN(0, YEL "FOUND: %s" CRESET, utstring_body(rtevents_dir)); */
/* #endif */
    }

    UT_string *templates;
    utstring_new(templates);
    utstring_printf(templates, "%s/%s",
                    runfiles, "templates/ocaml");

    UT_string *src_file;
    UT_string *dst_dir;
    UT_string *dst_file;
    utstring_new(src_file);
    utstring_new(dst_dir);
    utstring_new(dst_file);

    // 5.0.0+ has <switch>/lib/ocaml/rtevents/META
    // pre-5.0.0: no <switch>/lib/ocaml/rtevents
    // always emit <coswitch>/lib/ocaml/rtevents
    utstring_printf(src_file, "%s/%s",
                    utstring_body(templates),
                    "ocaml_rtevents.BUILD");
    utstring_printf(dst_dir,
                    "%s/ocaml/lib/runtime_events",
                    coswitch_lib);
    mkdir_r(utstring_body(dst_dir));
    utstring_printf(dst_file, "%s/BUILD.bazel",
                    utstring_body(dst_dir));
    LOG_DEBUG(2, "cp src: %s, dst: %s",
              utstring_body(src_file), utstring_body(dst_file));
    copy_buildfile(utstring_body(src_file), dst_file);

    if (add_toplevel) {
        // not found: <switch>/lib/rtevents
        // means >= 5.0.0
        // add <coswitch>/lib/rtevents for (bazel) compatibility
        _symlink_ocaml_rtevents(rtevents_dir,
                               utstring_body(dst_dir));
        _emit_toplevel(templates, "ocaml_rtevents_alias.BUILD",
                       src_file,
                       dst_dir, dst_file,
                       coswitch_lib, "runtime_events");
    } else {
        // if we found <switch>/lib/rtevents (versions < 5.0.0)
        // then we need to populate <coswitch>/lib/ocaml/rtevents
        // (its already there for >= 5.0.0)
        utstring_new(rtevents_dir);
        utstring_printf(rtevents_dir,
                        "%s/ocaml",
                        switch_lib);
        _symlink_ocaml_rtevents(rtevents_dir,
                               utstring_body(dst_dir));
    }

    utstring_free(src_file);
    utstring_free(dst_file);
    utstring_free(templates);
    utstring_free(rtevents_dir);
    TRACE_EXIT;
}

void _symlink_ocaml_rtevents(UT_string *rtevents_dir, char *tgtdir)
{
    TRACE_ENTRY;
#if defined(PROFILE_fastbuild)
    if (verbosity > 2) {
        LOG_DEBUG(0, "src: %s, dst: %s",
                  utstring_body(rtevents_dir), tgtdir);
    }
#endif
    UT_string *src;
    utstring_new(src);
    UT_string *dst;
    utstring_new(dst);
    int rc;

    DIR *d = opendir(utstring_body(rtevents_dir));
    if (d == NULL) {
        fprintf(stderr, "Unable to opendir for symlinking rtevents: %s\n",
                utstring_body(rtevents_dir));
        /* exit(EXIT_FAILURE); */
        /* this can happen if a related pkg is not installed */
        /* example, see topkg and topkg-care */
        return;
    /* } else { */
    /*     if (verbosity > 1) */
    /*         LOG_DEBUG(0, "opened dir %s", utstring_body(rtevents_dir)); */
    }

    struct dirent *direntry;
    while ((direntry = readdir(d)) != NULL) {
        if(direntry->d_type==DT_REG){
            /* link files starting with "rtevents" */
            if (strncmp("runtime_events", direntry->d_name, 7) != 0)
                continue;

            utstring_renew(src);
            utstring_printf(src, "%s/%s",
                            utstring_body(rtevents_dir),
                            direntry->d_name);
            utstring_renew(dst);
            utstring_printf(dst, "%s/%s",
                            tgtdir, direntry->d_name);
            /* LOG_DEBUG(0, "symlinking %s to %s", */
            /*           utstring_body(src), */
            /*           utstring_body(dst)); */
            errno = 0;
            rc = symlink(utstring_body(src),
                         utstring_body(dst));
            symlink_ct++;
            if (rc != 0) {
                if (errno != EEXIST) {
                    log_error("ERROR: %s", strerror(errno));
                    log_error("src: %s, dst: %s",
                              utstring_body(src),
                              utstring_body(dst));
                    log_error("exiting");
                    exit(EXIT_FAILURE);
                }
            }
        }
    }
    closedir(d);
    TRACE_EXIT;
}

/* ************************************* */
void emit_ocaml_str_pkg(char *runfiles,
                        char *switch_lib,
                        char *coswitch_lib)
{
    TRACE_ENTRY;

    // add_toplevel true means ocaml version >= 5.0.0
    // which means switch lacks lib/str
    // but we add it to coswitch so @str//lib/str will
    // work with >= 5.0.0
    bool add_toplevel = false;

    UT_string *str_dir;
    utstring_new(str_dir);
    utstring_printf(str_dir,
                    "%s/str",
                    switch_lib);
    int rc = access(utstring_body(str_dir), F_OK);
    if (rc != 0) {
#if defined(PROFILE_fastbuild)
        /* if (coswitch_trace) */
        LOG_WARN(0, YEL "NOT FOUND: %s" CRESET,
                 utstring_body(str_dir));
#endif
        utstring_new(str_dir);
        utstring_printf(str_dir,
                        "%s/ocaml/str",
                        switch_lib);
        int rc = access(utstring_body(str_dir), F_OK);
        if (rc != 0) {
#if defined(PROFILE_fastbuild)
            /* if (coswitch_trace) */
            LOG_WARN(0, YEL "NOT FOUND: %s" CRESET,
                     utstring_body(str_dir));
#endif
            utstring_free(str_dir);
            return;
        } else {
#if defined(PROFILE_fastbuild)
            // found lib/ocaml/str
            LOG_WARN(0, YEL "FOUND: %s" CRESET,
                     utstring_body(str_dir));
#endif
            add_toplevel = true;
        }
/* #if defined(PROFILE_fastbuild) */
/*     } else { */
/*         if (verbosity > 2) */
/*             LOG_WARN(0, YEL "FOUND: %s" CRESET, utstring_body(str_dir)); */
/* #endif */
    } else {
#if defined(PROFILE_fastbuild)
        /* if (coswitch_trace) */
        LOG_WARN(0, YEL "FOUND: %s" CRESET,
                 utstring_body(str_dir));
#endif
    }

    UT_string *templates;
    utstring_new(templates);
    utstring_printf(templates, "%s/templates/ocaml",
                    runfiles); //, "/new/templates");

    UT_string *src_file;
    UT_string *dst_dir;
    UT_string *dst_file;
    utstring_new(src_file);
    utstring_new(dst_dir);
    utstring_new(dst_file);

    utstring_printf(src_file, "%s/%s",
                    utstring_body(templates),
                    "ocaml_str.BUILD");
    if (add_toplevel) {
        // we found lib/ocaml/str, so we need to emit the
        // toplevel w/alias, lib/str
        // but we only symlink to lib/ocaml/lib/str
        /* LOG_DEBUG(0, "EMITTING STR TOPLEVEL"); */
        utstring_printf(dst_dir,
                        "%s/str",
                        coswitch_lib);
        mkdir_r(utstring_body(dst_dir));
        utstring_printf(dst_file, "%s/BUILD.bazel",
                        utstring_body(dst_dir));
        _emit_toplevel(templates,
                       "ocaml_str_alias.BUILD",
                       src_file,
                       dst_dir, dst_file,
                       coswitch_lib,
                       "str");

        // and also lib/ocaml/str
        utstring_renew(src_file);
        utstring_printf(src_file, "%s/%s",
                        utstring_body(templates),
                        "ocaml_str.BUILD");
        utstring_renew(dst_dir);
        utstring_printf(dst_dir,
                        "%s/ocaml/lib/str",
                        coswitch_lib);
        mkdir_r(utstring_body(dst_dir));
        utstring_renew(dst_file);
        utstring_printf(dst_file, "%s/BUILD.bazel",
                        utstring_body(dst_dir));
    LOG_DEBUG(2, "cp src: %s, dst: %s",
              utstring_body(src_file), utstring_body(dst_file));
        copy_buildfile(utstring_body(src_file), dst_file);
        // but symlink to lib/ocaml/str
        _symlink_ocaml_str(str_dir, utstring_body(dst_dir));

    } else {
        // if we found <switch>/lib/str (versions < 5.0.0)
        // then we need to link
        // from <coswitch>/lib/ocaml
        // to <coswitch>/lib/ocaml/lib/str
        utstring_printf(dst_dir,
                        "%s/ocaml/lib/str",
                        coswitch_lib);
        mkdir_r(utstring_body(dst_dir));
        utstring_printf(dst_file, "%s/BUILD.bazel",
                        utstring_body(dst_dir));
    LOG_DEBUG(2, "cp src: %s, dst: %s",
              utstring_body(src_file), utstring_body(dst_file));
        copy_buildfile(utstring_body(src_file), dst_file);

        utstring_new(str_dir);
        utstring_printf(str_dir,
                        "%s/ocaml",
                        switch_lib);
        _symlink_ocaml_str(str_dir,
                           utstring_body(dst_dir));

    }

    utstring_free(src_file);
    utstring_free(dst_file);
    utstring_free(templates);
    utstring_free(str_dir);
    TRACE_EXIT;
}

void _symlink_ocaml_str(UT_string *str_dir, char *tgtdir)
{
    TRACE_ENTRY;
#if defined(PROFILE_fastbuild)
    if (verbosity > 3) {
        LOG_DEBUG(0, "src: %s,\n\tdst %s",
                  utstring_body(str_dir),
                  tgtdir);
    }
#endif

    /* UT_string *opamdir; */
    /* utstring_new(opamdir); */
    /* utstring_printf(opamdir, "%s/ocaml", switch_lib); */
    /*                 /\* utstring_body(opam_switch_lib)); *\/ */

    UT_string *src;
    utstring_new(src);
    UT_string *dst;
    utstring_new(dst);
    int rc;

    DIR *d = opendir(utstring_body(str_dir));
    if (d == NULL) {
        log_error("Unable to opendir for symlinking str: %s\n",
                  utstring_body(str_dir));
        /* exit(EXIT_FAILURE); */
        /* this can happen if a related pkg is not installed */
        /* example, see topkg and topkg-care */
        return;
    /* } else { */
    /*     if (verbosity > 1) */
    /*         LOG_DEBUG(0, "opened dir %s", utstring_body(str_dir)); */
    } else {
#if defined(PROFILE_fastbuild)
        LOG_DEBUG(0, "fopened %s as symlink src",
                  utstring_body(str_dir));
#endif
    }

    struct dirent *direntry;
    while ((direntry = readdir(d)) != NULL) {
        /* LOG_DEBUG(0, "direntry: %s", direntry->d_name); */
        if(direntry->d_type==DT_REG){
            /* link files starting with "str." */
            if (strncmp("str.", direntry->d_name, 4) != 0)
                continue;

            utstring_renew(src);
            utstring_printf(src, "%s/%s",
                            utstring_body(str_dir),
                            direntry->d_name);
            utstring_renew(dst);
            utstring_printf(dst, "%s/%s",
                            tgtdir, direntry->d_name);
            /* LOG_DEBUG(0, "symlinking %s to %s", */
            /*           utstring_body(src), */
            /*           utstring_body(dst)); */
            rc = symlink(utstring_body(src),
                         utstring_body(dst));
            symlink_ct++;
            if (rc != 0) {
                if (errno != EEXIST) {
                    log_error("ERROR: %s", strerror(errno));
                    log_error("src: %s, dst: %s",
                              utstring_body(src),
                              utstring_body(dst));
                    log_error("exiting");
                    exit(EXIT_FAILURE);
                }
            }
        }
    }
    closedir(d);
    TRACE_EXIT;
}

/* ***************************************** */
/**
   pre-5.0.0: <switch>/threads and <switch>/lib/ocaml/threads
   5.0.0+:    <switch>/lib/ocaml/threads only
 */
void emit_ocaml_threads_pkg(char *runfiles,
                            char *switch_lib,
                            char *coswitch_lib)
{
    TRACE_ENTRY;
    bool add_toplevel = false;

    UT_string *threads_dir;
    utstring_new(threads_dir);
    utstring_printf(threads_dir,
                    "%s/threads",
                    switch_lib);
    int rc = access(utstring_body(threads_dir), F_OK);
    if (rc != 0) {
#if defined(PROFILE_fastbuild)
        /* if (coswitch_trace) */
        LOG_WARN(0, YEL "NOT FOUND: %s" CRESET,
                 utstring_body(threads_dir));
#endif
        utstring_new(threads_dir);
        utstring_printf(threads_dir,
                        "%s/ocaml/threads",
                        switch_lib);
        int rc = access(utstring_body(threads_dir), F_OK);
        if (rc != 0) {
#if defined(PROFILE_fastbuild)
            /* if (coswitch_trace) */
            LOG_WARN(0, YEL "NOT FOUND: %s" CRESET,
                     utstring_body(threads_dir));
#endif
            utstring_free(threads_dir);
            return;
#if defined(PROFILE_fastbuild)
        } else {
            LOG_WARN(0, YEL "FOUND: %s" CRESET,
                     utstring_body(threads_dir));
            add_toplevel = true;

#endif
        }
/* #if defined(PROFILE_fastbuild) */
/*     } else { */
/*         LOG_WARN(0, YEL "FOUND: %s" CRESET, utstring_body(threads_dir)); */
/* #endif */
    }

    // 5.0.0+ has <switch>/lib/ocaml/threads
    // pre-5.0.0: no <switch>/lib/ocaml/threads
    // always emit <coswitch>/lib/ocaml/threads
    UT_string *templates;
    utstring_new(templates);
    utstring_printf(templates, "%s/templates/ocaml", runfiles);

    UT_string *src_file;
    UT_string *dst_dir;
    UT_string *dst_file;
    utstring_new(src_file);
    utstring_new(dst_dir);
    utstring_new(dst_file);

    utstring_printf(src_file, "%s/%s",
                    utstring_body(templates),
                    "ocaml_threads.BUILD");
    utstring_printf(dst_dir,
                    "%s/ocaml/lib/threads",
                    coswitch_lib);
    mkdir_r(utstring_body(dst_dir));
    utstring_printf(dst_file, "%s/BUILD.bazel",
                    utstring_body(dst_dir));
    LOG_DEBUG(2, "cp src: %s, dst: %s",
              utstring_body(src_file), utstring_body(dst_file));
    copy_buildfile(utstring_body(src_file), dst_file);

    if (add_toplevel) {
        // not found: <switch>/lib/threads
        // means >= 5.0.0
        // add <coswitch>/lib/threads for (bazel) compatibility
        _emit_toplevel(templates,
                       "ocaml_threads_alias.BUILD",
                       src_file,
                       dst_dir, dst_file,
                       coswitch_lib,
                       "threads");
    }
    // all versions have <switch>/lib/ocaml/threads
    // so we always symlink to <coswitch>/lib/ocaml/threads
    utstring_new(threads_dir);
    utstring_printf(threads_dir,
                    "%s/ocaml",
                    switch_lib);
    utstring_renew(dst_dir);
    utstring_printf(dst_dir,
                    "%s/ocaml/lib/threads",
                    coswitch_lib);

    // symlink <switch>/lib/ocaml/threads
    _symlink_ocaml_threads(threads_dir, utstring_body(dst_dir));
    // symlink <switch>/lib/ocaml/libthreads.s, libthreadsnat.a

    utstring_free(src_file);
    utstring_free(dst_file);
    utstring_free(templates);
    utstring_free(threads_dir);
    TRACE_EXIT;
}

void _symlink_ocaml_threads(UT_string *threads_dir, char *tgtdir)
{
    TRACE_ENTRY;
#if defined(PROFILE_fastbuild)
    if (verbosity > 2) {
        LOG_DEBUG(0, "  src: %s", utstring_body(threads_dir));
        LOG_DEBUG(0, "  dst: %s", tgtdir);
    }
#endif
    UT_string *src;
    utstring_new(src);
    UT_string *dst;
    utstring_new(dst);
    int rc;

    utstring_printf(src, "%s/libthreads.a",
                    utstring_body(threads_dir));
    //FIXME: verify access
    utstring_printf(dst, "%s/libthreads.a",
                    tgtdir);
    if (verbosity > log_symlinks)
        fprintf(INFOFD, GRN "INFO" CRESET
                " symlink: %s -> %s\n",
                  utstring_body(src),
                  utstring_body(dst));
    rc = symlink(utstring_body(src),
                 utstring_body(dst));
    symlink_ct++;
    if (rc != 0) {
        if (errno != EEXIST) {
            log_error("ERROR: %s", strerror(errno));
            log_error("src: %s, dst: %s",
                      utstring_body(src),
                      utstring_body(dst));
            log_error("exiting");
            exit(EXIT_FAILURE);
        }
    }

    utstring_renew(src);
    utstring_printf(src, "%s/libthreadsnat.a",
                    utstring_body(threads_dir));
    //FIXME: verify access
    utstring_renew(dst);
    utstring_printf(dst, "%s/libthreadsnat.a",
                    tgtdir);
    if (verbosity > log_symlinks) {
        fprintf(INFOFD, GRN "INFO" CRESET
                " symlink: %s -> %s\n",
                utstring_body(src),
                utstring_body(dst));
    }
    rc = symlink(utstring_body(src),
                 utstring_body(dst));
    symlink_ct++;
    if (rc != 0) {
        if (errno != EEXIST) {
            log_error("ERROR: %s", strerror(errno));
            log_error("src: %s, dst: %s",
                      utstring_body(src),
                      utstring_body(dst));
            log_error("exiting");
            exit(EXIT_FAILURE);
        }
    }

    utstring_printf(threads_dir, "/%s", "threads");
    DIR *d = opendir(utstring_body(threads_dir));
    if (d == NULL) {
        fprintf(stderr, "Unable to opendir for symlinking threads: %s\n",
                utstring_body(threads_dir));
        log_error("Unable to opendir for symlinking threads: %s\n", utstring_body(threads_dir));
        /* exit(EXIT_FAILURE); */
        /* this can happen if a related pkg is not installed */
        /* example, see topkg and topkg-care */
        return;
    }

    struct dirent *direntry;
    while ((direntry = readdir(d)) != NULL) {
        if(direntry->d_type==DT_REG){
            utstring_renew(src);
            utstring_printf(src, "%s/%s",
                            utstring_body(threads_dir),
                            direntry->d_name);
            utstring_renew(dst);
            utstring_printf(dst, "%s/%s",
                            tgtdir, direntry->d_name);
            /* printf("symlinking %s to %s\n", */
            /*        utstring_body(src), */
            /*        utstring_body(dst)); */
            rc = symlink(utstring_body(src),
                         utstring_body(dst));
            symlink_ct++;
            if (rc != 0) {
                if (errno != EEXIST) {
                    log_error("ERROR: %s", strerror(errno));
                    log_error("src: %s, dst: %s",
                              utstring_body(src),
                              utstring_body(dst));
                    log_error("exiting");
                    exit(EXIT_FAILURE);
                }
            }
        }
    }
    closedir(d);
    TRACE_EXIT;
}

/* ***************************************** */
//FIXMEFIXME
void emit_ocaml_ocamldoc_pkg(char *runfiles,
                             char *switch_lib,
                             char *coswitch_lib)
{
    TRACE_ENTRY;

    UT_string *ocamldoc_dir;
    utstring_new(ocamldoc_dir);
    utstring_printf(ocamldoc_dir,
                    "%s/ocamldoc",
                    switch_lib);
    int rc = access(utstring_body(ocamldoc_dir), F_OK);
    if (rc != 0) {
#if defined(PROFILE_fastbuild)
        /* if (coswitch_trace) */
        LOG_WARN(0, YEL "NOT FOUND: %s" CRESET,
                 utstring_body(ocamldoc_dir));
#endif
        utstring_new(ocamldoc_dir);
        utstring_printf(ocamldoc_dir,
                        "%s/ocaml/ocamldoc",
                        switch_lib);
        int rc = access(utstring_body(ocamldoc_dir), F_OK);
        if (rc != 0) {
#if defined(PROFILE_fastbuild)
            /* if (coswitch_trace) */
            LOG_WARN(0, YEL "NOT FOUND: %s" CRESET,
                     utstring_body(ocamldoc_dir));
#endif
            utstring_free(ocamldoc_dir);
            return;
#if defined(PROFILE_fastbuild)
        } else {
            LOG_WARN(0, YEL "FOUND: %s" CRESET,
                     utstring_body(ocamldoc_dir));
#endif
        }
/* #if defined(PROFILE_fastbuild) */
/*     } else { */
/*         LOG_WARN(0, YEL "FOUND: %s" CRESET, utstring_body(ocamldoc_dir)); */
/* #endif */
    }

    UT_string *templates;
    utstring_new(templates);
    utstring_printf(templates, "%s/templates/ocaml", runfiles);
    /* utstring_printf(templates, "%s/%s", */
    /*                 runfiles, "/new/coswitch/templates"); */

    UT_string *src_file;
    UT_string *dst_dir;
    UT_string *dst_file;
    utstring_new(src_file);
    utstring_new(dst_dir);
    utstring_new(dst_file);

    utstring_printf(src_file, "%s/%s",
                    utstring_body(templates),
                    "ocaml_ocamldoc.BUILD");
    utstring_printf(dst_dir,
                    "%s/ocaml/lib/ocamldoc",
                    coswitch_lib);
    mkdir_r(utstring_body(dst_dir));
    utstring_printf(dst_file, "%s/BUILD.bazel",
                    utstring_body(dst_dir));
    LOG_DEBUG(2, "cp src: %s, dst: %s",
              utstring_body(src_file), utstring_body(dst_file));
    copy_buildfile(utstring_body(src_file), dst_file);

    _symlink_ocaml_ocamldoc(ocamldoc_dir, utstring_body(dst_dir));

    utstring_free(src_file);
    utstring_free(dst_file);
    utstring_free(templates);
    utstring_free(ocamldoc_dir);
    TRACE_EXIT;
}

void _symlink_ocaml_ocamldoc(UT_string *ocamldoc_dir,
                             char *tgtdir)
{
    TRACE_ENTRY;

    /* UT_string *opamdir; */
    /* utstring_new(opamdir); */
    /* utstring_printf(opamdir, "%s/ocaml/ocamldoc", switch_lib); */
    /*                 /\* utstring_body(opam_switch_lib)); *\/ */

    UT_string *src;
    utstring_new(src);
    UT_string *dst;
    utstring_new(dst);
    int rc;

    DIR *d = opendir(utstring_body(ocamldoc_dir));
    if (d == NULL) {
        fprintf(stderr, "Unable to opendir for symlinking ocamldoc: %s\n",
                utstring_body(ocamldoc_dir));
        /* exit(EXIT_FAILURE); */
        /* this can happen if a related pkg is not installed */
        /* example, see topkg and topkg-care */
        return;
    /* } else { */
    /*     if (verbosity > 1) */
    /*         LOG_DEBUG(0, "opened dir %s", utstring_body(ocamldoc_dir)); */
    }

    struct dirent *direntry;
    while ((direntry = readdir(d)) != NULL) {
        if(direntry->d_type==DT_REG){
            /* if (strcasestr(direntry->d_name, "ocamldoc") == NULL) */
            /* if (strncmp("odoc", direntry->d_name, 4) != 0) */
            /*     continue; */

            utstring_renew(src);
            utstring_printf(src, "%s/%s",
                            utstring_body(ocamldoc_dir),
                            direntry->d_name);
            utstring_renew(dst);
            utstring_printf(dst, "%s/%s",
                            tgtdir, direntry->d_name);
            /* LOG_DEBUG(0, "ocamldoc symlinking %s to %s", */
            /*           utstring_body(src), */
            /*           utstring_body(dst)); */
            rc = symlink(utstring_body(src),
                         utstring_body(dst));
            symlink_ct++;
            if (rc != 0) {
                if (errno != EEXIST) {
                    log_error("ERROR: %s", strerror(errno));
                    log_error("src: %s, dst: %s",
                              utstring_body(src),
                              utstring_body(dst));
                    log_error("exiting");
                    exit(EXIT_FAILURE);
                }
            }
        }
    }
    closedir(d);
    TRACE_EXIT;
}

void emit_ocaml_unix_pkg(char *runfiles,
                         char *switch_lib,
                         char *coswitch_lib)
{
    TRACE_ENTRY;
    bool add_toplevel = false;

    UT_string *unix_dir;
    utstring_new(unix_dir);
    utstring_printf(unix_dir,
                    "%s/unix",
                    switch_lib);

    int rc = access(utstring_body(unix_dir), F_OK);
    if (rc != 0) {
#if defined(PROFILE_fastbuild)
        /* if (coswitch_trace) */
        LOG_WARN(0, YEL "NOT FOUND: %s" CRESET,
                 utstring_body(unix_dir));
#endif
        utstring_new(unix_dir);
        utstring_printf(unix_dir,
                        "%s/ocaml/unix",
                        switch_lib);
        int rc = access(utstring_body(unix_dir), F_OK);
        if (rc != 0) {
#if defined(PROFILE_fastbuild)
            /* if (coswitch_trace) */
            LOG_WARN(0, YEL "NOT FOUND: %s" CRESET,
                     utstring_body(unix_dir));
#endif
            utstring_free(unix_dir);
            return;
        } else {
#if defined(PROFILE_fastbuild)
            LOG_WARN(0, YEL "FOUND: %s" CRESET, utstring_body(unix_dir));
#endif
            add_toplevel = true;
        }
/* #if defined(PROFILE_fastbuild) */
/*     } else { */
/*         LOG_WARN(0, YEL "FOUND: %s" CRESET, utstring_body(unix_dir)); */
/* #endif */
    /* } else { */
    /*     LOG_WARN(0, YEL "FOUND: %s CRESET", */
    /*              utstring_body(unix_dir)); */
    }

    UT_string *templates;
    utstring_new(templates);
    utstring_printf(templates, "%s/templates/ocaml", runfiles);
    /* utstring_printf(templates, "%s/%s", */
    /*                 runfiles, "/new/coswitch/templates"); */

    UT_string *src_file;
    UT_string *dst_dir;
    UT_string *dst_file;
    utstring_new(src_file);
    utstring_new(dst_dir);
    utstring_new(dst_file);

    // 5.0.0+ has <switch>/lib/ocaml/unix
    // pre-5.0.0: no <switch>/lib/ocaml/unix
    // always emit <coswitch>/lib/ocaml/lib/unix
    utstring_printf(src_file, "%s/%s",
                    utstring_body(templates),
                    "ocaml_unix.BUILD");
    utstring_printf(dst_dir,
                    "%s/ocaml/lib/unix",
                    coswitch_lib);
    mkdir_r(utstring_body(dst_dir));
    utstring_printf(dst_file, "%s/BUILD.bazel",
                    utstring_body(dst_dir));
    LOG_DEBUG(2, "cp src: %s, dst: %s",
              utstring_body(src_file), utstring_body(dst_file));
    copy_buildfile(utstring_body(src_file), dst_file);

    if (add_toplevel) {
        // found <switch>/lib/ocaml/unix,
        // not found: <switch>/lib/unix, means >= 5.0.0
        // link the former,
        // add <coswitch>/lib/unix alias for (bazel) compat
        _symlink_ocaml_unix(unix_dir,
                            utstring_body(dst_dir));
        // also link <switch>/lib/ocaml/libunix.a
        utstring_renew(src_file);
        utstring_printf(src_file, "%s/ocaml/libunix.a",
                        switch_lib);
        //FIXME: verify access
        utstring_renew(dst_file);
        utstring_printf(dst_file, "%s/libunix.a",
                        utstring_body(dst_dir));
        /* LOG_DEBUG(0, "SYMLINKING %s to %s\n", */
        /*           utstring_body(src_file), */
        /*           utstring_body(dst_file)); */
        rc = symlink(utstring_body(src_file),
                     utstring_body(dst_file));
        symlink_ct++;
        if (rc != 0) {
            if (errno != EEXIST) {
                log_error("ERROR: %s", strerror(errno));
                log_error("src: %s, dst: %s",
                          utstring_body(src_file),
                          utstring_body(dst_file));
                log_error("exiting");
                exit(EXIT_FAILURE);
            }
        }

        _emit_toplevel(templates,
                       "ocaml_unix_alias.BUILD",
                       src_file,
                       dst_dir, dst_file,
                       coswitch_lib,
                       "unix");
    } else {
        // if we found <switch>/lib/unix (versions < 5.0.0)
        // then we need to link to <coswitch>/lib/ocaml/unix
        // (its already there for >= 5.0.0)
        utstring_new(unix_dir);
        utstring_printf(unix_dir,
                        "%s/ocaml",
                        switch_lib);
        _symlink_ocaml_unix(unix_dir, utstring_body(dst_dir));
    }

    utstring_free(src_file);
    utstring_free(dst_file);
    utstring_free(templates);
    utstring_free(unix_dir);
    TRACE_EXIT;
}

void _symlink_ocaml_unix(UT_string *unix_dir, char *tgtdir)
{
    TRACE_ENTRY;
    /* LOG_DEBUG(0, "unix_dir: %s", utstring_body(unix_dir)); */
    /* LOG_DEBUG(0, "tgtdir: %s", tgtdir); */
    UT_string *src;
    utstring_new(src);
    UT_string *dst;
    utstring_new(dst);
    int rc;

    DIR *d = opendir(utstring_body(unix_dir));
    if (d == NULL) {
        fprintf(stderr, "Unable to opendir for symlinking unix: %s\n",
                utstring_body(unix_dir));
        /* exit(EXIT_FAILURE); */
        /* this can happen if a related pkg is not installed */
        /* example, see topkg and topkg-care */
        return;
    }

    struct dirent *direntry;
    while ((direntry = readdir(d)) != NULL) {
        if(direntry->d_type==DT_REG){
            /* link files starting with "unix." */
            if (strncmp("unix.", direntry->d_name, 5) != 0)
                if (strncmp("unixLabels.", direntry->d_name, 11) != 0)
                    if (strncmp("libunix", direntry->d_name, 7) != 0)
                continue;

            utstring_renew(src);
            utstring_printf(src, "%s/%s",
                            utstring_body(unix_dir),
                            direntry->d_name);
            utstring_renew(dst);
            utstring_printf(dst, "%s/%s",
                            tgtdir, direntry->d_name);
            /* LOG_DEBUG(0, "symlinking %s to %s", */
            /*        utstring_body(src), */
            /*        utstring_body(dst)); */
            rc = symlink(utstring_body(src),
                         utstring_body(dst));
            symlink_ct++;
            if (rc != 0) {
                if (errno != EEXIST) {
                    log_error("ERROR: %s", strerror(errno));
                    log_error("src: %s, dst: %s",
                              utstring_body(src),
                              utstring_body(dst));
                    log_error("exiting");
                    exit(EXIT_FAILURE);
                }
            }
        }
    }
    closedir(d);
    TRACE_EXIT;
}

/* ***************************************** */
void emit_ocaml_c_api_pkg(char *runfiles,
                          char *switch_lib,
                          char *coswitch_lib)
{
    TRACE_ENTRY;
    UT_string *templates;
    utstring_new(templates);
    utstring_printf(templates, "%s/templates/ocaml", runfiles);

    UT_string *src_file;
    UT_string *dst_dir;
    UT_string *dst_file;
    utstring_new(src_file);
    utstring_new(dst_dir);
    utstring_new(dst_file);

    utstring_printf(src_file, "%s/%s",
                    utstring_body(templates),
                    "ocaml_c_api.BUILD");
    utstring_printf(dst_dir,
                    "%s/ocaml/lib/sdk",
                    coswitch_lib);
    mkdir_r(utstring_body(dst_dir));

    utstring_printf(dst_file,
                    "%s/BUILD.bazel",
                    utstring_body(dst_dir));
    LOG_DEBUG(2, "cp src: %s, dst: %s",
              utstring_body(src_file), utstring_body(dst_file));
    copy_buildfile(utstring_body(src_file), dst_file);

    _symlink_ocaml_c_hdrs(switch_lib, utstring_body(dst_dir));

    utstring_free(templates);
    utstring_free(src_file);
    utstring_free(dst_dir);
    utstring_free(dst_file);
    TRACE_EXIT;
}

void _symlink_ocaml_c_hdrs(char *switch_lib, char *tgtdir)
{
    TRACE_ENTRY;
    UT_string *opamdir;
    utstring_new(opamdir);
    utstring_printf(opamdir, "%s/ocaml/caml", switch_lib);

    UT_string *obazldir;
    utstring_new(obazldir);
    utstring_printf(obazldir, "%s/caml", tgtdir);
    mkdir_r(utstring_body(obazldir));

    UT_string *src;
    utstring_new(src);
    UT_string *dst;
    utstring_new(dst);
    int rc;

    DIR *d = opendir(utstring_body(opamdir));
    if (d == NULL) {
        fprintf(stderr, "Unable to opendir for symlinking c sdk: %s\n",
                utstring_body(opamdir));
        /* exit(EXIT_FAILURE); */
        /* this can happen if a related pkg is not installed */
        /* example, see topkg and topkg-care */
        return;
    }

    struct dirent *direntry;
    while ((direntry = readdir(d)) != NULL) {
        if (direntry->d_type==DT_REG){
            /* if (strncmp("lib", direntry->d_name, 3) != 0) */
            /*     continue;       /\* no match *\/ */
            //FIXME: check for .h?

            utstring_renew(src);
            utstring_printf(src, "%s/%s",
                            utstring_body(opamdir),
                            direntry->d_name);
            utstring_renew(dst);
            utstring_printf(dst, "%s/%s",
                            utstring_body(obazldir),
                            direntry->d_name);

            if (verbosity > log_symlinks) {
                fprintf(INFOFD, GRN "INFO" CRESET
                        " symlink: %s -> %s\n",
                        utstring_body(src),
                        utstring_body(dst));
            }
            errno = 0;
            rc = symlink(utstring_body(src),
                         utstring_body(dst));
            symlink_ct++;
            if (rc != 0) {
                if (errno != EEXIST) {
                    log_error("ERROR: %s", strerror(errno));
                    log_error("src: %s, dst: %s",
                              utstring_body(src),
                              utstring_body(dst));
                    log_error("exiting");
                    exit(EXIT_FAILURE);
                }
            }
        }
    }
    closedir(d);
}

void _symlink_ocaml_runtime_libs(char *switch_lib, char *tgtdir)
{
    TRACE_ENTRY;
#if defined(PROFILE_fastbuild)
    LOG_DEBUG(0, "\tswitch_lib: %s\n", switch_lib);
    LOG_DEBUG(0, "\ttgtdir    : %s\n", tgtdir);
#endif

    UT_string *opamdir;
    utstring_new(opamdir);
    utstring_printf(opamdir, "%s/ocaml", switch_lib);

    UT_string *obazldir;
    utstring_new(obazldir);
    utstring_printf(obazldir, "%s", tgtdir);
    mkdir_r(utstring_body(obazldir));

    UT_string *src;
    utstring_new(src);
    UT_string *dst;
    utstring_new(dst);
    int rc;

    DIR *d = opendir(utstring_body(opamdir));
    if (d == NULL) {
        fprintf(stderr, "Unable to opendir for symlinking c_libs: %s\n",
                utstring_body(opamdir));
        /* exit(EXIT_FAILURE); */
        /* this can happen if a related pkg is not installed */
        /* example, see topkg and topkg-care */
        return;
    }

    struct dirent *direntry;
    while ((direntry = readdir(d)) != NULL) {
        if(direntry->d_type==DT_REG){
            if (strncmp("lib", direntry->d_name, 3) != 0) {
                if (strncmp("stdlib.a",
                            direntry->d_name, 8) != 0) {
                continue;
                }
            }
            /* if (strncmp("libasm", direntry->d_name, 6) != 0) { */
            /*     if (strncmp("libcaml", */
            /*                 direntry->d_name, 7) != 0) { */
            /*         continue;       /\* no match *\/ */
            /*     } */
            /* } */

            utstring_renew(src);
            utstring_printf(src, "%s/%s",
                            utstring_body(opamdir), direntry->d_name);
            utstring_renew(dst);
            utstring_printf(dst, "%s/%s",
                            utstring_body(obazldir), /* tgtdir, */
                            direntry->d_name);
            LOG_DEBUG(1, "c_libs: symlinking %s to %s\n",
                      utstring_body(src), utstring_body(dst));
            errno = 0;
            rc = symlink(utstring_body(src),
                         utstring_body(dst));
            symlink_ct++;
            if (rc != 0) {
                if (errno != EEXIST) {
                    fprintf(stdout, "c_libs symlink errno: %d: %s\n",
                            errno, strerror(errno));
                    fprintf(stdout, "c_libs src: %s, dst: %s\n",
                            utstring_body(src),
                            utstring_body(dst));
                    fprintf(stderr, "exiting\n");
                    exit(EXIT_FAILURE);
                }
            }
        }
    }
    closedir(d);
}

void _symlink_ocaml_runtime_compiler_libs(char *switch_lib, char *tgtdir)
{
    TRACE_ENTRY;
#if defined(PROFILE_fastbuild)
    LOG_DEBUG(0, "\tswitch_lib: %s\n", switch_lib);
    LOG_DEBUG(0, "\ttgtdir    : %s\n", tgtdir);
#endif

    UT_string *opamdir;
    utstring_new(opamdir);
    utstring_printf(opamdir, "%s/ocaml/compiler-libs", switch_lib);

    UT_string *obazldir;
    utstring_new(obazldir);
    utstring_printf(obazldir, "%s", tgtdir);
    mkdir_r(utstring_body(obazldir));

    UT_string *src;
    utstring_new(src);
    UT_string *dst;
    utstring_new(dst);
    int rc;

    DIR *d = opendir(utstring_body(opamdir));
    if (d == NULL) {
        fprintf(stderr, "Unable to opendir for symlinking c_libs: %s\n",
                utstring_body(opamdir));
        /* exit(EXIT_FAILURE); */
        /* this can happen if a related pkg is not installed */
        /* example, see topkg and topkg-care */
        return;
    }

    struct dirent *direntry;
    while ((direntry = readdir(d)) != NULL) {
        if(direntry->d_type==DT_REG){
            if (strncmp("ocaml", direntry->d_name, 5) != 0) {
                continue;
            }
            /* if (strncmp("libasm", direntry->d_name, 6) != 0) { */
            /*     if (strncmp("libcaml", */
            /*                 direntry->d_name, 7) != 0) { */
            /*         continue;       /\* no match *\/ */
            /*     } */
            /* } */

            utstring_renew(src);
            utstring_printf(src, "%s/%s",
                            utstring_body(opamdir), direntry->d_name);
            utstring_renew(dst);
            utstring_printf(dst, "%s/%s",
                            utstring_body(obazldir), /* tgtdir, */
                            direntry->d_name);
            LOG_DEBUG(1, "c_libs: symlinking %s to %s\n",
                      utstring_body(src),
                      utstring_body(dst));
            errno = 0;
            rc = symlink(utstring_body(src),
                         utstring_body(dst));
            symlink_ct++;
            if (rc != 0) {
                if (errno != EEXIST) {
                    fprintf(stdout, "c_libs symlink errno: %d: %s\n",
                            errno, strerror(errno));
                    fprintf(stdout, "c_libs src: %s, dst: %s\n",
                            utstring_body(src),
                            utstring_body(dst));
                    fprintf(stderr, "exiting\n");
                    exit(EXIT_FAILURE);
                }
            }
        }
    }
    closedir(d);
}

void emit_ocaml_version(char *coswitch_lib, char *version)
{
    TRACE_ENTRY;
    UT_string *dst_dir;
    UT_string *dst_file;
    utstring_new(dst_dir);
    utstring_new(dst_file);

    utstring_printf(dst_dir,
                    "%s/ocaml/version",
                    coswitch_lib);
    mkdir_r(utstring_body(dst_dir));

    utstring_printf(dst_file,
                    "%s/BUILD.bazel",
                    utstring_body(dst_dir));

    errno = 0;
    FILE *ostream;
    ostream = fopen(utstring_body(dst_file), "w");
    if (ostream == NULL) {
        log_error("fopen fail on %s: %s",
                  utstring_body(dst_file),
                  strerror(errno));
        exit(EXIT_FAILURE);
    }

    fprintf(ostream, "## generated file - DO NOT EDIT\n");

    fprintf(ostream,
            "load(\"@bazel_skylib//rules:common_settings.bzl\", \"string_setting\")\n\n");

    fprintf(ostream,
            "package(default_visibility=[\"//visibility:public\"])\n\n");

    fprintf(ostream,
            "string_setting(name = \"version\",\n"
            "               build_setting_default = \"%s\")\n",
            version);

    fclose(ostream);

    if (verbosity > log_writes) {
        fprintf(INFOFD, GRN "INFO" CRESET
                " wrote: %s\n", utstring_body(dst_file));
    }
    utstring_free(dst_dir);
    utstring_free(dst_file);
    TRACE_EXIT;
}


/*
  src: an opam <switch>/lib dir
  dst: same as src
      or XDG_DATA_HOME/obazl/opam/<switch>/
      or project-local _opam/
  bootstrap_FILE: remains open so opam pkgs can write to it
 */
EXPORT void emit_ocaml_workspace(UT_string *registry,
                                 char *compiler_version,
                                 struct obzl_meta_package *pkgs,
                                 char *switch_name,
                                 char *switch_pfx,
                                 char *coswitch_lib,
                                 char *runfiles)
{
    TRACE_ENTRY;
    LOG_DEBUG(0, "switch_name: %s", switch_name);
#if defined(PROFILE_fastbuild)
    /* if (verbosity > 0) { */
        LOG_TRACE(0, BLU "EMIT_ocaml_workspace:" CRESET
                  " switch_pfx:%s, dst: %s",
                  switch_pfx, coswitch_lib);
    /* } */
#endif

    char *switch_lib = opam_switch_lib(switch_name);

    /* this emits reg record for both ocaml and stublibs pkgs */
    emit_registry_record(registry, compiler_version, NULL, pkgs);

    UT_string *dst_file;
    utstring_new(dst_file);
    /* utstring_concat(dst_file, coswitch_lib); // pfx); */
    utstring_printf(dst_file, "%s/ocaml", coswitch_lib);
    mkdir_r(utstring_body(dst_file));

    utstring_printf(dst_file, "/WORKSPACE.bazel");

    FILE *ostream = fopen(utstring_body(dst_file), "w");
    if (ostream == NULL) {
        log_error("fopen: %s: %s", strerror(errno),
                  utstring_body(dst_file));
        fprintf(stderr, "fopen: %s: %s", strerror(errno),
                utstring_body(dst_file));
        fprintf(stderr, "exiting\n");
        /* perror(utstring_body(dst_file)); */
        exit(EXIT_FAILURE);
    }
    fprintf(ostream, "# generated file - DO NOT EDIT\n");

    fclose(ostream);
    if (verbosity > log_writes)
        fprintf(INFOFD, GRN "INFO" CRESET
                " wrote %s\n", utstring_body(dst_file));

    // now MODULE.bazel
    utstring_renew(dst_file);
    utstring_printf(dst_file,
                    "%s/ocaml/MODULE.bazel",
                    coswitch_lib);
    ostream = fopen(utstring_body(dst_file), "w");
    if (ostream == NULL) {
        log_error("%s", strerror(errno));
        perror(utstring_body(dst_file));
        exit(EXIT_FAILURE);
    }
    fprintf(ostream, "## generated file - DO NOT EDIT\n\n");
    fprintf(ostream, "module(\n");
    fprintf(ostream, "    name = \"ocaml\", version = \"%s\",\n",
            default_version);
    fprintf(ostream, "    compatibility_level = %d,\n",
            default_compat);
    fprintf(ostream, "    bazel_compatibility = [\">=%s\"]\n",
            bazel_compat);
    fprintf(ostream, ")\n");
    fprintf(ostream, "\n");
    fprintf(ostream,
            "bazel_dep(name = \"platforms\", version = \"%s\")\n", platforms_version);
    fprintf(ostream,
            "bazel_dep(name = \"bazel_skylib\", version = \"%s\")\n", skylib_version);
    fprintf(ostream,
            "bazel_dep(name = \"rules_ocaml\", version = \"%s\")\n", rules_ocaml_version);

    fprintf(ostream,
            "bazel_dep(name = \"stublibs\", version = \"%s\")\n",
            default_version);

    fclose(ostream);
    if (verbosity > log_writes)
        fprintf(INFOFD, GRN "INFO" CRESET
                " wrote %s\n", utstring_body(dst_file));



    /* now drop WORKSPACE.bazel from path */
    utstring_renew(dst_file);
    utstring_printf(dst_file, "%s/ocaml", coswitch_lib);

    /*
      if we are in a bazel env get runfiles dir
      else runfiles dir is <switch_pfx>/share/obazl/templates
     */

    /* UT_string *xrunfiles; */
    /* utstring_new(xrunfiles); */
    /* LOG_DEBUG(0, "PWD: %s", getcwd(NULL,0)); */
    /* LOG_DEBUG(0, "OPAM_SWITCH_PREFIX: %s", */
    /*           getenv("OPAM_SWITCH_PREFIX")); */
    /* LOG_DEBUG(0, "BAZEL_CURRENT_REPOSITORY: %s", */
    /*           BAZEL_CURRENT_REPOSITORY); */
    /* if (getenv("BUILD_WORKSPACE_DIRECTORY")) { */
    /*     if (strlen(BAZEL_CURRENT_REPOSITORY) == 0) { */
    /*         utstring_printf(xrunfiles, "%s", */
    /*                         realpath("new", NULL)); */
    /*     } else { */
    /*         LOG_DEBUG(0, "XXXXXXXXXXXXXXXX %s", */
    /*                   BAZEL_CURRENT_REPOSITORY); */
    /*         char *rp = realpath("external/" */
    /*                             BAZEL_CURRENT_REPOSITORY, */
    /*                             NULL); */
    /*         LOG_DEBUG(0, "PWD: %s", getcwd(NULL,0)); */
    /*         LOG_DEBUG(0, "AAAAAAAAAAAAAAAA %s", rp); */
    /*         utstring_printf(xrunfiles, "%s", rp); */
    /* LOG_DEBUG(0, "XXXXXXXXXXXXXXXX %s", */
    /*           utstring_body(xrunfiles)); */
    /*     } */
    /* } else { */
    /*     utstring_printf(xrunfiles, "%s/share/obazl", */
    /*                     switch_pfx); */
    /*     // runfiles = <switch-pfx>/share/obazl */
    /*     /\* runfiles = "../../../share/obazl"; *\/ */
    /* } */

    /* char *runfiles = utstring_body(xrunfiles); // strdup? */
    /* LOG_DEBUG(0, "RUNFILES: %s", runfiles); */

    emit_ocaml_bin_dir(switch_pfx, coswitch_lib);

    UT_string *templates;
    utstring_new(templates);
    utstring_printf(templates, "%s/templates", runfiles);

    emit_ocaml_platform_buildfiles(templates, coswitch_lib);

    emit_ocaml_toolchain_buildfiles(runfiles, coswitch_lib);

    emit_ocaml_stdlib_pkg(runfiles, switch_lib, coswitch_lib);

    /* this is an obazl invention: */
    emit_ocaml_runtime_pkg(runfiles,
                           switch_lib,
                           coswitch_lib);

    emit_ocaml_stublibs(switch_pfx, coswitch_lib);

    // for <switch>lib/stublibs:
    char *switch_stublibs = opam_switch_stublibs(switch_name);
    emit_lib_stublibs_pkg(registry, switch_stublibs, coswitch_lib);

    emit_compiler_libs_pkg(runfiles, coswitch_lib);
    emit_ocaml_compiler_libs_pkg(runfiles,
                                 switch_lib,
                                 coswitch_lib);

    emit_ocaml_bigarray_pkg(runfiles, switch_lib, coswitch_lib);

    emit_ocaml_dynlink_pkg(runfiles, switch_lib, coswitch_lib);

    emit_ocaml_num_pkg(runfiles, switch_lib, coswitch_lib);

    //FIXMEFIXME:
    emit_ocaml_ocamldoc_pkg(runfiles, switch_lib, coswitch_lib);

    emit_ocaml_rtevents_pkg(runfiles, switch_lib, coswitch_lib);

    emit_ocaml_str_pkg(runfiles, switch_lib, coswitch_lib);

    //NB: vmthreads removed in v. 4.08.0?
    emit_ocaml_threads_pkg(runfiles, switch_lib, coswitch_lib);
    /* if (!ocaml_prev5) */
        /* emit_registry_record(registry, compiler_version, */
        /*                      NULL, pkgs); */

    emit_ocaml_unix_pkg(runfiles, switch_lib, coswitch_lib);

    emit_ocaml_c_api_pkg(runfiles, switch_lib, coswitch_lib);

    emit_ocaml_version(coswitch_lib, compiler_version);

    TRACE_EXIT;
}
