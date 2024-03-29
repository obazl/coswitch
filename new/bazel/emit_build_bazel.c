#include <assert.h>
#include <errno.h>
#include <dirent.h>
#include <fnmatch.h>
#include <libgen.h>
/* #include <regex.h> */

#if EXPORT_INTERFACE
#include <stdio.h>
#endif

#ifdef __linux__
#include <linux/limits.h>
#else
#include <limits.h>
#endif
#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

/* #if EXPORT_INTERFACE */
/* #include "s7.h" */
/* #endif */

#include "liblogc.h"
/* #if EXPORT_INTERFACE */
/* #include "semver.h" */
#include "utarray.h"
#include "utstring.h"
/* #endif */

#include "findlibc.h"

#include "emit_build_bazel.h"

/* **************************************************************** */
/* s7_scheme *s7;                  /\* GLOBAL s7 *\/ */
/* /\* const char *errmsg = NULL; *\/ */

extern int level;
extern int spfactor;
extern char *sp;

extern int log_writes;
extern int log_symlinks;

#if defined(PROFILE_fastbuild)
#define DEBUG_LEVEL coswitch_debug
extern int  DEBUG_LEVEL;
#define TRACE_FLAG coswitch_trace
extern bool TRACE_FLAG;
extern int indent;
extern int delta;
#endif

/* extern */ char *ocaml_ws;
/* extern char *dst_root; */

extern bool verbose;
extern int debug;
extern bool trace;

/* extern bool stdlib_root; */

/* extern */ UT_string *bzl_switch_pfx;

/* char *coswitch_root = NULL;     /\* FIXME: rename bzl_switch_lib *\/ */

/* char *buildfile_prefix = "@//" HERE_OBAZL_ROOT "/buildfiles"; */
/* was: "@//.opam.d/buildfiles"; */

/* long *KPM_TABLE; */

/* FILE *opam_resolver; */

UT_string *repo_name = NULL;

UT_string *bazel_pkg_root;
UT_string *build_bazel_file;

char *pfxdir = NULL; // ""; // buildfiles";

int symlink_ct;

extern char *default_version;
extern int   default_compat;
extern char *bazel_compat;

bool distrib_pkg;

void emit_new_local_subpkg_entries(FILE *bootstrap_FILE,
                                   struct obzl_meta_package *_pkg,
                                   char *_subdir,
                                   char *_pkg_prefix,
                                   char *_filedeps_path)
{
    TRACE_ENTRY;
    /* dump_package(0, _pkg); */

    char *pkg_name = _pkg->name;
    int subpkg_ct = obzl_meta_package_subpkg_count(_pkg);
    LOG_DEBUG(0, "subpkg_ct: %d", subpkg_ct);

    if (subpkg_ct > 0) {

        obzl_meta_entries *entries = _pkg->entries;
        obzl_meta_entry *entry = NULL;
        obzl_meta_package *subpkg = NULL;

        UT_string *_new_pkg_prefix;
        utstring_new(_new_pkg_prefix);
        if (_pkg_prefix == NULL)
            utstring_printf(_new_pkg_prefix, "%s", pkg_name);
        else
            utstring_printf(_new_pkg_prefix, "%s/%s", _pkg_prefix, pkg_name);

        for (int i = 0; i < obzl_meta_entries_count(entries); i++) {
            entry = obzl_meta_entries_nth(entries, i);
            subpkg = entry->package;
            if (entry->type == OMP_PACKAGE) {
                LOG_DEBUG(0, "next subpkg: %s", subpkg->name);
                /* char *directory = NULL; */
                UT_string *filedeps_dir;
                utstring_new(filedeps_dir);
                if (_filedeps_path)
                    utstring_printf(filedeps_dir, "%s", _filedeps_path);

                struct obzl_meta_property *directory_prop = obzl_meta_entries_property(subpkg->entries, "directory");
                if (directory_prop) {
                    utstring_printf(filedeps_dir, "/%s",
                                    (char*)obzl_meta_property_value(directory_prop));
                }

                UT_string *subdir;
                utstring_new(subdir);

                /* dump_package(0, subpkg); */

                /* fprintf(bootstrap_FILE, "## subpkg: %s\n", subpkg->name); */

                if (obzl_meta_entries_property(subpkg->entries, "error")) {
                    // FIXME: emit 'fail()'
                    /* fprintf(bootstrap_FILE, */
                    /*         "            \"%s %s/lib\",\n", */
                    /*         subpkg->name, */
                    /*         opam_switch_prefix); */
                    ;           /* skip */
                } else {
                    fprintf(bootstrap_FILE,
                            "            \"@//%s/%s/%s:BUILD.bazel\":\n",
                            /* buildfile_prefix, */
                            utstring_body(bzl_switch_pfx),
                            utstring_body(_new_pkg_prefix),
                            /* pkg_name, */
                            subpkg->name);

                    utstring_renew(subdir);
                    if (_subdir == NULL) {
                        utstring_printf(subdir, "%s", subpkg->name);
                    } else {
                        utstring_printf(subdir, "%s/%s",
                                        _subdir, // _pkg_prefix,
                                        subpkg->name);
                    }
                    fprintf(bootstrap_FILE,
                            "            \"%s %s\",\n",
                            utstring_body(subdir),
                            /* opam_switch_prefix, */
                            /* depfiles_path, */
                            utstring_body(filedeps_dir));
                            /* filedeps_dir); */
                            /* pkg_name, */
                            /* subpkg->name); */
                }
                fprintf(bootstrap_FILE, "\n");

                /* recur to get subsubpkgs */
                emit_new_local_subpkg_entries(bootstrap_FILE,
                                              subpkg,
                                              utstring_body(subdir),
                                              utstring_body(_new_pkg_prefix),
                                              utstring_body(filedeps_dir));
                utstring_free(filedeps_dir);
                utstring_free(subdir);
            }
        }
    }
}

void emit_new_local_pkg_repo(FILE *bootstrap_FILE,
                             struct obzl_meta_package *_pkg)
{
    TRACE_ENTRY_MSG("pkg name: %s", _pkg->name);

    /* first emit for root pkg */
    /* then iterate over subpkgs */

    char *pkg_name = _pkg->name; // obzl_meta_package_name(_pkg);

    //FIXME rename:  new_local_opam_pkg_repository(\n");
    fprintf(bootstrap_FILE, "    new_local_pkg_repository(\n");
    /* fprintf(bootstrap_FILE, "    native.new_local_repository(\n"); */

    fprintf(bootstrap_FILE, "        name       = ");

    char *_pkg_prefix = NULL;
    // ????
    if (_pkg_prefix == NULL)
        fprintf(bootstrap_FILE, "\"%s\",\n", pkg_name);
    else {
        char *s = _pkg_prefix;
        char *tmp;
        while(s) {
            tmp = strchr(s, '/');
            if (tmp == NULL) break;
            *tmp = '.';
            s = tmp;
        }
        fprintf(bootstrap_FILE, "\"%s.%s\",\n",
                _pkg_prefix, pkg_name);
    }

    /* fprintf(bootstrap_FILE, "        environ = [\"OPAM_SWITCH_PREFIX\"],\n"); */

    fprintf(bootstrap_FILE, "        build_file = ");
    if (_pkg_prefix == NULL)
        fprintf(bootstrap_FILE,
                "\"@//%s/%s:BUILD.bazel\",\n",
                /* buildfile_prefix, */
                utstring_body(bzl_switch_pfx),
                pkg_name);
    else
        fprintf(bootstrap_FILE,
                "\"@//%s/%s/%s:BUILD.bazel\",\n",
                /* buildfile_prefix, */
                utstring_body(bzl_switch_pfx),
                _pkg_prefix, pkg_name);

    fprintf(bootstrap_FILE, "        path       = ");
    if (_pkg_prefix == NULL)
        fprintf(bootstrap_FILE, "\"%s\",\n",
                /* opam_switch_prefix, */
                pkg_name);
    else
        fprintf(bootstrap_FILE, "\"%s/%s\",\n",
                /* opam_switch_prefix, */
                _pkg_prefix, pkg_name);

    /* **************************************************************** */
    /*  now subpackages */
    /* **************************************************************** */

    if ((strncmp(_pkg->name, "threads", 7) == 0)
        && strlen(_pkg->name) == 7) {
        ; /* special case: skip threads subpkgs */
    } else {
        int subpkg_ct = obzl_meta_package_subpkg_count(_pkg);
        if (subpkg_ct > 0) {
            fprintf(bootstrap_FILE, "        subpackages = {\n");
            emit_new_local_subpkg_entries(bootstrap_FILE,
                                          _pkg,
                                          NULL,
                                          _pkg_prefix,
                                          _pkg->name); /* filedeps_path */
            fprintf(bootstrap_FILE, "        }\n");
        }
    }
    /* **************************************************************** */
    fprintf(bootstrap_FILE, "    )\n\n");
}

void emit_local_repo_decl(FILE *bootstrap_FILE,
                          char *pkg_name)
                          /* struct obzl_meta_package *_pkg) */
{
#if defined(PROFILE_fastbuild)
    LOG_DEBUG(0, "emit_local_repo_decl %s", pkg_name); //_pkg->name);
#endif

    /* UT_string *path; */
    /* utstring_new(path); */
    /* utstring_printf(path, "%s/%s/WORKSPACE.bazel", */
    /*                 utstring_body(bzl_switch_pfx), */
    /*                 pkg_name); */
    /* printf("checking %s\n", utstring_body(path)); */
    /* errno = 0; */
    /* if (access(utstring_body(path), F_OK) == 0) { */
    /*     /\* already processed *\/ */
    /*     return; */
    /* } */

    fflush(bootstrap_FILE);
    fprintf(bootstrap_FILE, "    native.local_repository(\n");
    fprintf(bootstrap_FILE, "        name       = ");

    char *_pkg_prefix = NULL;

    if (_pkg_prefix == NULL)
        fprintf(bootstrap_FILE, "\"%s\",\n", pkg_name);
    else {
        char *s = _pkg_prefix;
        char *tmp;
        while(s) {
            tmp = strchr(s, '/');
            if (tmp == NULL) break;
            *tmp = '.';
            s = tmp;
        }
        fprintf(bootstrap_FILE, "\"%s.%s\",\n",
                _pkg_prefix, pkg_name);
    }

    fprintf(bootstrap_FILE, "        path       = ");

    fprintf(bootstrap_FILE, "\"%s/%s\",\n",
            utstring_body(bzl_switch_pfx),
            pkg_name);

    fprintf(bootstrap_FILE, "    )\n\n");
    fflush(bootstrap_FILE);
}

/* **************************************************************** */
void emit_bazel_hdr(FILE* ostream)
//, int level, char *repo, char *pkg_prefix, obzl_meta_package *_pkg)
{
    fprintf(ostream,
            "load(\"@rules_ocaml//build:rules.bzl\", \"ocaml_import\")\n");

    //FIXME: only if --enable-jsoo passed
    /* fprintf(ostream, */
    /*         "load(\"@rules_jsoo//build:rules.bzl\", \"jsoo_library\", \"jsoo_import\")\n\n"); */

}

/* obzl_meta_values *resolve_setting_values(obzl_meta_setting *_setting, */
/*                                          obzl_meta_flags *_flags, */
/*                                          obzl_meta_settings *_settings) */
/* { */
/* #if defined(PROFILE_fastbuild) */
/*     LOG_DEBUG(0, "resolve_setting_values, opcode: %d", _setting->opcode); */
/* #endif */
/*     obzl_meta_values * vals = obzl_meta_setting_values(_setting); */
/*     /\* LOG_DEBUG(0, "vals ct: %d", obzl_meta_values_count(vals)); *\/ */
/*     if (_setting->opcode == OP_SET) */
/*         return vals; */

/*     /\* else OP_UPDATE *\/ */

/*     UT_array *resolved_values; */
/*     utarray_new(resolved_values, &ut_str_icd); */
/*     utarray_concat(resolved_values, vals->list); */

/*     /\* for each flag, search settings for matching flag *\/ */
/*     int settings_ct = obzl_meta_settings_count(_settings); */
/*     struct obzl_meta_setting *a_setting; */

/*     int flags_ct    = obzl_meta_flags_count(_flags); */
/*     /\* printf("\tflags_ct: %d\n", flags_ct); *\/ */
/*     struct obzl_meta_flag *a_flag = NULL; */

/*     for (int i=0; i < flags_ct; i++) { */
/*         a_flag = obzl_meta_flags_nth(_flags, i); */
/*         for (int j=0; j < settings_ct; j++) { */
/*             a_setting = obzl_meta_settings_nth(_settings, j); */
/*             if (a_setting == _setting) continue; /\* don't match self *\/ */

/*             obzl_meta_flags *setting_flags = obzl_meta_setting_flags(a_setting); */

/*             if (setting_flags == NULL) { */
/*                 /\* always match no flags, e.g. 'requires = "findlib.internal"' *\/ */
/*                 obzl_meta_values *vs = obzl_meta_setting_values(a_setting); */
/*                 /\* printf("xconcatenating\n"); fflush(stdout); fflush(stderr); *\/ */
/*                 utarray_concat(resolved_values, vs->list); */
/*                 continue; */
/*             } */

/*             int ct = obzl_meta_flags_count(setting_flags); */
/*             if (ct > 1) { */
/*                 /\* only try to match singletons? *\/ */
/*                 continue; */
/*             } */

/*             obzl_meta_flag *setting_flag = obzl_meta_flags_nth(setting_flags, 0); */
/*             if (setting_flag->polarity == a_flag->polarity) { */
/*                 if (strncmp(setting_flag->s, a_flag->s, 32) == 0) { */
/* #if defined(PROFILE_fastbuild) */
/*                     LOG_DEBUG(0, "matched flag"); */
/* #endif */
/*                     /\* we have found a setting with exactly one flag, that matches the search flag *\/ */
/*                     /\* now we check the setting's opcode - it should always be SET? *\/ */
/*                     /\* then add its values list *\/ */
/*                     obzl_meta_values *vs = obzl_meta_setting_values(a_setting); */
/*                     utarray_concat(resolved_values, vs->list); */
/*                     /\* dump_setting(4, a_setting); *\/ */
/*                 } */
/*             } */
/*         } */
/*     } */
/*     obzl_meta_values *new_values = (obzl_meta_values*)calloc(sizeof(obzl_meta_values),1); */
/*     new_values->list = resolved_values; */
/*     return new_values; */
/* } */

void emit_bazel_attribute(FILE* ostream,
                          int level, /* indentation control */
                          /* char *_repo, */
                          /* char *_pkg_path, */
                          char *_pkg_prefix,
                          char *_pkg_name,
                          /* for constructing import label: */
                          char *_filedeps_path, /* _lib */
                          /* char *_subpkg_dir, */
                          obzl_meta_entries *_entries,
                          char *property, /* = 'archive' or 'plugin' */
                          obzl_meta_package *_pkg)
{
    (void)_pkg;
#if defined(PROFILE_fastbuild)
    LOG_DEBUG(0, "EMIT_BAZEL_ATTRIBUTE _pkg_name: '%s'; prop: '%s'; filedeps path: '%s'",
              _pkg_name, property, _filedeps_path);
    LOG_DEBUG(0, "  pkg_prefix: %s", _pkg_prefix);
#endif

    /* static UT_string *archive_srcfile; // FIXME: dealloc? */

    /* obzl_meta_property *_prop = obzl_meta_entries_property(_entries,property); */
    /* if (strncmp(_pkg_name, "ppx_sexp_conv", 13) == 0) { */
    /*     char *_prop_name = obzl_meta_property_name(_prop); */
    /*     char *_prop_val = obzl_meta_property_value(_prop); */

    /*     LOG_DEBUG(0, "prop: '%s' == '%s'", */
    /*               _prop_name, _prop_val); */
    /*     LOG_DEBUG(0, "DUMP_PROP"); */
    /*     dump_property(0, _prop); */
    /* } */

    /* LOG_DEBUG(0, "    _pkg_path: %s", _pkg_path); */
    /*
      NOTE: archive/plugin location is relative to the 'directory' property!
      which may be empty, even for subpackages (e.g. opam-lib subpackages)
     */

    /* char *directory = NULL; */
    /* bool stdlib = false; */
    /* struct obzl_meta_property *directory_prop = obzl_meta_entries_property(_entries, "directory"); */
    /* LOG_DEBUG(0, "DIRECTORY: %s", directory); */
    /* LOG_DEBUG(0, " PROP: %s", property); */

    LOG_DEBUG(0, "## _filedeps_path: '%s'\n", _filedeps_path);

    struct obzl_meta_property *deps_prop = obzl_meta_entries_property(_entries, property);

    if ( deps_prop == NULL ) {
        LOG_WARN(0, "Prop '%s' not found: %s.", property, _pkg_name);
        return;
    }

    obzl_meta_settings *settings = obzl_meta_property_settings(deps_prop);

    int settings_ct = obzl_meta_settings_count(settings);
    LOG_INFO(0, "settings count: %d", settings_ct);
    if (settings_ct == 0) {
        LOG_INFO(0, "No settings for %s", obzl_meta_property_name(deps_prop));
        return;
    }

    /* dealing with OP_UDATE.
       e.g. ppx_sexp_conv has three settings:
       requires(ppx_driver) = "ppx_sexp_conv.expander ppxlib"
       requires(-ppx_driver) = "ppx_sexp_conv.runtime-lib"
       ppx(-ppx_driver,-custom_ppx) += "ppx_deriving"

       the 3rd must combine the 2nd because the op on the last is '+='
       condition: no_ppx_driver: ppx_sexp_conv.runtime-lib
       condition: no_ppx_driver_no_custom_ppx: ppx_sexp_conv.runtime-lib, ppx_deriving
       if op == UPDATE then for each flag, search flaglist for match and add vals if found
     */

    /* NB: 'property' will be 'archive' or 'plugin' (?) */
    if (settings_ct > 1) {
        fprintf(ostream, "%*s%s = select({\n", level*spfactor, sp, property);
    } else {
        fprintf(ostream, "%*s%s = [\n", level*spfactor, sp, property);
    }

    UT_string *condition_name;  /* i.e. flag */
    utstring_new(condition_name);

    obzl_meta_setting *setting = NULL;

    obzl_meta_values *vals;
    obzl_meta_value *archive_name = NULL;

    for (int i = 0; i < settings_ct; i++) {
        setting = obzl_meta_settings_nth(settings, i);
        /* LOG_DEBUG(0, "setting[%d]", i+1); */
        /* dump_setting(0, setting); */

        obzl_meta_flags *flags = obzl_meta_setting_flags(setting);

        /* skip any setting with deprecated flag, e.g. 'vm' */
        if (obzl_meta_flags_deprecated(flags)) {
            /* LOG_DEBUG(0, "OMIT attribute with DEPRECATED flag"); */
            continue;
        }

        /* if (flags != NULL) register_flags(flags); */

        /* if (g_ppx_pkg) {        /\* set by opam_bootstrap.handle_lib_meta *\/ */
        /* } */

        bool has_conditions = false;
        if (flags == NULL)
            utstring_printf(condition_name, "//conditions:default");
        else
            has_conditions = obzl_meta_flags_to_selection_label(flags, condition_name);

        if (!has_conditions) {
            goto next;          /* continue does not seem to exit loop */
        }

        /* char *condition_comment = obzl_meta_flags_to_comment(flags); */
        /* LOG_DEBUG(0, "condition_comment: %s", condition_comment); */

        /* FIXME: multiple settings means multiple flags; decide how
           to handle for deps */
        // construct a select expression on the flags
        if (settings_ct > 1) {
            fprintf(ostream, "        \"%s\"%-4s",
                    utstring_body(condition_name), ": [\n");
            /* condition_comment, /\* FIXME: name the condition *\/ */
        }

        /* now we handle UPDATE settings */
        vals = resolve_setting_values(setting, flags, settings);
        LOG_DEBUG(0, "setting values:", "");
        /* dump_values(0, vals); */

        /* now we construct a bazel label for each value */
        UT_string *label;
        utstring_new(label);

        for (int j = 0; j < obzl_meta_values_count(vals); j++) {
            archive_name = obzl_meta_values_nth(vals, j);
            LOG_INFO(0, "prop[%d] '%s' == '%s'",
                     j, property, (char*)*archive_name);

            /* char *s = (char*)*v; */
            /* while (*s) { */
            /*     /\* printf("s: %s\n", s); *\/ */
            /*     if(s[0] == '.') { */
            /*         /\* LOG_INFO(0, "Hit"); *\/ */
            /*         s[0] = '/'; */
            /*     } */
            /*     s++; */
            /* } */
            utstring_clear(label);
            /* utstring_printf(label, "//:%s", _filedeps_path); /\* e.g. _lib *\/ */
            /* LOG_DEBUG(0, "21 _filedeps_path: %s", _filedeps_path); */
            /* LOG_DEBUG(0, "22 _pkg_prefix: %d: %s", _pkg_prefix == NULL, _pkg_prefix); */
            int rc;
            if (_filedeps_path == NULL)
                rc = 1;
            else
                rc = strncmp("ocaml", _filedeps_path, 5);

            /* emit 'archive' attr targets */
            if (rc == 0)    /* i.e. 'ocaml', 'ocaml/ocamldoc' */
                 /* special cases: unix, dynlink, etc. */
                if (strlen(_filedeps_path) == 5) {
                    utstring_printf(label, "@%s//lib:%s",
                                    _filedeps_path, *archive_name);
                } else {
                    rc = strncmp("ocaml-compiler-libs", _filedeps_path, 19);
                    if (rc == 0)
                        /* utstring_printf(label, "ZZZZ@%s:%s", */
                        /*                 _filedeps_path, *archive_name); */
                        utstring_printf(label, ":%s",
                                        *archive_name);
                    else {
                        utstring_printf(label, ":%s",
                                        *archive_name);
                        utstring_printf(label, "ERROR@%s:%s",
                                        _filedeps_path, *archive_name);

                        /* ocaml-syntax-shims, ocamlbuild, etc. */
                    }
                }
            else {
                /* we're constructing archive attribute labels
                   _pkg_name is the name of the import target, not the arch
                */
                if (_pkg_prefix == NULL) {
                    utstring_printf(label,
                                    "@%s//%s", // filedeps path: %s",
                                    /* _pkg_name, */
                                    _filedeps_path,
                                    *archive_name);
                } else {
                    char *start = strchr(_pkg_prefix, '/');
                    int repo_len = start - (char*)_pkg_prefix;
                    if (start == NULL) {
                        utstring_printf(label,
                                        "@%.*s//%s:%s", // || PKG_pfx: %s",
                                        repo_len,
                                        _pkg_prefix,
                                        _pkg_name,
                                        *archive_name);
                                        /* _pkg_prefix); */
                    } else {
                        start++;
                        utstring_printf(label,
                                        "@%.*s//%s/%s:%s", // || PKG_pfx: %s",
                                        repo_len,
                                        _pkg_prefix,
                                        (char*)start,
                                        _pkg_name,
                                        *archive_name);
                                        /* _pkg_prefix); */
                    }

                        /* fprintf(ostream, "%*s\"@%.*s//%s\",\n", */
                        /*         (1+level)*spfactor, sp, */
                        /*         repo_len, */
                        /*         *v, */
                        /*         start+1); */

                }
                /* char *s = (char*)_filedeps_path; */
                /* char *tmp; */
                /* while(s) { */
                /*     tmp = strchr(s, '/'); */
                /*     if (tmp == NULL) break; */
                /*     *tmp = '.'; */
                /*     s = tmp; */
                /* } */
                /* char *start = strrchr(_filedeps_path, '.'); */

                /* if (start == NULL) */
                /*     utstring_printf(label, */
                /*                     "X@%s//%s", */
                /*                     _filedeps_path, *archive_name); */
                /* else { */
                /*     *start++ = '\0'; // split on '.' */
                /*     utstring_printf(label, */
                /*                     "Y@%s//%s:%s", */
                /*                     _filedeps_path, */
                /*                     start, */
                /*                     *archive_name); */
                /* } */
            }

            /* utstring_printf(label, "@rules_ocaml//cfg/:%s/%s", */
            /*                 _filedeps_path, *archive_name); */
            /* LOG_DEBUG(0, "label 2: %s", utstring_body(label)); */

            // FIXME: verify existence using access()?
            if (settings_ct > 1) {
                    fprintf(ostream, "            \"%s\",\n", utstring_body(label));
            } else {
                    fprintf(ostream, "\"%s\",\n", utstring_body(label));
            }

            /* if v is .cmxa, then add .a too? */
            /* int cmxapos = utstring_findR(label, -1, "cmxa", 4); */
            /* if (cmxapos > 0) { */
            /*     char *cmxa_a = strdup(utstring_body(label)); */
            /*     cmxa_a[utstring_len(label) - 4] = 'a'; */
            /*     cmxa_a[utstring_len(label) - 3] = '\0'; */

            /*     // stdlib-shims is a special case, with empty */
            /*     // stdlib_shims.cmxa and missing stdlib_shims.a; see */
            /*     // https://github.com/ocaml/ocaml/pull/9011 */
            /*     // so we need to test existence */
            /*     utstring_renew(archive_srcfile); */
            /*     // drop leading ':' from basename */
            /*     char * bn = basename(cmxa_a); */

            /*     /\* utstring_printf(archive_srcfile, "%s/%s", *\/ */
            /*     /\*                 _pkg->path, bn); *\/ */

            /*     /\* if (access(utstring_body(archive_srcfile), F_OK) == 0) *\/ */
            /*     /\*     fprintf(ostream, "            \"%s\",\n", cmxa_a); *\/ */
            /*     /\* else *\/ */
            /*     /\*     if (coswitch_debug) { *\/ */
            /*     /\*         LOG_DEBUG(0, "## missing %s\n", *\/ */
            /*     /\*                   utstring_body(archive_srcfile)); *\/ */
            /*     /\*     } *\/ */
            /* } */
            /* if (flags != NULL) /\* skip mt, mt_vm, mt_posix *\/ */
            /*     fprintf(ostream, "%s", "        ],\n"); */
        }
        utstring_free(label);
        if (settings_ct > 1) {
            /* fprintf(ostream, "\n"); // , (1+level)*spfactor, sp); */
            fprintf(ostream, "%*s],\n", (1+level)*spfactor, sp);
        }
        /* free(condition_comment); */
    next:
        ;
    }
    utstring_free(condition_name);
    if (settings_ct > 1)
        fprintf(ostream, "%*s}),\n", level*spfactor, sp);
    else
        fprintf(ostream, "%*s],\n", level*spfactor, sp);
}

void emit_bazel_cc_imports(FILE* ostream,
                           int level, /* indentation control */
                           char *_pkg_prefix,
                           char *_pkg_name,
                           char *_filedeps_path, /* _lib */
                           obzl_meta_entries *_entries,
                           obzl_meta_package *_pkg)
{
    (void)level;
    (void)_entries;
    (void)_filedeps_path;
    (void)_pkg;
    (void)_pkg_name;
    (void)_pkg_prefix;
    char *dname = dirname(utstring_body(build_bazel_file));
    /* LOG_DEBUG(0, "emit_bazel_cc_imports: %s", dname); */
    /* printf("emit_bazel_cc_imports: %s\n", dname); */
    /* LOG_DEBUG(0, "dname: %s\n", dname); */

    errno = 0;
    DIR *d = opendir(dname);
    if (d == NULL) {
        fprintf(stderr,
                "ERROR: bad opendir: %s\n", strerror(errno));
        return;
    }

    /* bool wrote_loader = false; */

    /* RASH ASSUMPTION: only one stublib per directory */
    struct dirent *direntry;
    while ((direntry = readdir(d)) != NULL) {
        if ((direntry->d_type==DT_REG)
            || (direntry->d_type==DT_LNK)) {
            if (fnmatch("lib*stubs.a", direntry->d_name, 0) == 0) {
                /* printf("FOUND STUBLIB: %s\n", direntry->d_name); */

                fprintf(ostream, "cc_import(\n");
                fprintf(ostream, "    name           = \"_%s\",\n",
                        direntry->d_name);
                fprintf(ostream, "    static_library = \"%s\",\n",
                        direntry->d_name);
                fprintf(ostream, ")\n");
            } else {
                if (fnmatch("*stubs.so", direntry->d_name, 0) == 0) {
                    printf("FOUND SO LIB: %s\n", direntry->d_name);
            }
                /* printf("skipping %s\n", direntry->d_name); */
            }
        }
    }
    closedir(d);
}

UT_array *cc_stubs;

void emit_bazel_stublibs_attr(FILE* ostream,
                              char *coswitch_lib,
                              int level, /* indentation lvl */
                              char *_pkg_root,
                              char *_pkg_prefix,
                              char *_pkg_name,
                              UT_string *_pkg_parent,
                              char *_filedeps_path, /* _lib */
                              obzl_meta_entries *_entries,
                              obzl_meta_package *_pkg)
{
    (void)_entries;
    (void)_filedeps_path;
    (void)_pkg;
    /* here we read the symlinks in the coswitch not the opam switch */
#if defined(PROFILE_fastbuild)
    LOG_DEBUG(0, "stublibs pkg root: '%s'", _pkg_root);
    LOG_DEBUG(0, "stublibs pkg prefix: '%s'", _pkg_prefix);
    LOG_DEBUG(0, "stublibs pkg name: '%s'", _pkg_name);
    LOG_DEBUG(0, "stublibs pkg parent: '%s'", utstring_body(_pkg_parent));
#endif

    static UT_string *dname;
    utstring_new(dname);
    utstring_printf(dname, "%s/%s/lib",
                    coswitch_lib, /* e.g. <switch>/lib */
                    /* FIXME: what about multilevels, e.g.
                     @foo//lib/bar/baz/buz ? */
                    _pkg_root);
    if (_pkg_parent != NULL) {
        if (utstring_len(_pkg_parent) > 0) {
            /* LOG_DEBUG(0, "parent: %s", utstring_body(_pkg_parent)); */
            utstring_printf(dname, "/%s", utstring_body(_pkg_parent));
        }
    }
    /* LOG_DEBUG(0, "dname: %s", utstring_body(dname)); */
    /* LOG_DEBUG(0, "pkgname: %s", _pkg_name); */
    utstring_printf(dname, "/%s", _pkg_name);

#if defined(PROFILE_fastbuild)
    LOG_DEBUG(0, "emit_bazel_stublibs_attr: %s",
              utstring_body(dname));
#endif

    errno = 0;
    DIR *d = opendir(utstring_body(dname));
    if (d == NULL) {
        fprintf(stderr,
                "ERROR: bad opendir: %s\n", strerror(errno));
        fprintf(ostream, "## ERROR: bad opendir: %s\n", utstring_body(dname));
        fprintf(ostream, "## pkg root: %s\n", _pkg_root);
        fprintf(ostream, "## pkg parent: %s\n", utstring_body(_pkg_parent));
        fprintf(ostream, "## _pkg_prefix: %s\n", _pkg_prefix);
        fprintf(ostream, "## _pkg_name: %s\n", _pkg_name);
        return;
    }

    /* bool wrote_loader = false; */

    utarray_new(cc_stubs, &ut_str_icd);
    /* char *s; */
    /* s = "hello"; utarray_push_back(cc_stubs, &s); */
    /* s = "world"; utarray_push_back(cc_stubs, &s); */

    char strbuf[128];
    char *sbuf = (char*)&strbuf;
    /* char *foo = "foo"; */
    struct dirent *direntry;
    int name_len;

    while ((direntry = readdir(d)) != NULL) {
        if ((direntry->d_type==DT_REG)
            || (direntry->d_type==DT_LNK)) {
            if (fnmatch("lib*stubs.a", direntry->d_name, 0) == 0) {

                /* fprintf(ostream, "%*scc_deps   = [\":_%s\"],\n", */
                /*         level*spfactor, sp, direntry->d_name); */

                name_len = strlen(direntry->d_name);
                memcpy(strbuf, direntry->d_name, 128);
                strbuf[name_len] = '\0';
                /* fprintf(ostream, "X %s\n", strbuf); */

                /* printf("STUBLIB: %s\n", strbuf); */
                /* printf("stublib: %s\n", direntry->d_name); */
                utarray_push_back(cc_stubs, &sbuf);

            } else {
                if (fnmatch("*stubs.so", direntry->d_name, 0) == 0) {
                    printf("FOUND SO STUBLIB: %s\n", direntry->d_name);
            }
                /* printf("skipping %s\n", direntry->d_name); */
            }
        }
    }

    char **p;
    p = NULL;
    fprintf(ostream, "%*scc_deps   = [\n", level*spfactor, sp);
    while ( (p=(char**)utarray_next(cc_stubs, p)) ) {
        fprintf(ostream, "        \":_%s\",\n", *p);
    }
    fprintf(ostream, "%*s],\n", level*spfactor, sp);

    utarray_free(cc_stubs);

    closedir(d);
}

//FIXME: only if --enable-jsoo passed
void emit_bazel_jsoo_runtime_attr(FILE* ostream,
                                  char *coswitch_lib,
                                  int level, /* indentation control */
                                  char *_pkg_root,
                                  char *_pkg_prefix,
                                  char *_pkg_name,
                                  UT_string *_pkg_parent,
                                  char *_filedeps_path, /* _lib */
                                  obzl_meta_entries *_entries,
                                  obzl_meta_package *_pkg)
{
    (void)_entries;
    (void)_filedeps_path;
    (void)_pkg;
    /* here we read the symlinks in the coswitch not the opam switch */
#if defined(PROFILE_fastbuild)
    LOG_DEBUG(0, "jsoo pkg root: %s", _pkg_root);
    LOG_DEBUG(0, "jsoo pkg prefix: %s", _pkg_prefix);
    LOG_DEBUG(0, "jsoo pkg name: %s", _pkg_name);
    LOG_DEBUG(0, "jsoo pkg parent: %s", utstring_body(_pkg_parent));
#endif

    static UT_string *dname;
    utstring_new(dname);
    utstring_printf(dname, "%s/%s/lib",
                    coswitch_lib, /* e.g. <switch>/lib */
                    /* FIXME: what about multilevels, e.g.
                     @foo//lib/bar/baz/buz ? */
                    _pkg_root);
    if (_pkg_parent != NULL)
        utstring_printf(dname, "/%s", utstring_body(_pkg_parent));
    utstring_printf(dname, "/%s", _pkg_name);

#if defined(PROFILE_fastbuild)
    LOG_DEBUG(0, "emit_bazel_jsoo_runtime_attr: %s", utstring_body(dname));
#endif

    errno = 0;
    DIR *d = opendir(utstring_body(dname));
    if (d == NULL) {
        fprintf(stderr,
                "ERROR: bad opendir: %s\n", strerror(errno));
        fprintf(ostream, "## ERROR: bad opendir: %s\n", utstring_body(dname));
        fprintf(ostream, "## pkg root: %s\n", _pkg_root);
        fprintf(ostream, "## pkg parent: %s\n", utstring_body(_pkg_parent));
        fprintf(ostream, "## _pkg_prefix: %s\n", _pkg_prefix);
        fprintf(ostream, "## _pkg_name: %s\n", _pkg_name);
        return;
    }

    /* bool wrote_loader = false; */

    /* TODO: read jsoo_runtime property of META */

    struct dirent *direntry;
    while ((direntry = readdir(d)) != NULL) {
        if ((direntry->d_type==DT_REG)
            || (direntry->d_type==DT_LNK)) {
            if (fnmatch("runtime.js", direntry->d_name, 0) == 0) {
                fprintf(ostream, "%*sjsoo_runtime = \"%s\",\n",
                        level*spfactor, sp, direntry->d_name);

                /* fprintf(ostream, "cc_import(\n"); */
                /* fprintf(ostream, "    name    = \"%s_stubs\",\n", */
                /*         _pkg_name); */
                /* fprintf(ostream, "    archive = \"%s\",\n", */
                /*         direntry->d_name); */
                /* fprintf(ostream, ")\n"); */
            /* } */
                /* printf("skipping %s\n", direntry->d_name); */
            }
        }
    }
    closedir(d);
}

void emit_bazel_archive_attr(FILE* ostream,
                             int level, /* indentation control */
                             /* char *_repo, */
                             /* char *_pkg_path, */
                             char *_pkg_prefix,
                             char *_pkg_name,
                             /* for constructing import label: */
                             char *_filedeps_path, /* _lib */
                             /* char *_subpkg_dir, */
                             obzl_meta_entries *_entries,
                             char *property, /* always 'archive' */
                             obzl_meta_package *_pkg)
{
    (void)level;
    (void)_pkg;
    (void)_pkg_name;
    (void)_filedeps_path;

    LOG_DEBUG(0, "EMIT_BAZEL_ARCHIVE_ATTR _pkg_name: '%s'; prop: '%s'; filedeps path: '%s'",
              _pkg_name, property, _filedeps_path);
    LOG_DEBUG(0, "  pkg_prefix: %s", _pkg_prefix);

    /* static UT_string *archive_srcfile; // FIXME: dealloc? */

    LOG_DEBUG(0, "_filedeps_path: '%s'", _filedeps_path);

    struct obzl_meta_property *deps_prop = obzl_meta_entries_property(_entries, property);
    if ( deps_prop == NULL ) {
        LOG_WARN(0, "Prop '%s' not found: %s.", property); //, pkg_name);
        return;
    }
    /*
      archive(byte) = "oUnit.cma"
      archive(native) = "oUnit.cmxa"
    */

    obzl_meta_settings *settings = obzl_meta_property_settings(deps_prop);

    int settings_ct = obzl_meta_settings_count(settings);
#if defined(PROFILE_fastbuild)
    LOG_INFO(0, "settings count: %d", settings_ct);
#endif
    if (settings_ct == 0) {
#if defined(PROFILE_fastbuild)
        LOG_INFO(0, "No settings for %s", obzl_meta_property_name(deps_prop));
#endif
        return;
    }
    UT_string *cmtag;  /* cma or cmxa */
    utstring_new(cmtag);

    obzl_meta_setting *setting = NULL;

    obzl_meta_values *vals;
    obzl_meta_value *archive_name = NULL;

    /* lhs */
    fprintf(ostream, "    archive  =  select({\n");
            //level*spfactor, sp,
            /* property); */

    /* iter over archive(byte), archive(native) */
    for (int i = 0; i < settings_ct; i++) {
        setting = obzl_meta_settings_nth(settings, i);
#if defined(PROFILE_fastbuild)
        LOG_DEBUG(0, "setting[%d]", i+1);
#endif
        /* dump_setting(0, setting); */

        obzl_meta_flags *flags = obzl_meta_setting_flags(setting);
        /* for archive: 'byte', 'native' */

        /* skip any setting with deprecated flag, e.g. 'vm' */
        if (obzl_meta_flags_deprecated(flags)) {
            /* LOG_DEBUG(0, "OMIT attribute with DEPRECATED flag"); */
            continue;
        }

        /* if (flags != NULL) register_flags(flags); */

        /* if (g_ppx_pkg) {        /\* set by opam_bootstrap.handle_lib_meta *\/ */
        /* } */

        vals = resolve_setting_values(setting, flags, settings);

        bool has_conditions = false;
        if (flags == NULL)
            utstring_printf(cmtag, "//conditions:default");
        else
            /* updates cmtag */
            /* maps: 'byte' => 'cma'; 'native' => 'cmxa' */
            has_conditions = obzl_meta_flags_to_cmtag(flags, cmtag);

        if (!has_conditions) {
            goto next;          /* continue does not seem to exit loop */
        }
/* #if defined(PROFILE_fastbuild) */
/*         LOG_DEBUG(0, "CMTAG: %s", utstring_body(cmtag)); */
/* #endif */
        if (settings_ct > 1) {
            if (obzl_meta_values_count(vals) > 0) {
                if (strncmp(utstring_body(cmtag), "cma", 4) == 0) {
                    fprintf(ostream,
                            "        "
                            "\"@ocaml//platform/emitter:vm\" :");
                    /* level*spfactor + 4, sp, */
                    /* "@ocaml//platform/emitter:vm"); */
                } else {
                    fprintf(ostream,
                            "        "
                            "\"@ocaml//platform/emitter:sys\":");

                    /* "\"//conditions:default\":        "); */
                    /* level*spfactor + 4, sp, */
                    /* "//conditions:default"); */
                }
                /* fprintf(ostream, "%*s%s", level*spfactor, sp, "archive"); */
                /* fprintf(ostream, "%*s%s", level*spfactor, sp, */
                /*         utstring_body(cmtag)); */
            }
        }
        //else???

        /* now we handle UPDATE settings */
        /* vals = resolve_setting_values(setting, flags, settings); */
        LOG_DEBUG(0, "setting values:", "");
        /* dump_values(0, vals); */

        /* now we construct a bazel label for each value */
        UT_string *label;
        utstring_new(label);
/* #if defined(PROFILE_fastbuild) */
/*         LOG_DEBUG(0, "vals ct: %d", obzl_meta_values_count(vals)); */
/* #endif */
        /* if (obzl_meta_values_count(vals) < 1) { */
        /*     fprintf(ostream, "XXXXXXXXXXXXXXXX\n"); */
        /* } */

        /* for archive, should be only 1 value */
        /* if (obzl_meta_values_count(vals) < 1) { */
        /*     /\* fprintf(ostream, "  None\n"); // e.g. pkg core.top is vm only *\/ */
        /* } else { */
        if (obzl_meta_values_count(vals) > 0) {
            for (int j = 0; j < obzl_meta_values_count(vals); j++) {
                archive_name = obzl_meta_values_nth(vals, j);
#if defined(PROFILE_fastbuild)
                LOG_INFO(0, "prop[%d] '%s' == '%s'",
                         j, property, (char*)*archive_name);
#endif
                utstring_clear(label);
                if (_pkg_prefix == NULL) {
                    utstring_printf(label,
                                    "%s",
                                    /* "@%s//%s", // filedeps path: %s", */
                                    /* /\* _pkg_name, *\/ */
                                    /* _filedeps_path, */
                                    *archive_name);
                } else {
                    char *start = strchr(_pkg_prefix, '/');
                    /* int repo_len = start - (char*)_pkg_prefix; */
                    if (start == NULL) {
                        utstring_printf(label,
                                        "%s",
                                        /* "@%.*s//%s:%s", // || PKG_pfx: %s", */
                                        /* repo_len, */
                                        /* _pkg_prefix, */
                                        /* _pkg_name, */
                                        *archive_name);
                        /* _pkg_prefix); */
                    } else {
                        start++;
                        utstring_printf(label,
                                        "%s",
                                        /* "@%.*s//%s/%s:%s", */
                                        /* repo_len, */
                                        /* _pkg_prefix, */
                                        /* (char*)start, */
                                        /* _pkg_name, */
                                        *archive_name);
                        /* _pkg_prefix); */
                    }

                    /* fprintf(ostream, "%*s\"@%.*s//%s\",\n", */
                    /*         (1+level)*spfactor, sp, */
                    /*         repo_len, */
                    /*         *v, */
                    /*         start+1); */

                }
                // FIXME: verify existence using access()?
                int indent = 3;
                if (strncmp(utstring_body(cmtag), "cmxa", 4) == 0)
                    indent--;
                if (settings_ct > 1) {
                    fprintf(ostream, " \"%s\",\n",
                            /* %*s */
                            /* indent, sp, */
                            utstring_body(label));
                } else {
                    fprintf(ostream, "5) %*s = \"%s\",\n",
                            indent, sp, utstring_body(label));
                }
            }
        }
        utstring_free(label);
    next:
        ;
    }
    fprintf(ostream,
            "    }, no_match_error=\"Bad platform\"),\n");
}

void emit_bazel_codeps_attr(FILE* ostream,
                             int level, /* indentation control */
                             /* char *_repo, */
                             /* char *_pkg_path, */
                             char *_pkg_prefix,
                             char *_pkg_name,
                             /* for constructing import label: */
                             char *_filedeps_path, /* _lib */
                             /* char *_subpkg_dir, */
                             obzl_meta_entries *_entries,
                            char *property, //'ppx_runtime_deps'
                            obzl_meta_package *_pkg)
{
    (void)level;
    (void)_pkg;
    (void)_pkg_name;
    (void)_filedeps_path;

    LOG_DEBUG(0, "EMIT_BAZEL_CODEPS_ATTR _pkg_name: '%s'; prop: '%s'; filedeps path: '%s'",
              _pkg_name, property, _filedeps_path);
    LOG_DEBUG(0, "  pkg_prefix: %s", _pkg_prefix);

    /* static UT_string *archive_srcfile; // FIXME: dealloc? */

    LOG_DEBUG(0, "_filedeps_path: '%s'", _filedeps_path);

    struct obzl_meta_property *deps_prop = obzl_meta_entries_property(_entries, property);
    if ( deps_prop == NULL ) {
        LOG_WARN(0, "Prop '%s' not found: %s.", property); //, pkg_name);
        return;
    }
    /*
      archive(byte) = "oUnit.cma"
      archive(native) = "oUnit.cmxa"
    */

    obzl_meta_settings *settings = obzl_meta_property_settings(deps_prop);

    int settings_ct = obzl_meta_settings_count(settings);
#if defined(PROFILE_fastbuild)
    LOG_INFO(0, "settings count: %d", settings_ct);
#endif
    if (settings_ct == 0) {
#if defined(PROFILE_fastbuild)
        LOG_INFO(0, "No settings for %s", obzl_meta_property_name(deps_prop));
#endif
        return;
    }
    UT_string *cmtag;  /* cma or cmxa */
    utstring_new(cmtag);

    obzl_meta_setting *setting = NULL;

    obzl_meta_values *vals;
    obzl_meta_value *archive_name = NULL;

    /* lhs */
    fprintf(ostream, "    archive  =  select({\n");
            //level*spfactor, sp,
            /* property); */

    /* iter over archive(byte), archive(native) */
    for (int i = 0; i < settings_ct; i++) {
        setting = obzl_meta_settings_nth(settings, i);
#if defined(PROFILE_fastbuild)
        LOG_DEBUG(0, "setting[%d]", i+1);
#endif
        /* dump_setting(0, setting); */

        obzl_meta_flags *flags = obzl_meta_setting_flags(setting);
        /* for archive: 'byte', 'native' */

        /* skip any setting with deprecated flag, e.g. 'vm' */
        if (obzl_meta_flags_deprecated(flags)) {
            /* LOG_DEBUG(0, "OMIT attribute with DEPRECATED flag"); */
            continue;
        }

        /* if (flags != NULL) register_flags(flags); */

        /* if (g_ppx_pkg) {        /\* set by opam_bootstrap.handle_lib_meta *\/ */
        /* } */

        vals = resolve_setting_values(setting, flags, settings);

        bool has_conditions = false;
        if (flags == NULL)
            utstring_printf(cmtag, "//conditions:default");
        else
            /* updates cmtag */
            /* maps: 'byte' => 'cma'; 'native' => 'cmxa' */
            has_conditions = obzl_meta_flags_to_cmtag(flags, cmtag);

        if (!has_conditions) {
            goto next;          /* continue does not seem to exit loop */
        }
/* #if defined(PROFILE_fastbuild) */
/*         LOG_DEBUG(0, "CMTAG: %s", utstring_body(cmtag)); */
/* #endif */
        if (settings_ct > 1) {
            if (obzl_meta_values_count(vals) > 0) {
                if (strncmp(utstring_body(cmtag), "cma", 4) == 0) {
                    fprintf(ostream,
                            "        "
                            "\"@ocaml//platform/emitter:vm\" :");
                    /* level*spfactor + 4, sp, */
                    /* "@ocaml//platform/emitter:vm"); */
                } else {
                    fprintf(ostream,
                            "        "
                            "\"@ocaml//platform/emitter:sys\":");

                    /* "\"//conditions:default\":        "); */
                    /* level*spfactor + 4, sp, */
                    /* "//conditions:default"); */
                }
                /* fprintf(ostream, "%*s%s", level*spfactor, sp, "archive"); */
                /* fprintf(ostream, "%*s%s", level*spfactor, sp, */
                /*         utstring_body(cmtag)); */
            }
        }
        //else???

        /* now we handle UPDATE settings */
        /* vals = resolve_setting_values(setting, flags, settings); */
        LOG_DEBUG(0, "setting values:", "");
        /* dump_values(0, vals); */

        /* now we construct a bazel label for each value */
        UT_string *label;
        utstring_new(label);
/* #if defined(PROFILE_fastbuild) */
/*         LOG_DEBUG(0, "vals ct: %d", obzl_meta_values_count(vals)); */
/* #endif */
        /* if (obzl_meta_values_count(vals) < 1) { */
        /*     fprintf(ostream, "XXXXXXXXXXXXXXXX\n"); */
        /* } */

        /* for archive, should be only 1 value */
        /* if (obzl_meta_values_count(vals) < 1) { */
        /*     /\* fprintf(ostream, "  None\n"); // e.g. pkg core.top is vm only *\/ */
        /* } else { */
        if (obzl_meta_values_count(vals) > 0) {
            for (int j = 0; j < obzl_meta_values_count(vals); j++) {
                archive_name = obzl_meta_values_nth(vals, j);
#if defined(PROFILE_fastbuild)
                LOG_INFO(0, "prop[%d] '%s' == '%s'",
                         j, property, (char*)*archive_name);
#endif
                utstring_clear(label);
                if (_pkg_prefix == NULL) {
                    utstring_printf(label,
                                    "%s",
                                    /* "@%s//%s", // filedeps path: %s", */
                                    /* /\* _pkg_name, *\/ */
                                    /* _filedeps_path, */
                                    *archive_name);
                } else {
                    char *start = strchr(_pkg_prefix, '/');
                    /* int repo_len = start - (char*)_pkg_prefix; */
                    if (start == NULL) {
                        utstring_printf(label,
                                        "%s",
                                        /* "@%.*s//%s:%s", // || PKG_pfx: %s", */
                                        /* repo_len, */
                                        /* _pkg_prefix, */
                                        /* _pkg_name, */
                                        *archive_name);
                        /* _pkg_prefix); */
                    } else {
                        start++;
                        utstring_printf(label,
                                        "%s",
                                        /* "@%.*s//%s/%s:%s", */
                                        /* repo_len, */
                                        /* _pkg_prefix, */
                                        /* (char*)start, */
                                        /* _pkg_name, */
                                        *archive_name);
                        /* _pkg_prefix); */
                    }

                    /* fprintf(ostream, "%*s\"@%.*s//%s\",\n", */
                    /*         (1+level)*spfactor, sp, */
                    /*         repo_len, */
                    /*         *v, */
                    /*         start+1); */

                }
                // FIXME: verify existence using access()?
                int indent = 3;
                if (strncmp(utstring_body(cmtag), "cmxa", 4) == 0)
                    indent--;
                if (settings_ct > 1) {
                    fprintf(ostream, " \"%s\",\n",
                            /* %*s */
                            /* indent, sp, */
                            utstring_body(label));
                } else {
                    fprintf(ostream, "5) %*s = \"%s\",\n",
                            indent, sp, utstring_body(label));
                }
            }
        }
        utstring_free(label);
    next:
        ;
    }
    fprintf(ostream,
            "    }, no_match_error=\"Bad platform\"),\n");
}

void emit_bazel_cmxs_attr(FILE* ostream,
                          int level, /* indentation control */
                          /* char *_repo, */
                          /* char *_pkg_path, */
                          char *_pkg_prefix,
                          char *_pkg_name,
                          /* for constructing import label: */
                          char *_filedeps_path, /* _lib */
                          /* char *_subpkg_dir, */
                          obzl_meta_entries *_entries,
                          char *property, /* = 'archive' or 'plugin' */
                          obzl_meta_package *_pkg)
{
    TRACE_ENTRY;
    (void)_pkg;
    (void)_pkg_name;
    (void)_filedeps_path;
#if defined(PROFILE_fastbuild)
    LOG_DEBUG(0, BLU "EMIT_BAZEL_CMXS_ATTR" CRESET, "");
    LOG_DEBUG(0, "\t_pkg_name: '%s'", _pkg_name);
    LOG_DEBUG(0, "\tprop: '%s'", property);
    LOG_DEBUG(0, "\tfiledeps path: '%s'",  _filedeps_path);
    LOG_DEBUG(0, "\tpkg_prefix: %s", _pkg_prefix);
#endif

    /* static UT_string *archive_srcfile; // FIXME: dealloc? */

    /* obzl_meta_property *_prop = obzl_meta_entries_property(_entries,property); */
    /* if (strncmp(_pkg_name, "ppx_sexp_conv", 13) == 0) { */
    /*     char *_prop_name = obzl_meta_property_name(_prop); */
    /*     char *_prop_val = obzl_meta_property_value(_prop); */

    /*     LOG_DEBUG(0, "prop: '%s' == '%s'", */
    /*               _prop_name, _prop_val); */
    /*     LOG_DEBUG(0, "DUMP_PROP"); */
    /*     dump_property(0, _prop); */
    /* } */

    /* LOG_DEBUG(0, "    _pkg_path: %s", _pkg_path); */
    /*
      NOTE: archive/plugin location is relative to the 'directory' property!
      which may be empty, even for subpackages (e.g. opam-lib subpackages)
     */

    /* char *directory = NULL; */
    /* bool stdlib = false; */
    /* struct obzl_meta_property *directory_prop = obzl_meta_entries_property(_entries, "directory"); */
    /* LOG_DEBUG(0, "DIRECTORY: %s", directory); */
    /* LOG_DEBUG(0, " PROP: %s", property); */

    struct obzl_meta_property *deps_prop = obzl_meta_entries_property(_entries, property);
    if ( deps_prop == NULL ) {
        LOG_WARN(0, "Prop '%s' not found: %s.", property); //, pkg_name);
        return;
    }

    obzl_meta_settings *settings = obzl_meta_property_settings(deps_prop);

    int settings_ct = obzl_meta_settings_count(settings);
#if defined(PROFILE_fastbuild)
    LOG_INFO(0, "settings count: %d", settings_ct);
#endif
    if (settings_ct == 0) {
#if defined(PROFILE_fastbuild)
        LOG_INFO(0, "No settings for %s", obzl_meta_property_name(deps_prop));
#endif
        return;
    }

    /* dealing with OP_UDATE.
       e.g. ppx_sexp_conv has three settings:
       requires(ppx_driver) = "ppx_sexp_conv.expander ppxlib"
       requires(-ppx_driver) = "ppx_sexp_conv.runtime-lib"
       ppx(-ppx_driver,-custom_ppx) += "ppx_deriving"

       the 3rd must combine the 2nd because the op on the last is '+='
       condition: no_ppx_driver: ppx_sexp_conv.runtime-lib
       condition: no_ppx_driver_no_custom_ppx: ppx_sexp_conv.runtime-lib, ppx_deriving
       if op == UPDATE then for each flag, search flaglist for match and add vals if found
     */

    /* NB: 'property' will be 'archive' or 'plugin' (?) */
    /* if (settings_ct > 1) { */
    /*     fprintf(ostream, "%*s%s = select({\n", level*spfactor, sp, property); */
    /* } else { */
    /*     fprintf(ostream, "%*s%s = [\n", level*spfactor, sp, property); */
    /* } */

    fprintf(ostream, "%*scmxs", level*spfactor, sp);

    UT_string *cmtag;  /* i.e. flag */
    utstring_new(cmtag);

    obzl_meta_setting *setting = NULL;

    obzl_meta_values *vals;
    obzl_meta_value *archive_name = NULL;

    for (int i = 0; i < settings_ct; i++) {
        setting = obzl_meta_settings_nth(settings, i);
        /* LOG_DEBUG(0, "setting[%d]", i+1); */
        /* dump_setting(0, setting); */

        obzl_meta_flags *flags = obzl_meta_setting_flags(setting);

        /* skip any setting with deprecated flag, e.g. 'vm' */
        if (obzl_meta_flags_deprecated(flags)) {
            /* LOG_DEBUG(0, "OMIT attribute with DEPRECATED flag"); */
            continue;
        }

        /* if (flags != NULL) register_flags(flags); */

        /* if (g_ppx_pkg) {        /\* set by opam_bootstrap.handle_lib_meta *\/ */
        /* } */

        bool has_conditions = false;
        if (flags == NULL)
            utstring_printf(cmtag, "//conditions:default");
        else
            has_conditions = obzl_meta_flags_to_cmtag(flags, cmtag);
            /* has_conditions = obzl_meta_flags_to_selection_label(flags, cmtag); */

        if (!has_conditions) {
            goto next;          /* continue does not seem to exit loop */
        }

        /* char *condition_comment = obzl_meta_flags_to_comment(flags); */
        /* LOG_DEBUG(0, "condition_comment: %s", condition_comment); */

        /* FIXME: multiple settings means multiple flags; decide how
           to handle for deps */
        // construct a select expression on the flags
        /* if (settings_ct > 1) { */
        /*     fprintf(ostream, "        \"%s\"%-4s", */
        /*             utstring_body(cmtag), ": [\n"); */
        /*     /\* condition_comment, /\\* FIXME: name the condition *\\/ *\/ */
        /* } */

        /* now we handle UPDATE settings */
        vals = resolve_setting_values(setting, flags, settings);
#if defined(PROFILE_fastbuild)
        LOG_DEBUG(0, "setting values:", "");
#endif
        /* dump_values(0, vals); */

        /* now we construct a bazel label for each value */
        UT_string *label;
        utstring_new(label);

        for (int j = 0; j < obzl_meta_values_count(vals); j++) {
            archive_name = obzl_meta_values_nth(vals, j);
#if defined(PROFILE_fastbuild)
            LOG_INFO(0, "\tprop[%d] '%s' == '%s'",
                     j, property, (char*)*archive_name);
#endif

            /* char *s = (char*)*v; */
            /* while (*s) { */
            /*     /\* printf("s: %s\n", s); *\/ */
            /*     if(s[0] == '.') { */
            /*         /\* LOG_INFO(0, "Hit"); *\/ */
            /*         s[0] = '/'; */
            /*     } */
            /*     s++; */
            /* } */
            utstring_clear(label);
            /* utstring_printf(label, "//%s", _filedeps_path); /\* e.g. _lib *\/ */
            /* LOG_DEBUG(0, "21 _filedeps_path: %s", _filedeps_path); */
            /* LOG_DEBUG(0, "22 _pkg_prefix: %d: %s", _pkg_prefix == NULL, _pkg_prefix); */
            /* int rc; */
            /* if (_filedeps_path == NULL) */
            /*     rc = 1; */
            /* else */
            /*     rc = strncmp("ocaml", _filedeps_path, 5); */

            /* /\* emit 'archive' attr targets *\/ */
            /* if (rc == 0)    /\* i.e. 'ocaml', 'ocaml/ocamldoc' *\/ */
            /*      /\* special cases: unix, dynlink, etc. *\/ */
            /*     if (strlen(_filedeps_path) == 5) { */
            /*         utstring_printf(label, "@%s//lib:%s", */
            /*                         _filedeps_path, *archive_name); */
            /*     } else { */
            /*         rc = strncmp("ocaml-compiler-libs", _filedeps_path, 19); */
            /*         if (rc == 0) */
            /*             /\* utstring_printf(label, "ZZZZ@%s:%s", *\/ */
            /*             /\*                 _filedeps_path, *archive_name); *\/ */
            /*             utstring_printf(label, ":%s", */
            /*                             *archive_name); */
            /*         else { */
            /*             utstring_printf(label, ":%s", */
            /*                             *archive_name); */
            /*             utstring_printf(label, "ERROR@%s:%s", */
            /*                             _filedeps_path, *archive_name); */

            /*             /\* ocaml-syntax-shims, ocamlbuild, etc. *\/ */
            /*         } */
            /*     } */
            /* else { */
                /* we're constructing archive attribute labels
                   _pkg_name is the name of the import target, not the arch
                */
                if (_pkg_prefix == NULL) {
                    utstring_printf(label,
                                    "%s",
                                    /* "@%s//%s", // filedeps path: %s", */
                                    /* /\* _pkg_name, *\/ */
                                    /* _filedeps_path, */
                                    *archive_name);
                } else {
                    char *start = strchr(_pkg_prefix, '/');
                    /* int repo_len = start - (char*)_pkg_prefix; */
                    if (start == NULL) {
                        utstring_printf(label,
                                        "%s",
                                        /* "@%.*s//%s:%s", // || PKG_pfx: %s", */
                                        /* repo_len, */
                                        /* _pkg_prefix, */
                                        /* _pkg_name, */
                                        *archive_name);
                                        /* _pkg_prefix); */
                    } else {
                        start++;
                        utstring_printf(label,
                                        "%s",
                                        /* "@%.*s//%s/%s:%s", // || PKG_pfx: %s", */
                                        /* repo_len, */
                                        /* _pkg_prefix, */
                                        /* (char*)start, */
                                        /* _pkg_name, */
                                        *archive_name);
                                        /* _pkg_prefix); */
                    }

                        /* fprintf(ostream, "%*s\"@%.*s//%s\",\n", */
                        /*         (1+level)*spfactor, sp, */
                        /*         repo_len, */
                        /*         *v, */
                        /*         start+1); */

                }
                /* char *s = (char*)_filedeps_path; */
                /* char *tmp; */
                /* while(s) { */
                /*     tmp = strchr(s, '/'); */
                /*     if (tmp == NULL) break; */
                /*     *tmp = '.'; */
                /*     s = tmp; */
                /* } */
                /* char *start = strrchr(_filedeps_path, '.'); */

                /* if (start == NULL) */
                /*     utstring_printf(label, */
                /*                     "X@%s//%s", */
                /*                     _filedeps_path, *archive_name); */
                /* else { */
                /*     *start++ = '\0'; // split on '.' */
                /*     utstring_printf(label, */
                /*                     "Y@%s//%s:%s", */
                /*                     _filedeps_path, */
                /*                     start, */
                /*                     *archive_name); */
                /* } */
            /* } */

            /* utstring_printf(label, "@rules_ocaml//cfg/:%s/%s", */
            /*                 _filedeps_path, *archive_name); */
            /* LOG_DEBUG(0, "label 2: %s", utstring_body(label)); */

            // FIXME: verify existence using access()?
                if (strncmp(utstring_body(cmtag), "cmxa", 4) == 0) {
                    fprintf(ostream, " = \"%s\",\n", utstring_body(label));
                }
        }
        utstring_free(label);
    next:
        ;
    }
    utstring_free(cmtag);
}

/*
  FIXME: version, description really apply to whole pkg; put them in comments, not rule attributes
 */
void emit_bazel_metadatum(FILE* ostream, int level,
                          char *repo,
                          /* char *pkg, */
                          obzl_meta_entries *_entries,
                          char *_property, // META property
                          char *_attrib    // Bazel attr name
                          )
{
    (void)level;
    (void)repo;
    /* emit version, description properties */

    struct obzl_meta_property *the_prop = obzl_meta_entries_property(_entries, _property);
    if ( the_prop == NULL ) {
        /* char *pkg_name = obzl_meta_package_name(_pkg); */
        /* LOG_WARN(0, "Prop '%s' not found: %s.", _property); //, pkg_name); */
        return;
    }

    obzl_meta_settings *settings = obzl_meta_property_settings(the_prop);
    obzl_meta_setting *setting = NULL;

    if (obzl_meta_settings_count(settings) == 0) {
        /* LOG_INFO(0, "No settings for property '%s'", _property); */
        return;
    }
    int settings_ct = obzl_meta_settings_count(settings);
    if (settings_ct > 1) {
        LOG_WARN(0, "settings count > 1 for property '%s'; using the first.", settings_ct, _property);
    }

    obzl_meta_values *vals;
    obzl_meta_value *v = NULL;

    setting = obzl_meta_settings_nth(settings, 0);

    /* should not be any flags??? */

    vals = obzl_meta_setting_values(setting);
    /* dump_values(0, vals); */
    /* LOG_DEBUG(0, "metadatum %s setting vals ct: %d", _property, obzl_meta_values_count(vals)); */
    /* if vals ct > 1 warn, should be only 1 */
    v = obzl_meta_values_nth(vals, 0);
    /* LOG_DEBUG(0, "vals[0]: %s", *v); */
    if (v != NULL)
        /* fprintf(stderr, "    %s = \"%s\",\n", _attrib, *v); */
        fprintf(ostream, "    %s = \"\"\"%s\"\"\",\n",
                _attrib,
                /* _property, */
                *v);
}

void emit_bazel_archive_rule(FILE* ostream,
                             char *coswitch_lib,
                             int level,
                             /* char *ws_name, */
                             char *_filedeps_path, /* _lib/... */
                             char *_pkg_root,
                             /* char *_pkg_path, */
                             char *_pkg_prefix,
                             char *_pkg_name,
                             UT_string *pkg_parent,
                             obzl_meta_entries *_entries, //)
                             /* char *_subpkg_dir) */
                             obzl_meta_package *_pkg)
{
    (void)level;
#if defined(PROFILE_fastbuild)
    LOG_DEBUG(0, "EMIT_BAZEL_ARCHIVE_RULE: _filedeps_path: %s", _filedeps_path);
#endif
    /* http://projects.camlcity.org/projects/dl/findlib-1.9.1/doc/ref-html/r759.html

The variable "archive" specifies the list of archive files. These files should be given either as (1) plain names without any directory information; they are only searched in the package directory. (2) Or they have the form "+path" in which case the files are looked up relative to the standard library. (3) Or they have the form "@name/file" in which case the files are looked up in the package directory of another package. (4) Or they are given as absolute paths.

The names of the files must be separated by white space and/or commas. In the preprocessor stage, the archive files are passed as extensions to the preprocessor (camlp4) call. In the linker stage (-linkpkg), the archive files are linked. In the compiler stage, the archive files are ignored.

Note that "archive" should only be used for archive files that are intended to be included in executables or loaded into toploops. For modules loaded at runtime there is the separate variable "plugin".
     */

    /* archive flags: byte, native */
    /* pkg threads: mt, mt_vm, mt_posix */
    /* ppx: ppx_driver, -ppx_driver always go together. not a syntactic rule, but seems to be the convention */

    /* _filedeps_path: relative to opam_switch_lib
       we need to convert it to a bazel label
       e.g. ounit2/advanced => @ounit2//advanced
       we do this here because it applies to all props/subpkgs
       BUT: just use pkg_prefix and pkg name?
     */

    //FIXME: only if --enable-jsoo passed
    /* emit_bazel_jsoo(ostream, 1, */
    /*                 _pkg_prefix, */
    /*                 _pkg_name, */
    /*                 /\* for constructing import label: *\/ */
    /*                 _filedeps_path, */
    /*                 /\* _subpkg_dir, *\/ */
    /*                 _entries, */
    /*                 "archive", */
    /*                 _pkg); */

    //FIXME: only emit this if headers exist
    fprintf(ostream, "\ncc_library(\n");
    fprintf(ostream, "    name = \"hdrs\",\n");
    fprintf(ostream, "    hdrs = glob([\"*.h\"])\n");
    fprintf(ostream, ")\n\n");

    /* write scheme opam-resolver table */
    //FIXME: for here-switch only
    /* write_opam_resolver(_pkg_prefix, _pkg_name, _entries); */
    fprintf(ostream, "\nocaml_import(\n");
    fprintf(ostream, "    name = \"%s\",\n", _pkg_name); /* default target provides archive */

    emit_bazel_metadatum(ostream, 1,
                         ocaml_ws,              /* @ocaml */
                         /* _filedeps_path, */
                         _entries, "version", "version");
    emit_bazel_metadatum(ostream, 1,
                         ocaml_ws, // "@ocaml",
                         /* _filedeps_path, */
                         _entries, "description", "doc");

    fprintf(ostream, "    sigs     = glob([\"*.cmi\"]),\n");

    emit_bazel_archive_attr(ostream, 1,
                            /* ws_name, // ocaml_ws, */
                            /* _pkg_path, */
                            _pkg_prefix,
                            _pkg_name,
                            /* for constructing import label: */
                            _filedeps_path,
                            /* _subpkg_dir, */
                            _entries,
                            "archive",
                            _pkg);

    /* emit_bazel_codeps_attr(ostream, 1, */
    /*                         /\* ws_name, // ocaml_ws, *\/ */
    /*                         /\* _pkg_path, *\/ */
    /*                         _pkg_prefix, */
    /*                         _pkg_name, */
    /*                         /\* for constructing import label: *\/ */
    /*                         _filedeps_path, */
    /*                         /\* _subpkg_dir, *\/ */
    /*                         _entries, */
    /*                         "ppx_runtime_deps", */
    /*                         _pkg); */

    /* fprintf(ostream, "    astructs    = select({\n" */
    /*         "        \"@ocaml//platform/emitter:vm\": glob([\"*.cmo\"]),\n" */
    /*         "        \"//conditions:default\": glob([\"*.cmx\"])\n" */
    /*         "    }),\n"), */


    /* FIXME: only .a stem matching archive stem  */
    fprintf(ostream,
            "    afiles   = select({\n"
            "        \"@ocaml//platform/emitter:vm\" : [],\n"
            "        \"@ocaml//platform/emitter:sys\": "
            "glob([\"*.a\"],\n"
            "                                         exclude=[\"*_stubs.a\"])\n"
            "    }, no_match_error=\"Bad platform\"),\n");

    /* FIXME: 'structs', not 'astructs' -  bazel rule decides what to do */
    //fprintf(ostream, "    astructs = glob([\"*.cmx\"]),\n");
    fprintf(ostream,
            "    astructs = select({\n"
            "        \"@ocaml//platform/emitter:vm\" : [],\n"
            "        \"@ocaml//platform/emitter:sys\": "
            "glob([\"*.cmx\"])\n"
            "    }, no_match_error=\"Bad platform\"),\n");

    /* FIXME: ofiles are never present? */
    //fprintf(ostream, "    ofiles   = glob([\"*.o\"]),\n");
    fprintf(ostream,
            "    ofiles   = select({\n"
            "        \"@ocaml//platform/emitter:vm\" : [],\n"
            "        \"@ocaml//platform/emitter:sys\": "
            "glob([\"*.o\"])\n"
            "    }, no_match_error=\"Bad platform\"),\n");

    fprintf(ostream, "    cmts     = glob([\"*.cmt\"]),\n");
    fprintf(ostream, "    cmtis    = glob([\"*.cmti\"]),\n");
    fprintf(ostream, "    vmlibs   = glob([\"dll*.so\"]),\n");
    fprintf(ostream, "    srcs     = glob([\"*.ml\", \"*.mli\"]),\n");

    //FIXME: only if --enable-jsoo passed
    /* emit_bazel_jsoo_runtime_attr(ostream, 1, */
    /*                              _pkg_root, */
    /*                              _pkg_prefix, */
    /*                              _pkg_name, */
    /*                              pkg_parent, */
    /*                              _filedeps_path, */
    /*                              _entries, */
    /*                              _pkg); */

    /* emit cc_deps attr with lib*stubs.a */
    emit_bazel_stublibs_attr(ostream,
                             coswitch_lib,
                             1, /* indent level */
                             _pkg_root,
                             _pkg_prefix,
                             _pkg_name,
                             pkg_parent,
                             _filedeps_path,
                             _entries,
                             _pkg);

    /* fprintf(ostream, "    all      = glob([\"*.\"]),\n"); */
    emit_bazel_deps_attribute(ostream, 1,
                              false, /* not jsoo */
                              ocaml_ws,
                              "lib", _pkg_name, _entries);

    emit_bazel_ppx_codeps(ostream, 1, ocaml_ws, _pkg_name, "lib", _entries);
    /* fprintf(ostream, "    visibility = [\"//visibility:public\"]\n"); */
    fprintf(ostream, ")\n");
}

void emit_bazel_plugin_rule(FILE* ostream, int level,
                            char *_repo,
                            char *_filedeps_path,
                            /* char *_pkg_path, */
                            char *_pkg_prefix,
                            char *_pkg_name,
                            obzl_meta_entries *_entries, //)
                            obzl_meta_package *_pkg)
                            /* char *_subpkg_dir) */
{
    (void)level;
    (void)_repo;
    TRACE_ENTRY;
#if defined(PROFILE_fastbuild)
    LOG_DEBUG(0, BLU "EMIT_BAZEL_PLUGIN_RULE:" CRESET " _filedeps_path: %s", _filedeps_path);
    LOG_DEBUG(0, "pkg_pfx: %s", _pkg_prefix);
    LOG_DEBUG(0, "_pkg_name: %s", _pkg_name);
    LOG_DEBUG(0, "pkg_name: %s", _pkg->name);
#endif
    //FIXME: for here-switch only
    /* write_opam_resolver(_pkg_prefix, _pkg_name, _entries); */

    fprintf(ostream, "\nocaml_import(\n");
    fprintf(ostream, "    name = \"plugin\",\n");
    /* LOG_DEBUG(0, "PDUMPP %s", _pkg_name); */
    /* dump_entries(0, _entries); */

    /* emit_bazel_attribute(ostream, 1, */
    /*                      /\* _repo, *\/ */
    /*                      /\* _pkg_path, *\/ */
    /*                      _pkg_prefix, */
    /*                      _pkg_name, */
    /*                      _filedeps_path, */
    /*                      /\* _subpkg_dir, *\/ */
    /*                      _entries, */
    /*                      "plugin", //); */
    /*                      _pkg); */

    /* emit_bazel_archive_attr(ostream, 1, */
    /*                         /\* _repo, // ocaml_ws, *\/ */
    /*                         /\* _pkg_path, *\/ */
    /*                         _pkg_prefix, */
    /*                         _pkg_name, */
    /*                         /\* for constructing import label: *\/ */
    /*                         _filedeps_path, */
    /*                         /\* _subpkg_dir, *\/ */
    /*                         _entries, */
    /*                         "plugin", */
    /*                         _pkg); */
    /* fprintf(ostream, "    cmxa = glob([\"*.cmxa\", \"*.a\"]),\n"); */
    /* fprintf(ostream, "    cma  = glob([\"*.cma\"]),\n"); */

    emit_bazel_cmxs_attr(ostream, 1,
                            /* _repo, // ocaml_ws, */
                            /* _pkg_path, */
                            _pkg_prefix,
                            _pkg_name,
                            /* for constructing import label: */
                            _filedeps_path,
                            /* _subpkg_dir, */
                            _entries,
                            "plugin",
                            _pkg);

    /* fprintf(ostream, "    cmxs = glob([\"*.cmxs\"]),\n"); */

    /* fprintf(ostream, "    cmi    = glob([\"*.cmi\"]),\n"); */
    /* fprintf(ostream, "    cmo    = glob([\"*.cmo\"]),\n"); */
    /* fprintf(ostream, "    cmx    = glob([\"*.cmx\"]),\n"); */
    /* fprintf(ostream, "    ofiles = glob([\"*.o\"]),\n"); */
    /* fprintf(ostream, "    afiles = glob([\"*.a\"]),\n"); */
    /* fprintf(ostream, "    cmts   = glob([\"*.cmt\"]),\n"); */
    /* fprintf(ostream, "    cmtis  = glob([\"*.cmti\"]),\n"); */
    /* fprintf(ostream, "    srcs   = glob([\"*.ml\", \"*.mli\"]),\n"); */

    emit_bazel_deps_attribute(ostream, 1,
                              false, /* not jsoo */
                              ocaml_ws,
                              "lib", _pkg_name, _entries);
    /* fprintf(ostream, "    visibility = [\"//visibility:public\"]\n"); */
    fprintf(ostream, ")\n");
}

/*
  special casing: META with directory starting with '^'
  bigarray, dynlink, str, threads, unix
  also: num, raw-spacetime, stdlib, ocamldoc?
  num is a special case. was part of core distrib, now it's a pkg

  we emit an alias to redirect from standalone to @ocaml
  e.g. @bigarray//bigarray => @ocaml//lib/bigarray

  this is an OPAM legacy thing in case people depend on these pkgs. we
  don't really need to do this since legacy-to-obazl conversion should
  map legacy pkg names correctly. But we do it anyway just in case.
 */

/* FIXME: copy templates instead? */
bool emit_special_case_rule(FILE* ostream,
                            obzl_meta_package *_pkg)
{
#if defined(PROFILE_fastbuild)
    LOG_TRACE(0, "emit_special_case_rule pkg: %s", _pkg->name);
#endif

    if ((strncmp(_pkg->name, "bigarray", 8) == 0)
        && strlen(_pkg->name) == 8) {

#if defined(PROFILE_fastbuild)
        LOG_TRACE(0, "emit_special_case_rule: %s", "bigarrray");
#endif

        fprintf(ostream, "alias(\n"
                "    name = \"bigarray\",\n"
                "    actual = \"@ocaml//lib/bigarray\",\n"
                "    visibility = [\"//visibility:public\"]\n"
                ")\n");
        return true;
    }

    if ((strncmp(_pkg->name, "dynlink", 7) == 0)
        && strlen(_pkg->name) == 7) {
#if defined(PROFILE_fastbuild)
        LOG_TRACE(0, "emit_special_case_rule: %s",  "dynlink");
#endif

        fprintf(ostream, "## special case: dynlink");
        fprintf(ostream, "alias(\n"
                "    name = \"dynlink\",\n"
                "    actual = \"@ocaml//lib/dynlink\",\n"
                "    visibility = [\"//visibility:public\"]\n"
                ")\n");
        return true;
    }

    /* WARNING: some META files list 'compiler-libs' as a requirement,
       but the META file for compiler-libs itself lists no content for
       that package, only subpackages compiler-libs.common etc. are
       populated. The official docs indirectly suggest that the base
       pkg is compiler-libs.common, so that's what we do here.
     */

    if ((strncmp(_pkg->name, "compiler-libs", 13) == 0)
        && strlen(_pkg->name) == 13) {
#if defined(PROFILE_fastbuild)
        LOG_TRACE(0, "emit_special_case_rule: %s", "compiler-libs");
#endif

        fprintf(ostream, "alias(\n"
                "    name = \"compiler-libs\",\n"
                "    actual = \"@ocaml//lib/compiler-libs/common\",\n"
                "    visibility = [\"//visibility:public\"]\n"
                ")\n");
        return true;
    }

    //FIXME: lib/ctypes/foreign/BUILD.bazel emits only
    // if lib/ctypes-foreign exists
    if ((strncmp(_pkg->name, "foreign", 7) == 0)
        && strlen(_pkg->name) == 7) {
#if defined(PROFILE_fastbuild)
        LOG_TRACE(0, "emit_special_case_rule: %s", "ctypes.foreign");
#endif

        fprintf(ostream, "alias(\n"
                "    name = \"foreign\",\n"
                "    actual = \"@ctypes-foreign//lib/ctypes-foreign\",\n"
                "    visibility = [\"//visibility:public\"]\n"
                ")\n");
        return true;
    }

    /* if ((strncmp(_pkg->name, "bytecomp", 8) == 0) */
    /*     && strlen(_pkg->name) == 8) { */
    /*     LOG_TRACE(0, "emit_special_case_rule: bytecomp"); */
    /*     LOG_TRACE(0, "emit_special_case_rule: pkg->dir: %s", _pkg->directory); */
    /*     LOG_TRACE(0, "emit_special_case_rule: pkg->path: %s", _pkg->path); */
    /* /\* char *name; *\/ */
    /* /\* char *path; *\/ */
    /* /\* char *directory;            /\\* subdir *\\/ *\/ */
    /* /\* char *metafile; *\/ */
    /* /\* obzl_meta_entries *entries;          /\\* list of struct obzl_meta_entry *\\/ *\/ */


    /*     fprintf(ostream, "alias(\n" */
    /*             "    name = \"bytecomp\",\n" */
    /*             "    actual = \"@ocaml//lib/compiler-libs/bytecomp\",\n" */
    /*             "    visibility = [\"//visibility:public\"]\n" */
    /*             ")\n"); */
    /*     return true; */
    /* } */

    //TODO: compiler-libs/common, bytecomp, optcomp,
    // toplevel, native-toplevel


    if ((strncmp(_pkg->name, "num", 3) == 0)
        && strlen(_pkg->name) == 3) {
#if defined(PROFILE_fastbuild)
        LOG_TRACE(0, "emit_special_case_rule: %s", "num");
#endif

        fprintf(ostream, "alias(\n"
                "    name = \"num\",\n"
                "    actual = \"@ocaml//lib/num/core\",\n"
                "    visibility = [\"//visibility:public\"]\n"
                ")\n");
        return true;
    }

    if ((strncmp(_pkg->name, "ocamldoc", 8) == 0)
        && strlen(_pkg->name) == 8) {
#if defined(PROFILE_fastbuild)
        LOG_TRACE(0, "emit_special_case_rule: %s", "ocamldoc");
#endif

        fprintf(ostream, "alias(\n"
                "    name = \"ocamldoc\",\n"
                "    actual = \"@ocaml//lib/ocamldoc\",\n"
                "    visibility = [\"//visibility:public\"]\n"
                ")\n");
        return true;
    }

    if ((strncmp(_pkg->name, "str", 3) == 0)
        && strlen(_pkg->name) == 3) {
#if defined(PROFILE_fastbuild)
        LOG_TRACE(0, "emit_special_case_rule: %s", "str");
#endif
        /* fprintf(ostream, "xxxxxxxxxxxxxxxx"); */
        fprintf(ostream, "alias(\n"
                "    name = \"str\",\n"
                "    actual = \"@ocaml//lib/str\",\n"
                "    visibility = [\"//visibility:public\"]\n"
                ")\n");
        return true;
    }

    if ((strncmp(_pkg->name, "threads", 7) == 0)
        && strlen(_pkg->name) == 7) {
#if defined(PROFILE_fastbuild)
        LOG_TRACE(0, "emit_special_case_rule: %s", "threads");
#endif

        fprintf(ostream, "alias(\n"
                "    name = \"threads\",\n"
                "    actual = \"@ocaml//lib/threads\",\n"
                "    visibility = [\"//visibility:public\"]\n"
                ")\n");
        return true;
    }

    if ((strncmp(_pkg->name, "unix", 4) == 0)
        && strlen(_pkg->name) == 4) {
#if defined(PROFILE_fastbuild)
        LOG_TRACE(0, "emit_special_case_rule: %s", "unix");
#endif

        fprintf(ostream, "alias(\n"
                "    name = \"unix\",\n"
                "    actual = \"@ocaml//lib/unix\",\n"
                "    visibility = [\"//visibility:public\"]\n"
                ")\n");
        return true;
    }

    return false;
}

/* **************************************************************** */
bool special_case_multiseg_dep(FILE* ostream,
                               obzl_meta_value *dep_name,
                               char *delim1)
{
    if (delim1 == NULL) {
        if (strncmp(*dep_name, "compiler-libs", 13) == 0) {
            fprintf(ostream, "%*s    \"@ocaml//lib/compiler-libs/common\",\n",
                    (1+level)*spfactor, sp);
            return true;
        } else {
            if (strncmp(*dep_name, "threads", 13) == 0) {
                fprintf(ostream, "%*s    \"@ocaml//lib/threads\",\n",
                        (1+level)*spfactor, sp);
                return true;
            }
        }
    } else {

        if (strncmp(*dep_name, "compiler-libs/", 14) == 0) {
            fprintf(ostream,
                    "%*s    \"@ocaml//lib/compiler-libs/%s\",\n",
                    (1+level)*spfactor, sp, delim1+1);
            return true;
        }

        if (strncmp(*dep_name, "threads/", 8) == 0) {
            /* threads.posix, threads.vm => threads */
            fprintf(ostream, "        \"@ocaml//lib/threads\",\n");
            /* (1+level)*spfactor, sp, delim1+1); */
            return true;
        }

        /* if (strncmp(*dep_name, "posix/", 8) == 0) { */
        /*     fprintf(ostream, "%*s\"@rules_ocaml//cfg/threads/%s\",\n", */
        /*             (1+level)*spfactor, sp, delim1+1); */
        /*     return true; */
        /* } */

    }
    return false;
}

void emit_bazel_deps_attribute(FILE* ostream, int level,
                               bool jsoo,
                               char *repo, char *pkg, char *pkg_name,
                               obzl_meta_entries *_entries)
/* obzl_meta_package *_pkg) */
{
    (void)repo;
    (void)pkg_name;
    TRACE_ENTRY;
    //FIXME: skip if no 'requires' or requires == ''
    /* obzl_meta_entries *entries = obzl_meta_package_entries(_pkg); */

    /* char *kind = "library_kind"; */
    /* deps_prop = obzl_meta_entries_property(_entries, kind); */
    /* if ( deps_prop == NULL ) { */
    /*     printf("NO LIBRARY_KIND! %s\n", pkg_name); */
    /* } else { */
    /*     obzl_meta_value k = obzl_meta_property_value(deps_prop); */
    /*     printf("LIBRARY_KIND! %s: %s\n", pkg_name, (char*)k); */
    /* } */

    char *pname = "requires";
    struct obzl_meta_property *deps_prop = NULL;
    deps_prop = obzl_meta_entries_property(_entries, pname);
    if ( deps_prop == NULL ) return;
    obzl_meta_value ds = obzl_meta_property_value(deps_prop);
    if (ds == NULL) return;

    obzl_meta_settings *settings = obzl_meta_property_settings(deps_prop);
    obzl_meta_setting *setting = NULL;

    int settings_ct = obzl_meta_settings_count(settings);
    int settings_no_ppx_driver_ct = obzl_meta_settings_flag_count(settings, "ppx_driver", false);

    settings_ct -= settings_no_ppx_driver_ct;

    /* LOG_INFO(0, "settings count: %d", settings_ct); */

    if (settings_ct == 0) {
        /* LOG_INFO(0, "No deps for %s", obzl_meta_property_name(deps_prop)); */
        return;
    }

    obzl_meta_values *vals;
    obzl_meta_value *dep_name_val = NULL;
    char *dep_name;

    if (settings_ct > 1) {
        fprintf(ostream, "%*sdeps = select({\n", level*spfactor, sp);
    } else {
        fprintf(ostream, "%*sdeps = [\n", level*spfactor, sp);
    }

    UT_string *condition_name;
    utstring_new(condition_name);

    for (int i = 0; i < settings_ct; i++) {
        setting = obzl_meta_settings_nth(settings, i);
        /* LOG_DEBUG(0, "setting %d", i+1); */

        obzl_meta_flags *flags = obzl_meta_setting_flags(setting);
        /* int flags_ct; // = 0; */
        if (flags != NULL) {
            /* register_flags(flags); */
            /* flags_ct = obzl_meta_flags_count(flags); */
        }

        if (obzl_meta_flags_has_flag(flags, "ppx_driver", false)) {
            continue;
        }

        bool has_conditions;
        if (flags == NULL)
            utstring_printf(condition_name, "//conditions:default");
        else
            has_conditions = obzl_meta_flags_to_selection_label(flags, condition_name);

        char *condition_comment = obzl_meta_flags_to_comment(flags);
        /* LOG_DEBUG(0, "condition_comment: %s", condition_comment); */

        /* 'requires' usually has no flags; when it does, empirically we find only */
        /*   ppx pkgs: ppx_driver, -ppx_driver */
        /*   pkg 'batteries': requires(mt) */
        /*   pkg 'num': requires(toploop) */
        /*   pkg 'findlib': requires(toploop), requires(create_toploop) */

        /* Multiple settings on 'requires' means multiple flags; */
        /* empirically, this only happens for ppx packages, typically as */
        /* requires(-ppx_driver,-custom_ppx) */
        /* the (sole?) exception is */
        /*   pkg 'threads': requires(mt,mt_vm), requires(mt,mt_posix) */

        if (settings_ct > 1) {
            fprintf(ostream, "%*s\"X%s%s\": [ ## predicates: %s\n",
                    (1+level)*spfactor, sp,
                    utstring_body(condition_name),
                    (has_conditions)? "" : "",
                    condition_comment);
        }

        vals = resolve_setting_values(setting, flags, settings);
        /* vals = obzl_meta_setting_values(setting); */
        /* LOG_DEBUG(0, "vals ct: %d", obzl_meta_values_count(vals)); */
        /* dump_values(0, vals); */
        /* now we handle UPDATE settings */

        for (int j = 0; j < obzl_meta_values_count(vals); j++) {
            dep_name_val = obzl_meta_values_nth(vals, j);
            /* LOG_INFO(0, "property val[%d]: '%s'", j, *dep_name_val); */
            dep_name = strdup((char*)*dep_name_val);

            char *s = (char*)dep_name;

            /* printf("DEP: %s\n", s); */

            /* special case: uchar */
            if ((strncmp(s, "uchar", 5) == 0)
                && (strlen(s) == 5)){
                /* LOG_DEBUG(0, "OMITTING UCHAR dep"); */
                continue;
            }
            /* special case: threads */
            /* if ((strncmp(s, "threads", 7) == 0) */
            /*     && (strlen(s) == 7)){ */
            /*     /\* LOG_DEBUG(0, "OMITTING THREADS dep"); *\/ */
            /*     continue; */
            /* } */
            while (*s) {
                /* printf("s: %s\n", s); */
                if(s[0] == '.') {
                    /* LOG_INFO(0, "Hit"); */
                    s[0] = '/';
                }
                s++;
            }

            /* emit 'deps' attr labels */

            if (settings_ct > 1) {
                fprintf(ostream, "%*s\"@FIXME: %s//%s\",\n",
                        (2+level)*spfactor, sp, dep_name, pkg);
            } else {
                /* first convert pkg string */
                /* char *s = (char*)*dep_name; */
                /* char *tmp; */
                /* while(s) { */
                /*     tmp = strchr(s, '/'); */
                /*     if (tmp == NULL) break; */
                /*     *tmp = '.'; */
                /*     s = tmp; */
                /* } */
                /* then extract target segment */
                char *delim1 = strchr(dep_name, '/');
                /* LOG_DEBUG(0, "WW *dep_name: %s; delim1: %s, null? %d\n", */
                /*         *dep_name, delim1, (delim1 == NULL)); */

                if (delim1 == NULL) {
                    if (special_case_multiseg_dep(ostream, &dep_name, delim1))
                        continue;
                    else {
                        /* single-seg pkg, e.g. ptime  */

                        // handle distrib-pkgs: dynlink, str, unix, etc.
                        /* FIXME: obsolete? */
                        if ((strncmp(dep_name, "bigarray", 8) == 0)
                            && strlen(dep_name) == 8) {
                            fprintf(ostream, "%*s\"@ocaml//lib/bigarray\",\n",
                                    (1+level)*spfactor, sp);
                        } else {
                            if ((strncmp(dep_name, "unix", 4) == 0)
                                && strlen(dep_name) == 4) {
                                fprintf(ostream, "%*s\"@ocaml//lib/unix\",\n",
                                        (1+level)*spfactor, sp);
                            } else {
                                /* WARNING: for distrib-pkgs, form @ocaml//lib/<pkg>,
                                   not @<pkg>//lib/<pkg>
                                 */
                                if (strncmp("dynlink", dep_name, 7) == 0)
                                    distrib_pkg = true;
                                else
                                    distrib_pkg = false;
                                fprintf(ostream,
                                        "%*s\"@%s//lib/%s%s\",\n",
                                        /* "%*s\"@%s//:%s\",\n", */
                                        (1+level)*spfactor, sp,
                                        distrib_pkg? "ocaml" : dep_name,
                                        dep_name,
                                        jsoo? ":js" : "");
                            }
                        }
                    }
                } else {
                    /* multi-seg pkg, e.g. lwt.unix, ptime.clock.os */
                    if (special_case_multiseg_dep(ostream, &dep_name, delim1))
                        continue;
                    else {
                        int repo_len = delim1 - (char*)dep_name;
                        fprintf(ostream,
                                "%*s\"@%.*s//lib/%s%s\",\n",
                                /* "%*s\"@%.*s//%s\",\n", */
                                (1+level)*spfactor, sp,
                                repo_len,
                                dep_name,
                                delim1+1,
                                jsoo? ":js" : "");
                        /* (1+level)*spfactor, sp, repo, pkg, *dep_name); */
                    }
                }
            }
            free(dep_name);
        }
        if (settings_ct > 1) {
            fprintf(ostream, "%*s],\n", (1+level)*spfactor, sp);
        }
        free(condition_comment);
    }
    utstring_free(condition_name);
    if (settings_ct > 1)
        fprintf(ostream, "%*s}),\n", level*spfactor, sp);
    else
        fprintf(ostream, "%*s],\n", level*spfactor, sp);
}

void emit_bazel_ppx_codeps(FILE* ostream, int level,
                           char *repo,
                           char *_pkg_name,
                           char *_pkg_prefix,
                           obzl_meta_entries *_entries)
{
    (void)_pkg_name;
    /* handle both 'ppx_runtime_deps' and 'requires(-ppx_driver)'

       e.g. ppx_sexp_conv has no ppx_runtime_deps property but:
       requires(-ppx_driver) = "ppx_sexp_conv.runtime-lib"

       ppx_bin_prot has both:
       requires(ppx_driver) = "base
                        compiler-libs.common
                        ppx_bin_prot.shape-expander
                        ppxlib
                        ppxlib.ast"
       ppx_runtime_deps = "bin_prot"
       but:
       requires(-ppx_driver) = "bin_prot ppx_here.runtime-lib"
       requires(-ppx_driver,-custom_ppx) += "ppx_deriving"

       and ppx_expect is even worse; in addition to
       requires(ppx_driver) and requires(-ppx_driver), it has
       ppx(-ppx_driver,-custom_ppx) = "./ppx.exe --as-ppx"

       what does it all mean?  who knows.

       we interpret this to mean: if you build (with ocamlfind) with
       predicate -ppx_driver (= NOT ppx_driver), it means you want to
       use the pkg as a regular dependency, not as part of a ppx
       executable. so in that case, you have to add the ppx_codep as a
       normal dep of the file you're compiling. If you were to use
       predicate ppx_driver, it would mean you are using ppx_sexp_conv
       as part of a ppx executable. The assumption then is that the
       build tool (dune) will compile the ppx executable and use it to
       compile the source file, as a kind of atomic operation, so to
       speak, so it must be smart enough to know that the ppx_codep
       (runtime dep) will be needed to compile the source file after
       ppx processing. which implies that the build tool must treat
       the 'requires(-ppx_driver)' value as a (co)dependency even when
       the predicate is ppx_driver. IOW, '-ppx_driver' does not mean
       "only use this if -ppx_driver' is passed"

       fuck. that means we really need two deps, one for ppx and one
       for non-ppx dep. Example: ppx_bin_prot. the META files have
       this comment:
       # This line makes things transparent for people mixing preprocessors
       # and normal dependencies
       requires(-ppx_driver) = "bin_prot ppx_here.runtime-lib"

       meaning we need a target @ppx_bin_prot//foo for using it as a
       "normal" dep, not a ppx dep.

     */

    //FIXME: skip if no 'requires'
    /* obzl_meta_entries *entries = obzl_meta_package_entries(_pkg); */

#if defined(PROFILE_fastbuild)
    LOG_DEBUG(0, "emit_bazel_ppx_codeps, repo: %s, pfx: %s, pkg: %s",
              repo, _pkg_prefix, _pkg_name);
#endif
/* (FILE* ostream, int level, */
/*                            char *repo, */
/*                            char *_pkg_prefix, */
/*                            obzl_meta_entries *_entries) */

    struct obzl_meta_property *deps_prop = obzl_meta_entries_property(_entries, "ppx_runtime_deps");
    if ( deps_prop == NULL ) {
        /* char *pkg_name = obzl_meta_package_name(_pkg); */
#if defined(PROFILE_fastbuild)
        LOG_WARN(0, "Prop 'ppx_runtime_deps' not found for pkg: %s.",
                 _pkg_name);
#endif
        return;
    }

    obzl_meta_settings *settings = obzl_meta_property_settings(deps_prop);
    obzl_meta_setting *setting = NULL;

    int settings_ct = obzl_meta_settings_count(settings);
    int settings_no_ppx_driver_ct = obzl_meta_settings_flag_count(settings, "ppx_driver", false);

    settings_ct -= settings_no_ppx_driver_ct;

    /* LOG_INFO(0, "settings count: %d", settings_ct); */

    if (settings_ct == 0) {
        /* LOG_INFO(0, "No deps for %s", obzl_meta_property_name(deps_prop)); */
        return;
    }

    obzl_meta_values *vals;
    obzl_meta_value *v = NULL;

    if (settings_ct > 1) {
        fprintf(ostream, "%*sppx_codeps = select({\n", level*spfactor, sp);
    } else {
        fprintf(ostream, "%*sppx_codeps = [\n", level*spfactor, sp);
    }

    UT_string *condition_name;
    utstring_new(condition_name);

    for (int i = 0; i < settings_ct; i++) {
        setting = obzl_meta_settings_nth(settings, i);
        /* LOG_DEBUG(0, "setting %d", i+1); */

        obzl_meta_flags *flags = obzl_meta_setting_flags(setting);
        /* int flags_ct; // = 0; */
        if (flags != NULL) {
            /* register_flags(flags); */
            /* flags_ct = obzl_meta_flags_count(flags); */
        }

        if (obzl_meta_flags_has_flag(flags, "ppx_driver", false)) {
            continue;
        }

        bool has_conditions = false;
        if (flags == NULL)
            utstring_printf(condition_name, "//conditions:default");
        else
            has_conditions = obzl_meta_flags_to_selection_label(flags, condition_name);

        char *condition_comment = obzl_meta_flags_to_comment(flags);
        /* LOG_DEBUG(0, "condition_comment: %s", condition_comment); */

        /* 'requires' usually has no flags; when it does, empirically we find only */
        /*   ppx pkgs: ppx_driver, -ppx_driver */
        /*   pkg 'batteries': requires(mt) */
        /*   pkg 'num': requires(toploop) */
        /*   pkg 'findlib': requires(toploop), requires(create_toploop) */

        /* Multiple settings on 'requires' means multiple flags; */
        /* empirically, this only happens for ppx packages, typically as */
        /* requires(-ppx_driver,-custom_ppx) */
        /* the (sole?) exception is */
        /*   pkg 'threads': requires(mt,mt_vm), requires(mt,mt_posix) */

        if (settings_ct > 1) {
            fprintf(ostream, "%*s\"%s%s\": [ ## predicates: %s\n",
                    (1+level)*spfactor, sp,
                    utstring_body(condition_name),
                    (has_conditions)? "_enabled" : "",
                    condition_comment);
        }

        vals = resolve_setting_values(setting, flags, settings);
        /* vals = obzl_meta_setting_values(setting); */
        /* LOG_DEBUG(0, "vals ct: %d", obzl_meta_values_count(vals)); */
        /* dump_values(0, vals); */
        /* now we handle UPDATE settings */

        for (int j = 0; j < obzl_meta_values_count(vals); j++) {
            v = obzl_meta_values_nth(vals, j);
            /* LOG_INFO(0, "XXXX property val: '%s'", *v); */
            /* fprintf(ostream, "property val: '%s'\n", *v); */

            char *s = (char*)*v;
            while (*s) {
                /* printf("s: %s\n", s); */
                if(s[0] == '.') {
                    /* LOG_INFO(0, "Hit"); */
                    s[0] = '/';
                }
                s++;
            }
            if (settings_ct > 1) {
                fprintf(ostream, "%*s\"FIXME%s//%s/%s\",\n",
                        (2+level)*spfactor, sp,
                        repo, _pkg_prefix, *v);
            } else {
                char *s = (char*)*v;
                char *tmp;
                while(s) {
                    tmp = strchr(s, '/');
                    if (tmp == NULL) break;
                    *tmp = '.';
                    s = tmp;
                }

                /* LOG_DEBUG(0, "%TESTING %s\n", *v); */

                /* then extract target segment */
                char * delim1 = strchr(*v, '.');

                if (delim1 == NULL) {
                    if (special_case_multiseg_dep(ostream, v, delim1))
                        continue;
                    else {
                /* if (delim1 == NULL) { */
                    int repo_len = strlen((char*)*v);
                    fprintf(ostream,
                            "%*s\"@%.*s//lib/%s\",\n",
                            (1+level)*spfactor, sp, repo_len,
                            *v, *v);
                    }
                } else {
                    if (special_case_multiseg_dep(ostream, v, delim1))
                        continue;
                    else {
                        //first the repo string
                        /* *delim1 = '\0'; // split string on '.' */
                        int repo_len = delim1 - (char*)*v;
                        /* fprintf(ostream, "%*s\"@%.*s//%s\",\n", */
                        fprintf(ostream,
                                "%*s\"@%.*s//lib",
                                (1+level)*spfactor, sp,
                                repo_len, *v);
                        /* delim1+1); */
                        // then the pkg:target
                        char *s = (char*)delim1;
                        char *tmp;
                        while(s) {
                            tmp = strchr(s, '.');
                            if (tmp == NULL) break;
                            *tmp = '/';
                            s = tmp;
                        }
                        fprintf(ostream, "%s\",\n", delim1);

                        /* fprintf(ostream, "%*s\"G@%s//%s\",\n", */
                        /*         (1+level)*spfactor, sp, *v, _pkg_prefix); */
                        /* (1+level)*spfactor, sp, repo, _pkg_prefix, *v); */
                    }
                }
            }
        }
        if (settings_ct > 1) {
            fprintf(ostream, "%*s],\n", (1+level)*spfactor, sp);
        }
        free(condition_comment);
    }
    utstring_free(condition_name);
    if (settings_ct > 1)
        fprintf(ostream, "%*s}),\n", level*spfactor, sp);
    else
        fprintf(ostream, "%*s],\n", level*spfactor, sp);
}

/* void emit_bazel_path_attrib(FILE* ostream, int level, */
/*                             char *repo, */
/*                             char *_pkg_src, */
/*                             char *pkg, */
/*                             obzl_meta_entries *_entries) */
/* { */
/*     LOG_DEBUG(0, "emit_bazel_path_attrib"); */
/*     /\* */
/* The variable "directory" redefines the location of the package directory. Normally, the META file is the first file read in the package directory, and before any other file is read, the "directory" variable is evaluated in order to see if the package directory must be changed. The value of the "directory" variable is determined with an empty set of actual predicates. The value must be either: an absolute path name of the alternate directory, or a path name relative to the stdlib directory of OCaml (written "+path"), or a normal relative path name (without special syntax). In the latter case, the interpretation depends on whether it is contained in a main or sub package, and whether the standard repository layout or the alternate layout is in effect (see site-lib for these terms). For a main package in standard layout the base directory is the directory physically containing the META file, and the relative path is interpreted for this base directory. For a main package in alternate layout the base directory is the directory physically containing the META.pkg files. The base directory for subpackages is the package directory of the containing package. (In the case that a subpackage definition does not have a "directory" setting, the subpackage simply inherits the package directory of the containing package. By writing a "directory" directive one can change this location again.) */
/*     *\/ */

/*     struct obzl_meta_property *deps_prop = obzl_meta_entries_property(_entries, "directory"); */
/*     if ( deps_prop == NULL ) { */
/*         /\* char *pkg_name = obzl_meta_package_name(_pkg); *\/ */
/*         /\* LOG_WARN(0, "Prop 'directory' not found: %s.", pkg_name); *\/ */
/*         return; */
/*     } */

/*     obzl_meta_settings *settings = obzl_meta_property_settings(deps_prop); */
/*     obzl_meta_setting *setting = NULL; */

/*     int settings_ct = obzl_meta_settings_count(settings); */
/*     /\* LOG_INFO(0, "settings count: %d", settings_ct); *\/ */

/*     if (settings_ct == 0) { */
/*         LOG_INFO(0, "No deps for %s", obzl_meta_property_name(deps_prop)); */
/*         return; */
/*     } */
/*     if (settings_ct > 1) { */
/*         LOG_WARN(0, "More than one setting for property 'description'; using the first"); */
/*         return; */
/*     } */

/*     UT_string *condition_name; */
/*     setting = obzl_meta_settings_nth(settings, 0); */

/*     obzl_meta_flags *flags = obzl_meta_setting_flags(setting); */
/*     int flags_ct = obzl_meta_flags_count(flags); */
/*     if (flags_ct > 0) { */
/*         LOG_ERROR(0, "Property 'directory' has unexpected flags."); */
/*         return; */
/*     } */

/*     obzl_meta_values *vals = resolve_setting_values(setting, flags, settings); */
/*     LOG_DEBUG(0, "vals ct: %d", obzl_meta_values_count(vals)); */
/*     /\* dump_values(0, vals); *\/ */

/*     if (obzl_meta_values_count(vals) == 0) { */
/*         return; */
/*     } */

/*     if (obzl_meta_values_count(vals) > 1) { */
/*         LOG_ERROR(0, "Property 'directory' has more than one value."); */
/*         return; */
/*     } */

/*     obzl_meta_value *dir = NULL; */
/*     dir = obzl_meta_values_nth(vals, 0); */
/*     LOG_INFO(0, "property val: '%s'", *dir); */

/*     bool stdlib = false; */
/*     if ( strncmp(*dir, "+", 1) == 0 ) { */
/*         (*dir)++; */
/*         stdlib = true; */
/*     } */
/*     LOG_INFO(0, "property val 2: '%s'", *dir); */
/*     /\* fprintf(ostream, "%s", *\/ */
/*     /\*         "    modules = [\"//:compiler-libs.common\"],\n"); *\/ */
/* } */

/*
  A "deps target" is one that has a 'deps' attribute but no other that
  provides output (i.e. no 'archive' or 'plugin'). Some packages are
  there for compatibility; their resources have been migrated to the
  OCaml distrib. They contain something like 'version = "[distributed
  with Ocaml]"'. Some of these can be treated like normal packages:
  they have 'requires' and/or 'plugin', and a 'directory' property
  redirecting to stdlib, i.e. '^' or leading '+'. So they require no
  special handling (e.g. bigarray). Others may omit 'directory' and
  have no 'archive' or 'plugin' properties; but they do have
  'requires', so we need to emit a target for pkgs depending on them.

 */
void emit_bazel_deps_target(FILE* ostream, int level,
                               char *_repo,
                               /* char *_pkg_prefix, */
                               /* char *_pkg_path, */
                               char *_pkg_name,
                               obzl_meta_entries *_entries)
{
    (void)level;
#if defined(PROFILE_fastbuild)
    LOG_DEBUG(0, "DUMMY %s", _pkg_name);
#endif

    /* FIXME: only for here-switch
    write_opam_resolver(_pkg_prefix, _pkg_name, _entries);
    */

    fprintf(ostream, "## empty targets, in case they are deps of some other target\n");

    /* sorry, special findlib hack: */
    if (strncmp(_pkg_name, "findlib", 7) == 0) {
        fprintf(ostream, "\nalias(\n");
        fprintf(ostream, "    name   = \"findlib\",\n");
        fprintf(ostream, "    actual = \"@findlib//lib/internal\",\n");
        /* fprintf(ostream, "    visibility = [\"//visibility:public\"]\n"); */
        fprintf(ostream, ")\n");
    } else {
        fprintf(ostream, "\nocaml_import(\n");
        fprintf(ostream, "    name = \"%s\",\n", _pkg_name);
        emit_bazel_metadatum(ostream, 1,
                             _repo,
                             /* _pkg_path, */
                             _entries, "version", "version");
        emit_bazel_metadatum(ostream, 1,
                             _repo, // "@ocaml",
                             /* _pkg_path, */
                             _entries, "description", "doc");
        emit_bazel_deps_attribute(ostream, 1,
                                  false, /* not jsoo */
                                  ocaml_ws, "lib", _pkg_name, _entries);
        /* emit_bazel_path_attrib(ostream, 1, ocaml_ws, _pkg_prefix, "lib", _entries); */
        /* fprintf(ostream, "    visibility = [\"//visibility:public\"]\n"); */
        fprintf(ostream, ")\n");
    }

    //FIXME: only if --enable-jsoo passed
    /* fprintf(ostream, "\njsoo_library(name = \"js\")\n"); */
}

/*
  A small number of META packages are there just to throw an error;
  for example, core.syntax.
 */
void emit_bazel_error_target(FILE* ostream, int level,
                               char *_repo,
                               char *_pkg_src,
                               /* char *_pkg_path, */
                               char *_pkg_name,
                               obzl_meta_entries *_entries)
{
    (void)level;
    (void)_pkg_src;
#if defined(PROFILE_fastbuild)
    LOG_DEBUG(0, "ERROR TARGET: %s", _pkg_name);
#endif
    fprintf(ostream, "\nocaml_import( # error\n");
    fprintf(ostream, "    name = \"%s\",\n", _pkg_name);
    emit_bazel_metadatum(ostream, 1,
                         _repo,
                         /* _pkg_path, */
                         _entries, "version", "version");
    emit_bazel_metadatum(ostream, 1,
                         _repo, // "@ocaml",
                         /* _pkg_path, */
                         _entries, "description", "doc");
    emit_bazel_metadatum(ostream, 1,
                         _repo,
                         /* _pkg_path, */
                         _entries, "error", "error");

    /* fprintf(ostream, "    visibility = [\"//visibility:public\"]\n"); */
    fprintf(ostream, ")\n");
}

// FIXME: most of this duplicates emit_bazel_deps_attribute
/* void emit_bazel_ppx_dummy_deps(FILE* ostream, int level, char *repo, char *pkg, */
/*                                obzl_meta_entries *_entries) */
/*                      /\* obzl_meta_package *_pkg) *\/ */
/* { */
/*     LOG_DEBUG(0, "emit_bazel_ppx_dummy_deps"); */
/*     //FIXME: skip if no 'requires' */
/*     /\* obzl_meta_entries *entries = obzl_meta_package_entries(_pkg); *\/ */

/*     char *pname = "requires"; */
/*     /\* struct obzl_meta_property *deps_prop = obzl_meta_package_property(_pkg, pname); *\/ */
/*     struct obzl_meta_property *deps_prop = obzl_meta_entries_property(_entries, pname); */
/*     if ( deps_prop == NULL ) { */
/*         /\* char *pkg_name = obzl_meta_package_name(_pkg); *\/ */
/*         /\* LOG_WARN(0, "Prop '%s' not found: %s.", pname, pkg_name); *\/ */
/*         return; */
/*     } */

/*     obzl_meta_settings *settings = obzl_meta_property_settings(deps_prop); */
/*     obzl_meta_setting *setting = NULL; */

/*     int settings_ct = obzl_meta_settings_count(settings); */
/*     int settings_no_ppx_driver_ct = obzl_meta_settings_flag_count(settings, "ppx_driver", false); */

/*     /\* settings_ct -= settings_no_ppx_driver_ct; *\/ */

/*     /\* LOG_INFO(0, "settings count: %d", settings_ct); *\/ */

/*     if (settings_no_ppx_driver_ct == 0) { */
/*         LOG_INFO(0, "No 'requires(-ppx_driver)' for %s", obzl_meta_property_name(deps_prop)); */
/*         return; */
/*     } */

/*     obzl_meta_values *vals; */
/*     obzl_meta_value *v = NULL; */

/*     if (settings_no_ppx_driver_ct > 1) { */
/*         fprintf(ostream, "%*sDEPS = select({\n", level*spfactor, sp); */
/*     } else { */
/*         fprintf(ostream, "%*sDEPS = [\n", level*spfactor, sp); */
/*     } */

/*     UT_string *condition_name; */
/*     utstring_new(condition_name); */

/*     for (int i = 0; i < settings_ct; i++) { */
/*         setting = obzl_meta_settings_nth(settings, i); */
/*         LOG_DEBUG(0, "setting %d", i+1); */
/*         /\* dump_setting(8, setting); *\/ */

/*         if (obzl_meta_setting_has_flag(setting, "ppx_driver", true)) { */
/*             LOG_DEBUG(0, "skipping setting with flag +ppx_driver"); */
/*             continue; */
/*         } */

/*         obzl_meta_flags *flags = obzl_meta_setting_flags(setting); */
/*         /\* int flags_ct = 0; *\/ */
/*         /\* if (flags != NULL) { *\/ */
/*         /\*     register_flags(flags); *\/ */
/*         /\*     flags_ct = obzl_meta_flags_count(flags); *\/ */
/*         /\* } *\/ */

/*         /\* select only settings with flag -ppx_driver *\/ */
/*         if ( !obzl_meta_flags_has_flag(flags, "ppx_driver", false) ) { */
/*             continue; */
/*         } */

/*         bool has_conditions; */
/*         if (flags == NULL) */
/*             utstring_printf(condition_name, "//conditions:default"); */
/*         else */
/*             has_conditions = obzl_meta_flags_to_selection_label(flags, condition_name); */

/*         char *condition_comment = obzl_meta_flags_to_comment(flags); */
/*         /\* LOG_DEBUG(0, "condition_comment: %s", condition_comment); *\/ */

/*         /\* 'requires' usually has no flags; when it does, empirically we find only *\/ */
/*         /\*   ppx pkgs: ppx_driver, -ppx_driver *\/ */
/*         /\*   pkg 'batteries': requires(mt) *\/ */
/*         /\*   pkg 'num': requires(toploop) *\/ */
/*         /\*   pkg 'findlib': requires(toploop), requires(create_toploop) *\/ */

/*         /\* Multiple settings on 'requires' means multiple flags; *\/ */
/*         /\* empirically, this only happens for ppx packages, typically as *\/ */
/*         /\* requires(-ppx_driver,-custom_ppx) *\/ */
/*         /\* the (sole?) exception is *\/ */
/*         /\*   pkg 'threads': requires(mt,mt_vm), requires(mt,mt_posix) *\/ */

/*         // FIXME: we're assuming we always have: */
/*         // 'requires(-ppx_driver)' and 'requires(-ppx_driver,-custom_ppx)' */

/*         if (settings_ct > 1) { */
/*             fprintf(ostream, "%*s\"%s\": [ ## predicates: %s\n", */
/*                     (1+level)*spfactor, sp, */
/*                     utstring_body(condition_name), // : "//conditions:default", */
/*                     /\* (has_conditions)? "_enabled" : "", *\/ */
/*                     condition_comment); */
/*         } */

/*         vals = resolve_setting_values(setting, flags, settings); */
/*         /\* vals = obzl_meta_setting_values(setting); *\/ */
/*         /\* LOG_DEBUG(0, "vals ct: %d", obzl_meta_values_count(vals)); *\/ */
/*         /\* dump_values(0, vals); *\/ */
/*         /\* now we handle UPDATE settings *\/ */

/*         for (int j = 0; j < obzl_meta_values_count(vals); j++) { */
/*             v = obzl_meta_values_nth(vals, j); */
/*             /\* LOG_INFO(0, "property val: '%s'", *v); *\/ */

/*             char *s = (char*)*v; */
/*             while (*s) { */
/*                 /\* printf("s: %s\n", s); *\/ */
/*                 if(s[0] == '.') { */
/*                     /\* LOG_INFO(0, "Hit"); *\/ */
/*                     s[0] = '/'; */
/*                 } */
/*                 s++; */
/*             } */
/*             if (settings_ct > 1) { */
/*                 fprintf(ostream, "%*s\"%s//%s/%s\",\n", */
/*                         (2+level)*spfactor, sp, */
/*                         repo, pkg, *v); */
/*             } else { */
/*                 fprintf(ostream, "%*s\"%s//%s/%s\"\n", (1+level)*spfactor, sp, repo, pkg, *v); */
/*             } */
/*         } */
/*         if (settings_ct > 1) { */
/*             fprintf(ostream, "%*s],\n", (1+level)*spfactor, sp); */
/*         } */
/*         free(condition_comment); */
/*     } */
/*     utstring_free(condition_name); */
/*     if (settings_no_ppx_driver_ct > 1) */
/*         fprintf(ostream, "%*s}),\n", level*spfactor, sp); */
/*     else */
/*         fprintf(ostream, "%*s],\n", level*spfactor, sp); */
/* } */

int newlevel = 0;

void emit_bazel_subpackages(// char *ws_name,
                            UT_string *opam_switch_lib,
                            char *coswitch_lib,
                            int level,
                            char *_pkg_root,
                            UT_string *pkg_parent,
                            char *_pkg_suffix,
                            char *_filedeps_path,
                            /* char *_pkg_path, */
                            struct obzl_meta_package *_pkg) //,
                            /* UT_array *opam_pending_deps, */
                            /* UT_array *opam_completed_deps) */
                            /* char *_subpkg_dir) */
{
    (void)level;
    TRACE_ENTRY;
#if defined(PROFILE_fastbuild)
    LOG_DEBUG(0, BLU "EMIT_BAZEL_SUBPACKAGES" CRESET " pkg: %s", _pkg->name);
    /* LOG_DEBUG(0, "\tws_name: %s", ws_name); */
    LOG_DEBUG(0, "\t_coswitch_lib: %s", coswitch_lib);
    LOG_DEBUG(0, "\tlevel: %d", level);
    LOG_DEBUG(0, "\t_pkg_root: %s", _pkg_root);
    LOG_DEBUG(0, "\t_pkg_parent: %s", utstring_body(pkg_parent));
    LOG_DEBUG(0, "\t_pkg_suffix: %s", _pkg_suffix); //, _pkg_path);
    LOG_DEBUG(0, "\t_filedeps_path: %s", _filedeps_path);
#endif

    obzl_meta_entries *entries = _pkg->entries;
#if defined(PROFILE_fastbuild)
    LOG_DEBUG(0, "entries ct: %d", obzl_meta_entries_count(entries));
#endif

    obzl_meta_entry *entry = NULL;

    char *pkg_name = obzl_meta_package_name(_pkg);
    UT_string *_new_pkg_suffix;
    utstring_new(_new_pkg_suffix);
    if (_pkg_suffix == NULL)
        utstring_printf(_new_pkg_suffix, "%s", pkg_name);
    else {
        utstring_printf(_new_pkg_suffix, "%s/%s", _pkg_suffix, pkg_name);
    }

    char *curr_parent = strdup(utstring_body(pkg_parent));
    newlevel++;
    if (newlevel > 1) {
        utstring_printf(pkg_parent, "%s", pkg_name);
    }

#if defined(PROFILE_fastbuild)
    LOG_DEBUG(0, "starting level: %d", newlevel);
#endif
    /* UT_string *filedeps_path; */
    for (int i = 0; i < obzl_meta_entries_count(entries); i++) {
        entry = obzl_meta_entries_nth(entries, i);
        if (entry->type == OMP_PACKAGE) {
            obzl_meta_package *subpkg = entry->package;
            /* LOG_DEBUG(0, "SUBPKG: %s", subpkg->name); */
            emit_build_bazel(// ws_name,
                             opam_switch_lib,
                             coswitch_lib,
                             newlevel,
                             _pkg_root,
                             pkg_parent,
                             utstring_body(_new_pkg_suffix), /*  _pkg_suffix */
                             _filedeps_path,
                             /* utstring_body(filedeps_path), */
                             //_pkg_path,
                             subpkg); //,
                             /* opam_pending_deps, */
                             /* opam_completed_deps); */
            /* select only subpackages with non-empty 'directory' prop */
            /* obzl_meta_property *dprop */
            /*     = obzl_meta_entries_property(e->package->entries, */
            /*                                  "directory"); */
            /* if (dprop) { */
            /*     LOG_DEBUG(0, "DPROP %s := %s", */
            /*               obzl_meta_property_name(dprop), */
            /*               obzl_meta_property_value(dprop)); */
            /*     emit_build_bazel(ws_name, coswitch_lib, _pkg_suffix, _pkg_path, e->package); */
            /* } else { */
            /*     LOG_DEBUG(0, "skipping subpackage with empty 'directory' prop"); */
            /* } */
        }
    }
#if defined(PROFILE_fastbuild)
    LOG_DEBUG(0, "Finished level: %d", newlevel);
#endif
    /*  restore */
    newlevel--;
    utstring_renew(pkg_parent);
    utstring_printf(pkg_parent, "%s", curr_parent);
    free(curr_parent);

    utstring_free(_new_pkg_suffix);
    TRACE_EXIT;
}

void handle_directory_property(FILE* ostream, int level,
                               char *_repo,
                               char *_pkg_src,
                               char *_pkg_name,
                               obzl_meta_entries *_entries)

{
    (void)_entries;
    (void)level;
    (void)ostream;
    (void)_pkg_name;
    (void)_pkg_src;
    (void)_repo;
    /*
The variable "directory" redefines the location of the package directory. Normally, the META file is the first file read in the package directory, and before any other file is read, the "directory" variable is evaluated in order to see if the package directory must be changed. The value of the "directory" variable is determined with an empty set of actual predicates. The value must be either: an absolute path name of the alternate directory, or a path name relative to the stdlib directory of OCaml (written "+path"), or a normal relative path name (without special syntax). In the latter case, the interpretation depends on whether it is contained in a main or sub package, and whether the standard repository layout or the alternate layout is in effect (see site-lib for these terms). For a main package in standard layout the base directory is the directory physically containing the META file, and the relative path is interpreted for this base directory. For a main package in alternate layout the base directory is the directory physically containing the META.pkg files. The base directory for subpackages is the package directory of the containing package. (In the case that a subpackage definition does not have a "directory" setting, the subpackage simply inherits the package directory of the containing package. By writing a "directory" directive one can change this location again.)
    */

    /*
      IOW, the 'directory' property tells us where 'archive' and
      'plugin' files are to be found. Does not affect interpretation
      of 'requires' property (I think).
     */
    /*
      Directory '^' means '@rules_ocaml//cfg/lib/ocaml'. problem
      is lib/ocaml has no META file; it's the stdlib, always included.
      what about subdirs like 'threads' or 'integers'?

      If directory starts with '+', pkg is relative to '@rules_ocaml//cfg/lib/ocaml'. problem
      is lib/ocaml has no META file; it's the stdlib, always included.
      what about subdirs like 'threads' or 'integers'?
    */

    /* should have one setting */
    /* char *dir = (char*)*obzl_meta_property_value(e->property); */

    /*
      Special cases: ^, +
    */

    /* char *pkg_name = obzl_meta_package_name(_pkg); */
    /* LOG_DEBUG(0, "Directory prop val: %s", dir); */
    /* LOG_DEBUG(0, "Package name: %s", pkg_name); */
    /* if ( strncmp(dir, pkg_name, PATH_MAX) ) { */
    /*     LOG_ERROR(0, "Mismatch: pkg '%s', directory prop '%s', in %s", */
    /*               pkg_name, dir, THE_METAFILE); */
        /* fclose(ostream); */
        /* exit(EXIT_FAILURE); */
    /* } */
}

EXPORT void emit_workspace_file(UT_string *ws_file, const char *repo_name)
{
    (void)repo_name;
    TRACE_ENTRY;
    errno = 0;
    FILE *ostream;
    ostream = fopen(utstring_body(ws_file), "w");
    if (ostream == NULL) {
        LOG_DEBUG(0, "XXXX", "");

        LOG_ERROR(0, 0, "%s", strerror(errno));
        perror(utstring_body(ws_file));
        exit(EXIT_FAILURE);
    /* } else { */
    /*     LOG_DEBUG(0, "YYYYYYYYYYYYYYYY"); */
    }

    fprintf(ostream, "## generated file - DO NOT EDIT\n");
    /* fprintf(ostream, "workspace( name = \"%s\" )\n", repo_name); */

    fclose(ostream);
    if (verbosity > log_writes)
        fprintf(INFOFD, GRN "INFO" CRESET
                " wrote: %s\n", utstring_body(ws_file));
    TRACE_EXIT;
}

EXPORT void emit_module_file(UT_string *module_file,
                             char *compiler_version,
                             struct obzl_meta_package *_pkg,
                             struct obzl_meta_package *_pkgs)
{
    TRACE_ENTRY_MSG("%s", _pkg->name);
    findlib_version_t *semv;
    char version[256];
    semv = findlib_pkg_version(_pkg);
    sprintf(version, "%d.%d.%d",
            semv->major, semv->minor, semv->patch);

    FILE *ostream;
    ostream = fopen(utstring_body(module_file), "w");
    if (ostream == NULL) {
        LOG_ERROR(0, "fopen fail: %s %s",
                  utstring_body(module_file),
                  strerror(errno));
        perror(utstring_body(module_file));
        exit(EXIT_FAILURE);
    }

    fprintf(ostream, "## generated file - DO NOT EDIT\n\n");

    fprintf(ostream, "module(\n");
    fprintf(ostream, "    name = \"%s\",\n", _pkg->module_name);
    fprintf(ostream, "    version = \"%s\",  # %s\n",
            default_version, version);
    fprintf(ostream, "    compatibility_level = %d, # %d\n",
            default_compat, semv->major);
    fprintf(ostream, "    bazel_compatibility = [\">=%s\"]\n",
            bazel_compat);
    fprintf(ostream, ")\n");
    fprintf(ostream, "\n");

    fprintf(ostream, "bazel_dep(name = \"rules_ocaml\",");
    fprintf(ostream, " version = \"%s\")\n", rules_ocaml_version);
    fprintf(ostream, "bazel_dep(name = \"ocaml\", # %s\n",
            compiler_version);
    fprintf(ostream, "          version = \"0.0.0\")\n");
    fprintf(ostream, "\n");

    // get **repo** deps: direct deps of pkg and all subpkgs
    UT_array *pkg_deps = findlib_pkg_deps(_pkg, true);
    if (pkg_deps) {
        char **p = NULL;
        struct obzl_meta_package *pkg;
        /* LOG_DEBUG(0, "HASH CT: %d", HASH_COUNT(_pkgs)); */
        /* exit(0); */
        while ( (p=(char**)utarray_next(pkg_deps, p))) {
            if (strncmp(*p, _pkg->name, 512) != 0) {
                HASH_FIND_STR(_pkgs, *p, pkg);
                if (pkg) {
                    free(semv);
                    version[0] = '\0';
                    semv = findlib_pkg_version(pkg);
                    sprintf(version, "%d.%d.%d",
                            semv->major, semv->minor, semv->patch);
                } else {
                    //FIXME: pkg 'compiler-libs' (a dep of
                    // ppxlib.astlib etc.) is a pseudo-pkg,
                    // referring to lib/ocaml/compiler-libs,
                    // so it has neither pkg entry nor version
                    if (strncmp(*p, "compiler-libs", 13) == 0) {
                        sprintf(version, "%d.%d.%d", 0,0,0);
                    } else {
                        sprintf(version, "%d.%d.%d", -1, -1 , -1);
                    }
                }
                fprintf(ostream, "bazel_dep(name = \"%s\", # %s\n",
                        *p, version);
                fprintf(ostream, "          version = \"%s\")\n",
                        default_version);
            }
        }
    }

    /* HACK ALERT! This hideous code deals with ppx_runtime_deps,
       and must check to prevent duplicates.
     */
    char **already = NULL;      /* for searching */
    if (pkg_deps) {
        utarray_sort(pkg_deps,strsort);
    }
    UT_array *pkg_codeps = findlib_pkg_codeps(_pkg, true);
    if (pkg_codeps) {
        char **p = NULL;
        struct obzl_meta_package *pkg;
        /* LOG_DEBUG(0, "HASH CT: %d", HASH_COUNT(_pkgs)); */
        /* exit(0); */
        while ( (p=(char**)utarray_next(pkg_codeps, p))) {
            if (strncmp(*p, _pkg->name, 512) != 0) {
                HASH_FIND_STR(_pkgs, *p, pkg);
                if (pkg) {
                    free(semv);
                    version[0] = '\0';
                    semv = findlib_pkg_version(pkg);
                    sprintf(version, "%d.%d.%d",
                            semv->major, semv->minor, semv->patch);
                } else {
                    //FIXME: pkg 'compiler-libs' (a dep of
                    // ppxlib.astlib etc.) is a pseudo-pkg,
                    // referring to lib/ocaml/compiler-libs,
                    // so it has neither pkg entry nor version
                    if (strncmp(*p, "compiler-libs", 13) == 0) {
                        sprintf(version, "%d.%d.%d", 0,0,0);
                    } else {
                        sprintf(version, "%d.%d.%d", -1, -1 , -1);
                    }
                }
                if (pkg_deps) {
                    already = NULL;
                    already = (char**)utarray_find(pkg_deps, p, strsort);
                    if (already == NULL) {
                        fprintf(ostream, "bazel_dep(name = \"%s\", # %s\n",
                                *p, version);
                        fprintf(ostream, "          version = \"%s\") #codep\n",
                                default_version);
                    }
                }
            }
        }
    }

    fprintf(ostream, "\n");

    fclose(ostream);
    if (verbosity > log_writes) {
        fprintf(INFOFD, GRN "INFO" CRESET
                " wrote: %s\n", utstring_body(module_file));
    }
    if (pkg_deps) {
        utarray_free(pkg_deps);
    }
    if (pkg_codeps) {
        utarray_free(pkg_codeps);
    }
    TRACE_EXIT;
}

void emit_pkg_symlinks(UT_string *opam_switch_lib,
                       UT_string *dst_dir,
                       UT_string *src_dir,
                       char *pkg_name)
{
    TRACE_ENTRY;
#if defined(PROFILE_fastbuild)
    if (verbosity > 1)
        LOG_DEBUG(0, "  pkg name: %s", pkg_name);
#endif

/*     if ((strncmp(pkg_name, "dune", 4) == 0)) { */
/*         || (strncmp(utstring_body(src_dir), "dune/configurator", 17) == 0)) { */
/* #if defined(PROFILE_fastbuild) */
/*         if (coswitch_debug) */
/*             LOG_DEBUG(0, "Skipping 'dune' stuff\n"); */
/* #endif */
/*         return; */
/*     } */
    LOG_DEBUG(0, "\tpkg_symlinks src_dir: %s", utstring_body(src_dir));
    LOG_DEBUG(0, "\tpkg_symlinks dst_dir: %s", utstring_body(dst_dir));

    UT_string *opamdir;
    utstring_new(opamdir);
    utstring_printf(opamdir, "%s/%s",
                    utstring_body(opam_switch_lib),
                    //pkg_name
                    utstring_body(src_dir)
                    );

    UT_string *src;
    utstring_new(src);
    UT_string *dst;
    utstring_new(dst);
    int rc;

#if defined(PROFILE_fastbuild)
    LOG_DEBUG(0, "opening src_dir for read: %s", utstring_body(opamdir));
#endif
    DIR *d = opendir(utstring_body(opamdir));
    if (d == NULL) { // inaccessible
        if (strncmp(pkg_name, "care", 4) == 0) {
            if (strncmp(utstring_body(src_dir),
                        "topkg/../topkg-care",
                        19) == 0) {
                LOG_DEBUG(0, "Skipping missing %s", "'topkg-care'");
                if (verbosity > 2)
                    log_info("Skipping missing pkg: %s", "'topkg-care'");
                return;
            }
        } else if (strncmp(pkg_name, "configurator", 12) == 0) {
            char *s = utstring_body(opamdir);
            int len = strlen(s);
            char *ptr = s + len - 21;
            /* LOG_INFO(0, "XXXX src_dir: %s", s); */
            /* LOG_INFO(0, "XXXX configurator len: %d", len); */
            /* LOG_INFO(0, "XXXX configurator ptr: %s", ptr); */
            if (len > 20) {
                if (strncmp(ptr, "lib/dune/configurator", 21) == 0) {
                    if (verbosity > 2) {
                        LOG_WARN(0, "Skipping dune/configurator; use @dune-configurator instead.", "");
                    }
                    return;
                } else {
                    LOG_ERROR(0, "MISMATCH dune/configurator", "");
                    exit(0);
                }
            /* } else { */
            /*     LOG_DEBUG(0, "configurator not dune/configurator"); */
            }
        } else {
            LOG_WARN(0, "pkg_symlinks: Unable to opendir %s",
                     utstring_body(opamdir));
        }
        /* exit(EXIT_FAILURE); */
        /* this can happen if a related pkg is not installed */
        /* example, see topkg and topkg-care */
        return;
    }

    struct dirent *direntry;
    while ((direntry = readdir(d)) != NULL) {
        //Condition to check regular file.
        if(direntry->d_type==DT_REG){ // or symlink?
/* #if defined(PROFILE_fastbuild) */
/*             if (coswitch_debug) { */
/*                 LOG_DEBUG(0, "dirent: %s/%s", */
/*                           utstring_body(opamdir), direntry->d_name); */
/*             } */
/* #endif */

            if (strncmp(direntry->d_name, "WORKSPACE", 9) == 0) {
                continue;
            }

            utstring_renew(src);
            utstring_printf(src, "%s/%s",
                            utstring_body(opamdir), direntry->d_name);
            utstring_renew(dst);
            utstring_printf(dst, "%s/%s/%s",
                            utstring_body(dst_dir),
                            pkg_name,
                            direntry->d_name);
/* #if defined(PROFILE_fastbuild) */
/*             if (coswitch_debug) { */
                /* LOG_DEBUG(0, "pkg symlinking"); */
                /* fprintf(stderr, "  from %s\n", utstring_body(src)); */
                /* fprintf(stderr, "  to   %s\n", utstring_body(dst)); */
/*             } */
/* #endif */
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

/*
  params:
  ws_name: "ocaml"
  coswitch_lib:
  _tgtroot: output dir

  _pkg_prefix - result of concatenating ancestors up to parent; starts
                with bzl_lib (default: "lib")

  pkg path will be made by concatenating _pkg_prefix to pkg name

  pending/completed deps: used to avoid duplication of effort. When
  retrieving list of deps for a pkg, we check to see if its already
  been processed, or if its already in the list of pending deps. If
  neither, we add it to pending deps.
 */
EXPORT void emit_build_bazel(// char *ws_name,
                             UT_string *opam_switch_lib,
                             char *coswitch_lib,
                             int level,
                             char *_pkg_root,
                             UT_string *pkg_parent,
                             char *_pkg_suffix,
                             char *_filedeps_path,
                             struct obzl_meta_package *_pkg) //,
                             /* UT_array *opam_pending_deps, */
                             /* UT_array *opam_completed_deps) */
/* FILE *bootstrap_FILE) */
                             /* char *_subpkg_dir) */
{
    TRACE_ENTRY;
    /* ENTRY */
#if defined(PROFILE_fastbuild)
    LOG_INFO(0, "\tpkg: %s", obzl_meta_package_name(_pkg));
    LOG_INFO(0, "\tpkg->name: %s", obzl_meta_package_name(_pkg));
    LOG_INFO(0, "\tcoswitch_lib: %s", coswitch_lib);
    LOG_INFO(0, "\tlevel: %d", level);
    LOG_INFO(0, "\t_pkg_root: %s", _pkg_root);
    LOG_INFO(0, "\tpkg_parent: '%s'", utstring_body(pkg_parent));
    LOG_INFO(0, "\t_pkg_suffix: '%s'", _pkg_suffix);
    LOG_INFO(0, "\t_filedeps_path: %s", _filedeps_path);
#endif
    /* dump_package(0, _pkg); */

    if (strncmp(_pkg->name, "compiler-libs", 13) == 0) {
        /* handled separately */
        return;
    }

    if (strncmp(_pkg->name, "compiler-libs", 13) == 0) {
        /* handled separately */
        return;
    }

    /* int rc = pkg_deps(_pkg, opam_pending_deps, opam_completed_deps); */

    /* char **p = NULL; */
    /* while ( (p=(char**)utarray_next(opam_completed_deps, p))) { */
    /*     LOG_DEBUG(0, RED "  completed dep:" CRESET " %s",*p); */
    /* } */
    /* while ( (p=(char**)utarray_next(opam_pending_deps, p))) { */
    /*     LOG_DEBUG(0, CYN "  pending dep:" CRESET " %s",*p); */
    /* } */

    /* if (strncmp(_pkg->name, "ppx_fixed_literal", 17) == 0) { */
    /*     LOG_DEBUG(0, "PPX_FIXED_LITERAL dump:"); */
    /*     dump_package(0, _pkg); */
    /* } */

    char *pkg_name = obzl_meta_package_name(_pkg);
#if defined(PROFILE_fastbuild)
    /* log_set_quiet(false); */
    /* LOG_DEBUG(0, "%*sparsed name: %s", indent, sp, pkg_name); */
    /* LOG_DEBUG(0, "%*spkg dir:  %s", indent, sp, obzl_meta_package_dir(_pkg)); */
    /* LOG_DEBUG(0, "%*sparsed src:  %s", indent, sp, obzl_meta_package_src(_pkg)); */
    LOG_DEBUG(0, "computing output paths", "");
    /* LOG_INFO(0, "COSWITCH_LIB: %s", coswitch_lib); */
    LOG_INFO(0, "PFXDIR: %s", pfxdir);
#endif

    utstring_renew(bazel_pkg_root);
    utstring_renew(build_bazel_file);
    /* utstring_clear(build_bazel_file); */

/*     if (_pkg_suffix == NULL) { */
/*         if (pfxdir == NULL) { */
/* #if defined(PROFILE_fastbuild) */
/*             LOG_DEBUG(0, "NULL NULL"); */
/* #endif */
/*             utstring_printf(build_bazel_file, "%s/%s", coswitch_lib, pkg_name); */
/*         } else { */
/* #if defined(PROFILE_fastbuild) */
/*             LOG_DEBUG(0, "NULL NOTNULL"); */
/* #endif */
/*             utstring_printf(build_bazel_file, "%s/%s", coswitch_lib, pfxdir); */
/*         } */
/*     } else { */
/* #if defined(PROFILE_fastbuild) */
/*         LOG_DEBUG(0, "_pkg_suffix != NULL"); */
/* #endif */
/*         if (pfxdir == NULL) { */
/* #if defined(PROFILE_fastbuild) */
/*             LOG_DEBUG(0, "pfxdir == NULL"); */
/* #endif */
/*             utstring_printf(build_bazel_file, "%s/%s", */
/*                             coswitch_lib, _pkg_suffix); */
/*         } else { */
/* #if defined(PROFILE_fastbuild) */
/*             LOG_DEBUG(0, "NOTNULL NOTNULL"); */
/* #endif */
/*             utstring_printf(build_bazel_file, "%s/%s/%s", */
/*                             coswitch_lib, pfxdir, _pkg_suffix); */
/*         } */
/*     } */
#if defined(PROFILE_fastbuild)
    LOG_DEBUG(0, "build_bazel_file: %s", utstring_body(build_bazel_file));
#endif
    /* printf("_PKG_SUFFIX: %s\n", _pkg_suffix); */

    /* WARNING: pkg may be empty (no entries) */
    /* e.g. js_of_ocaml contains: package "weak" ( ) */
    obzl_meta_entries *entries = _pkg->entries;
    UT_string *new_filedeps_path = NULL;
    utstring_renew(new_filedeps_path);
    char *directory = obzl_meta_directory_property(entries);
#if defined(PROFILE_fastbuild)
    LOG_DEBUG(0, "DIRECTORY property: '%s'", directory);
#endif
    if ( directory == NULL ) {
/* #if defined(PROFILE_fastbuild) */
/*         LOG_DEBUG(0, "AAAA"); */
/* #endif */
        /* directory omitted or directory = "" */
        /* if (_filedeps_path != NULL) */
        utstring_printf(new_filedeps_path, "%s", _filedeps_path);
    } else {
        /* utstring_printf(build_bazel_file, "/%s", subpkg_dir); */
        if ( strncmp(directory, "+", 1) == 0 ) {
#if defined(PROFILE_fastbuild)
            LOG_DEBUG(0, "BBBB", "");
#endif
            /* Initial '+' means 'relative to stdlib dir', i.e. these
               are libs found in lib/ocaml. used by: compiler-libs, ?
               subdirs of lib/ocaml: stublibs, caml, compiler-libs,
               threads, threads/posix, ocamldoc.
            */
            /* stdlib = true; */
#if defined(PROFILE_fastbuild)
            LOG_DEBUG(0, "Found STDLIB directory '%s' for %s", directory, pkg_name);
#endif
            directory++;
            utstring_printf(new_filedeps_path, "ocaml/%s", directory);
            /* utstring_printf(new_filedeps_path, "_lib/ocaml/%s", directory); */
        } else {
            if (strncmp(directory, "^", 1) == 0) {
                LOG_DEBUG(0, "STDLIB REDIRECT ^", "");
                /* Initial '^' means pkg within stdlib, i.e.
                   lib/ocaml; "Only included because of
                   findlib-browser" or "distributed with Ocaml". only
                   applies to: raw-spacetime, num, threads, bigarray,
                   unix, stdlib, str, dynlink, ocamldoc.
                   for <5.0.0, in the opam
                   repo these only have a META file, which redirects
                   to files or subdirs in lib/ocaml.
                   for >=5.0.0, these have no toplevel dir w/META;
                   instead they have a subdir w/META inside lib/ocaml,
                   which does not use '^'
                */
                /* stdlib = true; */
                directory++;
                /* stdlib_root = true; */
                if (strlen(directory) == 1)
                    utstring_printf(new_filedeps_path, "%s", "ocaml");
                else
                    utstring_printf(new_filedeps_path,
                                    "ocaml/%s",
                                    directory);
                /* utstring_printf(new_filedeps_path, "_lib/%s", "ocaml"); */
#if defined(PROFILE_fastbuild)
                LOG_DEBUG(0, "Found STDLIB root directory for %s: '%s' => '%s'",
                          pkg_name, directory, utstring_body(new_filedeps_path));
#endif
            } else {
                utstring_printf(new_filedeps_path,
                                "%s/%s", _filedeps_path, directory);
            }
        }
    }
#if defined(PROFILE_fastbuild)
    LOG_DEBUG(0, "new_filedeps_path: %s", utstring_body(new_filedeps_path));
#endif

    /* if (obzl_meta_package_dir(_pkg) != NULL) */

    /* printf("PKG_NAME: %s\n", pkg_name); */

    /* utstring_renew(workspace_file); */
    /* utstring_concat(workspace_file, build_bazel_file); */
    /* utstring_printf(workspace_file, "/%s", "WORKSPACE.bazel"); */

    utstring_renew(bazel_pkg_root); /* OUTPUT dir */
    /* utstring_printf(bazel_pkg_root, "%s/%s/lib", coswitch_lib, _pkg_root); */
    utstring_printf(bazel_pkg_root, "%s/%s/lib", coswitch_lib, _pkg_root);

    if (level > 1) {
        /* e.g. a.b.c => <switch>/lib/a/lib/b/c = @a//lib/b/c */
        utstring_printf(bazel_pkg_root, "/%s", utstring_body(pkg_parent));
    }
    /* /\* do not create pkg subdir for pkg itself *\/ */
    /* /\* target will be @foo//:foo, not @foo//foo *\/ */
    /* if (strncmp(_pkg_root, pkg_name, strlen(pkg_name)) != 0) { */
    /*     utstring_printf(bazel_pkg_root, "/%s", pkg_name); */
    /* } */
    LOG_DEBUG(0, "bazel_pkg_root: %s", utstring_body(bazel_pkg_root));

    /* utstring_printf(build_bazel_file, "/%s", pkg_name); */

    /* utstring_renew(bazel_pkg_root); */
    /* utstring_concat(bazel_pkg_root, build_bazel_file); */

    /* mkdir_r(utstring_body(bazel_pkg_root)); */

    utstring_renew(build_bazel_file);
    utstring_printf(build_bazel_file, "%s/%s",
                    utstring_body(bazel_pkg_root),
                    pkg_name);
    mkdir_r(utstring_body(build_bazel_file));
    utstring_printf(build_bazel_file, "/%s", "BUILD.bazel");

    LOG_DEBUG(0, "bazel_pkg_root: %s", utstring_body(bazel_pkg_root));
    LOG_DEBUG(0, CYN "build_bazel_file:" CRESET, "");
    LOG_DEBUG(0, CYN "\t%s" CRESET, utstring_body(build_bazel_file));

    errno = 0;
    int rc = access(utstring_body(build_bazel_file), F_OK);
    if (rc == 0) {
        /* OK - file already exists*/
    }

    /* if (rc < 0) { */
    /*     LOG_INFO(0, "EMITTING: %s", utstring_body(build_bazel_file)); */
    /* } else { */
    /*     LOG_INFO(0, "ALREADY EMITTED: %s", utstring_body(build_bazel_file)); */
    /*     return; */
    /* } */

    /* ################################################################ */
    /* emit_new_local_pkg_repo(bootstrap_FILE, */
    /*                         _pkg_suffix, */
    /*                         _pkg); */

#if defined(PROFILE_fastbuild)
    LOG_DEBUG(0, "fopening for write: %s", utstring_body(build_bazel_file));
#endif

    FILE *ostream;
    ostream = fopen(utstring_body(build_bazel_file), "w");
    if (ostream == NULL) {
        perror(utstring_body(build_bazel_file));
        LOG_ERROR(0, "fopen failure for %s", utstring_body(build_bazel_file));
        /* LOG_ERROR(0, "Value of errno: %d", errno); */
        /* LOG_ERROR(0, "fopen error %s", strerror( errnum )); */
        exit(EXIT_FAILURE);
    }

    /* SPECIAL CASE HANDLING

       <5.0.0 : toplevel num, bigarray, unix, threads,
       str, ocamldoc, dynlink

       >=5.0.0 : num only?
     */
    if ((_pkg_suffix == NULL)
        || ((strncmp(_pkg_suffix, "ocaml", 5) == 0) )) {
            /* && (strlen(_pkg_suffix) == 5))) */
        if (emit_special_case_rule(ostream, _pkg)) {
#if defined(PROFILE_fastbuild)
            LOG_TRACE(0, "+emit_special_case_rule:TRUE", "");
            LOG_TRACE(0, "\tpkg suffix: %s", _pkg_suffix);
            LOG_TRACE(0, "\tpkg name: %s", _pkg->name);
#endif
            return;
#if defined(PROFILE_fastbuild)
            LOG_TRACE(0, "-emit_special_case_rule:FALSE %s", _pkg->name);
#endif
        }
    }

    if (strncmp(_pkg_root, "ctypes", 6) == 0) {
        if (strncmp(_pkg->name, "foreign", 7) == 0) {
            if (emit_special_case_rule(ostream, _pkg)) {
                /* #if defined(PROFILE_fastbuild) */
                /* LOG_TRACE(0, "emit_special_case_rule:TRUE %s", _pkg->name); */
                /* #endif */
                return;
            }
        }
    }
    /* **************************************************************** */
    /* if (_pkg->entries != NULL) { */
        /* if (strncmp(pkg_name, _pkg_root, strlen(pkgname)) == 0) { */
    /*FIXME symlinks only needed for base pkg, not subpkgs */
    emit_pkg_symlinks(opam_switch_lib,
                      bazel_pkg_root, /* dest */
                      new_filedeps_path, /* src */
                      pkg_name);
        /* } */
    /* } */

    fprintf(ostream, "## generated file - DO NOT EDIT\n");

    fprintf(ostream, "## original: %s\n\n", obzl_meta_package_src(_pkg));

    /*FIXME: META file does not contain info about executables
      generated by dune-package. E.g. ppx_tools. So we can't emit
      target code for them. For now we just export everything.

      options: iterate over files and pick out executables; or parse
      the dune-package file.
    */
    fprintf(ostream, "package(default_visibility=[\"//visibility:public\"])\n");
    fprintf(ostream, "exports_files(glob([\"**\"]))\n\n");

    emit_bazel_hdr(ostream); //, 1, ocaml_ws, "lib", _pkg);

    /* special case */
    if ((strncmp(pkg_name, "ctypes", 6) == 0)
        && strlen(pkg_name) == 6) {
        //FIXME: use a template file?
        /* "load(\"@rules_cc//cc:defs.bzl\", \"cc_library\")\n" */
        fprintf(ostream,
                "cc_library(\n"
                "    name = \"hdrs\",\n"
                /* "    srcs = glob([\"*.a\"]),\n" */
                "    hdrs = glob([\"*.h\"]),\n"
                /* "    visibility = [\"//visibility:public\"],\n" */
                ")\n");
    }

    /* special case */
    if ((strncmp(pkg_name, "cstubs", 6) == 0)
        && strlen(pkg_name) == 6) {
        /* "load(\"@rules_cc//cc:defs.bzl\", \"cc_library\")\n" */
        fprintf(ostream,
                "cc_library(\n"
                "    name = \"hdrs\",\n"
                /* "    srcs = glob([\"*.a\"]),\n" */
                "    hdrs = glob([\"*.h\"]),\n"
                /* "    visibility = [\"//visibility:public\"],\n" */
                ")\n");
    }

    /* if (strncmp(_pkg->name, "ppx_fixed_literal", 17) == 0) { */
    /*     LOG_DEBUG(0, "PPX_FIXED_LITERAL dump:"); */
    /*     /\* dump_package(0, _pkg); *\/ */

    /*     LOG_DEBUG(0, "PPX_FIXED_LITERAL ENTRIES count: %d", */
    /*               obzl_meta_entries_count(_pkg->entries)); */
    /*     /\* dump_entries(0, _pkg->entries); *\/ */
    /* } */

    /* if (strncmp(_pkg->name, "base", 17) == 0) { */
    /*     LOG_DEBUG(0, "BASE dump:"); */
    /*     dump_package(0, _pkg); */
    /* } */

    emit_bazel_cc_imports(ostream, 1,
                          _pkg_suffix,
                          pkg_name,
                          _filedeps_path,
                          entries,
                          _pkg);

    obzl_meta_entry *e = NULL;
    if (_pkg->entries == NULL) {
        LOG_DEBUG(0, "EMPTY ENTRIES", "");
    } else {
        for (int i = 0; i < obzl_meta_entries_count(_pkg->entries); i++) {
            e = obzl_meta_entries_nth(_pkg->entries, i);
	    /* fprintf(stdout, "Processing %s\n", _pkg->name); */

            /* if (strncmp(_pkg->name, "ppx_fixed_literal", 17) == 0) */
            /*     LOG_DEBUG(0, "PPX_FIXED_LITERAL, entry %d, name: %s, type %d", */
            /*               i, e->property->name, e->type); */

            /*
              FIXME: some packages use plugin or toploop flags in an
              'archive' property. We need to check the archive property,
              and if this is the case, generate separate import targets
              for them. Unlike findlib we do not use flags to select the
              files we want; instead we expose everything using
              ocaml_import targets.
            */

            if (e->type == OMP_PROPERTY) {

                /* if ((e->type == OMP_PROPERTY) || (e->type == OMP_PACKAGE)) { */
                /* LOG_DEBUG(0, "\tOMP_PROPERTY"); */

                /* 'library_kind` :=  'ppx_deriver' or 'ppx_rewriter' */
                /* if (strncmp(e->property->name, "library_kind", 12) == 0) { */
                /*     emit_bazel_ppx_dummy_rule(ostream, 1, ocaml_ws, "_lib", _pkg_path, pkg_name, entries); */
                /*     continue; */
                /* } */

                if (strncmp(e->property->name, "archive", 7) == 0) {
                    emit_bazel_archive_rule(ostream,
                                            coswitch_lib,
                                            1, /* indent level */
                                            /* ws_name, // ocaml_ws, */
                                            //_filedeps_path,
                                            utstring_body(new_filedeps_path),
                                            _pkg_root,
                                            _pkg_suffix,
                                            pkg_name,
                                            pkg_parent,
                                            entries, //);
                                            _pkg);
                    /* subpkg_dir); */
                    continue;
                }

                /* LOG_DEBUG(0, "PDUMP0 %s", _pkg->name); */
                /* dump_entries(0, entries); */

                if (strncmp(e->property->name, "plugin", 6) == 0) {
                    /* LOG_DEBUG(0, "PDUMPPPP plugin"); */
                    emit_bazel_plugin_rule(ostream, 1,
                                           ocaml_ws,
                                           utstring_body(new_filedeps_path),
                                           _pkg_suffix,
                                           pkg_name,
                                           entries, //);
                                           _pkg);
                    continue;
                }
                /* if (strncmp(e->property->name, "ppx", 3) == 0) { */
                /*     LOG_WARN(0, "IGNORING PPX PROPERTY for %s", pkg_name); */
                /*     continue; */
                /* } */

                /* at this point we've processed archive and plugin props.
                   now we handle pkgs that have deps ('requires') but do
                   not deliver any files ('archive' or 'plugin').
                */

                if (strncmp(e->property->name, "requires", 6) == 0) {
                    /* some pkgs have 'requires' but no 'archive' nor 'plugin' */
                    /* compiler-libs has 'requires = ""' and 'director = "+compiler-libs"' */
                    /* ppx packages have 'requires(ppx_driver)' and 'requires(-ppx_driver)' */

                    /* we should've already handled these 2: */
                    if (obzl_meta_entries_property(entries, "archive"))
                        continue;
                    if (obzl_meta_entries_property(entries, "plugin"))
                        continue;

                    /* compatibility pkgs whose resources are now
                       'distributed with OCaml' may not have 'archive' or
                       'plugin', but we still need to generate a target,
                       since they have deps ('requires') that legacy code
                       may depend on. */

                    /* UPDATE: we now generate the standard dummy pkgs
                       like bigarray and threads using a repo rule */
                    emit_bazel_deps_target(ostream, 1, ocaml_ws,
                                           /* _pkg_suffix, */
                                           pkg_name, entries);
                    continue;
                }

                /* **************** */
                //TODO:FIXME: only if --enable-jsoo passed
                /* if (strncmp(e->property->name, "jsoo_runtime", 12) == 0) { */
                /*     obzl_meta_value ds = obzl_meta_property_value(e->property); */
                /*     fprintf(ostream, "\njsoo_import(\n"); */
                /*     fprintf(ostream, "    name = \"jsoo_runtime\",\n"); */
                /*     fprintf(ostream, "    src = \"%s\",\n", (char*)ds); */
                /*     fprintf(ostream, ")\n"); */
                /* } */


                /* **************** */
                /* if (strncmp(_pkg->name, "ppx_fixed_literal", 17) == 0) */
                /*     LOG_DEBUG(0, "PPX_fixed_literal, entry %d, name: %s, type %d", */
                /*               i, e->property->name, e->type); */
                /* no 'archive', 'plugin', or 'requires' */
                /* if (strncmp(e->property->name, "directory", 9) == 0) { */
                /*     handle_directory_property(ostream, 1, ocaml_ws, */
                /*                               "_lib", pkg_name, entries); */
                /*     continue; */
                /* } */

                if (obzl_meta_entries_property(entries, "error")) {
                    emit_bazel_error_target(ostream, 1, ocaml_ws,
                                            "FIXME_ERROR",
                                            /* _pkg_suffix, */
                                            pkg_name, entries);
                    break;
                }

#if defined(PROFILE_fastbuild)
                LOG_WARN(0, "processing other property: %s", e->property->name);
#endif
                /* dump_entry(0, e); */
                // push all flags to global pos_flags
            }
        }
    }
    /* emit_bazel_flags(ostream, coswitch_lib, ws_name, _pkg_suffix, _pkg); */

    /* LOG_DEBUG(0, "fclosing %s", utstring_body(build_bazel_file)); */
    fclose(ostream);
    if (verbosity > log_writes) {
        fprintf(INFOFD, GRN "INFO" CRESET
                " wrote: %s\n", utstring_body(build_bazel_file));
    }
    /* printf("pkg pfx: %s, pkg name: %s\n", _pkg_suffix, pkg_name); */
    /* if (_pkg_suffix == NULL) */
    /*     emit_workspace_file(workspace_file, pkg_name); */


    if (_pkg->entries != NULL) {
        /* emit_pkg_symlinks(bazel_pkg_root, */
        /*                   new_filedeps_path, */
        /*                   pkg_name); */

        /* char new_pkg_path[PATH_MAX]; */
        /* new_pkg_path[0] = '\0'; */
        /* mystrcat(new_pkg_path, _pkg_path); */
        /* if (strlen(_pkg_path) > 0) */
        /*     mystrcat(new_pkg_path, "/"); */

        /* subpackage may not have a 'directory' property. example: ptime */
        /* char  *subpkg_name = obzl_meta_package_name(_pkg); */
        /* char *subpkg_dir = obzl_meta_directory_property(entries); */
        /* LOG_DEBUG(0, "SUBPKG NAME: %s; DIR: %s", subpkg_name, subpkg_dir); */

        /* mystrcat(new_pkg_path, subpkg_name); */
        /* LOG_DEBUG(0, "NEW_PKG_PATH: %s; PFX: %s", new_pkg_path, _pkg_suffix); */
        /* LOG_DEBUG(0, "_PKG_SUFFIX: %s, _PKG_PATH: %s, _PKG: %s", */
        /*           _pkg_suffix, _pkg_path, _pkg->name); */

        /* SPECIAL CASE: compiler-libs */
        /* if (strncmp(_pkg_path, "compiler-libs", 13) == 0) { */
        /*     /\* emit_bazel_compiler_lib_subpackages(coswitch_lib, ws_name, _pkg_suffix, new_pkg_path, _pkg); *\/ */
        /*     emit_bazel_subpackages(ws_name, coswitch_lib, */
        /*                            _pkg_suffix, _filedeps_path, */
        /*                            // _pkg_path, */
        /*                            _pkg); //, _subpkg_dir); */
        /* } else { */

        /* if pkg exports executables, emit a bin/ subdir */
        /* first check to see if it already has it */

        // IF pkg contains subpkgs!

        emit_bazel_subpackages(// ws_name,
                               opam_switch_lib,
                               coswitch_lib,
                               level,
                               _pkg_root,
                               pkg_parent,
                               _pkg_suffix,
                               utstring_body(new_filedeps_path),
                               //_filedeps_path,
                               // new_pkg_path,
                               _pkg); //, _subpkg_dir);
                               /* opam_pending_deps, */
                               /* opam_completed_deps); */
    }
}

