//FIXME: support -j (--jsoo-enable) flag

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>

#if INTERFACE
#ifdef __linux__
#include <linux/limits.h>
#else
#include <limits.h>
#endif
#endif
#include <sys/stat.h>
#include <unistd.h>

#include "cwalk.h"
#include "gopt.h"
#include "libs7.h"
#include "log.h"
#include "findlibc.h"
#include "opamc.h"
#include "semver.h"
#include "utarray.h"
#include "uthash.h"
#include "utstring.h"
#include "xdgc.h"

#include "coswitch.h"

bool bazel_env;

bool xdg_install = false;

static UT_string *meta_path;

static char *switch_name;
static char *coswitch_name; // may be "local"

#if defined(DEBUG_fastbuild)
bool coswitch_trace;
int  coswitch_debug;
extern bool findlibc_trace;
extern int  findlibc_debug;
extern bool opamc_trace;
extern int  opamc_debug;
#endif

bool quiet;
bool verbose;
int  verbosity;

int level = 0;
int spfactor = 2;
char *sp = " ";

s7_scheme *s7;

UT_string *imports_path;
UT_string *pkg_parent;

struct paths_s {
    UT_string *registry;
    UT_string *coswitch_lib;
    struct obzl_meta_package *pkgs;
};

/* UT_string *coswitch_runfiles_root; */

char *default_version = "0.0.0";
int   default_compat  = 0;
char *bazel_compat    = "6.0.0";
char *rules_ocaml_version = "2.0.0";
char *skylib_version      = "1.4.2";

int log_writes = 1; // threshhold for logging all writes
int log_symlinks = 2;
/* extern */ bool enable_jsoo;

char *pkg_path = NULL;

enum OPTS {
    OPT_PKG = 0,
    OPT_SWITCH,
    FLAG_JSOO,
    FLAG_XDG_INSTALL,
    FLAG_CLEAN,
    FLAG_SHOW_CONFIG,
#if defined(DEBUG_fastbuild)
    FLAG_DEBUG,
    FLAG_TRACE,
    OPT_DEBUG_FINDLIBC,
    FLAG_TRACE_FINDLIBC,
    OPT_DEBUG_OPAMC,
    FLAG_TRACE_OPAMC,
#endif
    FLAG_VERBOSE,
    FLAG_QUIET,
    FLAG_HELP,
    LAST
};

void _print_usage(void) {
    printf("Usage:\t$ bazel run @coswitch//new [flags, options]\n");

    printf("Flags\n");
    /* printf("\t-j, --jsoo\t\t\tImport Js_of_ocaml resources.\n"); */
    /* printf("\t-c, --clean\t\t\tClean coswitch and reset to uninitialized state. (temporarily disabled)\n"); */

#if defined(DEBUG_fastbuild)
    printf("\t-d, --debug\t\t\tEnable debug flags. Repeatable.\n");
    printf("\t-t, --trace\t\t\tEnable all trace flags (debug only).\n");
#endif
    printf("\t-v, --verbose\t\t\tEnable verbosity. Repeatable.\n");
}

static struct option options[] = {
    /* 0 */
    [OPT_PKG] = {.long_name="pkg",.short_name='p',
                 .flags=GOPT_ARGUMENT_REQUIRED},
    [OPT_SWITCH] = {.long_name="switch",.short_name='s',
                 .flags=GOPT_ARGUMENT_REQUIRED},
    [FLAG_XDG_INSTALL] = {.long_name="xdg", .short_name='x',
                          .flags=GOPT_ARGUMENT_FORBIDDEN},
    [FLAG_JSOO] = {.long_name="jsoo", .short_name='j',
                   .flags=GOPT_ARGUMENT_FORBIDDEN},
    [FLAG_CLEAN] = {.long_name="clean",.short_name='c',
                    .flags=GOPT_ARGUMENT_FORBIDDEN},
    [FLAG_SHOW_CONFIG] = {.long_name="show-config",
                          .flags=GOPT_ARGUMENT_FORBIDDEN},

#if defined(DEBUG_fastbuild)
    [FLAG_DEBUG] = {.long_name="debug",.short_name='d',
                    .flags=GOPT_ARGUMENT_FORBIDDEN | GOPT_REPEATABLE},
    [FLAG_TRACE] = {.long_name="trace",.short_name='t',
                    .flags=GOPT_ARGUMENT_FORBIDDEN},

    [OPT_DEBUG_FINDLIBC] = {.long_name="debug-findlibc",
                           .flags=GOPT_ARGUMENT_REQUIRED},
    [FLAG_TRACE_FINDLIBC] = {.long_name="trace-findlibc",
                             .flags=GOPT_ARGUMENT_FORBIDDEN},

    [OPT_DEBUG_OPAMC] = {.long_name="debug-opamc",
                           .flags=GOPT_ARGUMENT_REQUIRED},
    [FLAG_TRACE_OPAMC] = {.long_name="trace",
                          .flags=GOPT_ARGUMENT_FORBIDDEN},
#endif

    [FLAG_VERBOSE] = {.long_name="verbose",.short_name='v',
                      .flags=GOPT_ARGUMENT_FORBIDDEN | GOPT_REPEATABLE},
    [FLAG_QUIET] = {.long_name="quiet",.short_name='q',
                    .flags=GOPT_ARGUMENT_FORBIDDEN},
    [FLAG_HELP] = {.long_name="help",.short_name='h',
                   .flags=GOPT_ARGUMENT_FORBIDDEN},
    [LAST] = {.flags = GOPT_LAST}
};

void _set_options(struct option options[])
{
    if (options[FLAG_HELP].count) {
        _print_usage();
        exit(EXIT_SUCCESS);
    }

    if (options[FLAG_VERBOSE].count) {
        /* printf("verbose ct: %d\n", options[FLAG_VERBOSE].count); */
        verbose = true;
        verbosity = options[FLAG_VERBOSE].count;
    }

#if defined(DEBUG_fastbuild)
    if (options[FLAG_DEBUG].count) {
        coswitch_debug = options[FLAG_DEBUG].count;
    }
    if (options[FLAG_TRACE].count) {
        coswitch_trace = true;
    }

    if (options[OPT_DEBUG_FINDLIBC].count) {
        errno = 0;
        long tmp = strtol(options[OPT_DEBUG_FINDLIBC].argument,
                          NULL, 10);
        if (errno) {
            /* fprintf(stderr, "--findlib-debug must be an int."); */
            log_error( "--findlib-debug must be an int.");
            exit(EXIT_FAILURE);
        } else {
            findlibc_debug = (int)tmp;
        }
    }
    if (options[FLAG_TRACE_FINDLIBC].count) {
        findlibc_trace = true;
    }

    if (options[OPT_DEBUG_OPAMC].count) {
        errno = 0;
        long tmp = strtol(options[OPT_DEBUG_OPAMC].argument,
                          NULL, 10);
        if (errno) {
            /* fprintf(stderr, "--findlib-debug must be an int."); */
            log_error( "--debug-opamc must be an int.");
            exit(EXIT_FAILURE);
        } else {
            opamc_debug = (int)tmp;
        }
    }
    if (options[FLAG_TRACE_OPAMC].count) {
        opamc_trace = true;
    }
#endif
    if (options[FLAG_JSOO].count) {
        enable_jsoo = true;
    }

    if (options[FLAG_XDG_INSTALL].count) {
        xdg_install = true;
    }

    /* if (options[OPT_PKG].count) { */
    /*     utarray_push_back(opam_include_pkgs, &optarg); */
    /* } */

    /* case 'x': */
    /*     printf("EXCL %s\n", optarg); */
    /*     utarray_push_back(opam_exclude_pkgs, &optarg); */
    /*     break; */
}

/* **************** **************** */
void pkg_handler(char *switch_pfx,
                 char *site_lib, /* switch_lib */
                 char *pkg_dir,  /* subdir of site_lib */
                 void *_paths) /* struct paths_s* */
        /* .registry = registry, */
        /* .coswitch_lib = coswitch_lib */
{
    bool empty_pkg = false;
    struct paths_s *paths = (struct paths_s*)_paths;
    UT_string *registry = (UT_string*)paths->registry;
    UT_string *coswitch_lib = (UT_string*)paths->coswitch_lib;
    struct obzl_meta_package *pkgs
        = (struct obzl_meta_package*)paths->pkgs;

    if (verbosity > 1) {
        log_debug("pkg_handler: %s", pkg_dir);
        log_debug("site-lib: %s", site_lib);
        log_debug("registry: %s", utstring_body(registry));
        log_debug("coswitch: %s", utstring_body(coswitch_lib));
        log_debug("pkgs ct: %d", HASH_COUNT(pkgs));
    }

    utstring_renew(meta_path);
    utstring_printf(meta_path, "%s/%s/META",
                    site_lib,
                    pkg_dir);
                    /* utstring_body(opam_switch_lib), pkg_name); */
    if (verbosity > 1)
        log_info("meta_path: %s", utstring_body(meta_path));

    errno = 0;
    if ( access(utstring_body(meta_path), F_OK) != 0 ) {
        // no META happens for e.g. <switch>/lib/stublibs
        /* log_warn("%s: %s", */
        /*          strerror(errno), utstring_body(meta_path)); */
        return;
    /* } else { */
    /*     /\* exists *\/ */
    /*     log_info("accessible: %s", utstring_body(meta_path)); */
    }
    empty_pkg = false;
    errno = 0;
    // pkg must be freed...
    struct obzl_meta_package *pkg
        = obzl_meta_parse_file(utstring_body(meta_path));

    if (pkg == NULL) {
        if ((errno == -1)
                || (errno == -2)) {
            empty_pkg = true;
/* #if defined(TRACING) */
            if (verbosity > 2)
                log_warn("Empty META file: %s", utstring_body(meta_path));
/* #endif */
            /* check dune-package for installed executables */
            /* chdir(old_cwd); */

    pkg = (struct obzl_meta_package*)calloc(sizeof(struct obzl_meta_package), 1);
 char *fname = strdup(utstring_body(meta_path));
    pkg->name      = package_name_from_file_name(fname);
    /* pkg->name      = pkg_dir; */
    char *x = strdup(pkg->name);
    char *p;
    // module names may not contain uppercase
    for (p = x; *p; ++p) *p = tolower(*p);
    pkg->module_name = strdup(x);
    free(x);
    x = strdup(fname);
    pkg->path      = strdup(dirname(x));
    free(x);
    pkg->directory = pkg->name; // dirname(fname);
    pkg->metafile  = utstring_body(meta_path);
    pkg->entries = NULL;
            /* return; */
        /* } */
        /* else if (errno == -2) { */
        /*     log_warn("META file contains only whitespace: %s", utstring_body(meta_path)); */
        /*     return; */
        } else {
            log_error("Error parsing %s", utstring_body(meta_path));
            return;
        }
        /* emitted_bootstrapper = false; */
    } else {
        /* #if defined(DEVBUILD) */
        if (verbosity > 2) {
            log_info("Parsed %d %s", verbosity, utstring_body(meta_path));
        }
        /* if (coswitch_debug_findlib) */
        /* DUMP_PKG(0, pkg); */
        /* #endif */
        // write module stuff in site-lib
        // write registry stuff
    }

    /* struct obzl_meta_packages_s *pkg */
    /*     = malloc(sizeof(obzl_meta_packages_s)); */
    /* char *n =  "TEST"; */
    /* pkg->name = n; */

    /* if (HASH_COUNT(pkgs) == 0) { */
    /*     log_debug("adding initial pkg: %s", pkg->name); */
    /*     pkgs = pkg; */
    /* } else { */
    /*     log_debug("adding next pkg", pkg->name); */
    /*     HASH_ADD_PTR(pkgs, name, pkg); */
    /* } */

    /* log_debug("pkg->name: %s", pkg->name); */
    /* log_debug("pkg->module_name: %s", pkg->module_name); */

    char *pkg_name = strdup(pkg->name);
    LOG_DEBUG(0, "pkg_name: %s", pkg_name);

    /* char *p = pkg_name; */
    /* for (p = pkg_name; *p; ++p) *p = tolower(*p); */

    /* log_debug("adding next pkg: %s, (%p)", pkg->name, pkg); */
    /* log_debug("hashing keyptr: %s", pkg->name); */
    /* log_debug("  path: %s (%p)", pkg->path, pkg->path); */

    HASH_ADD_KEYPTR(hh, paths->pkgs,
                    pkg_name, strlen(pkg_name),
                    pkg);

    /* log_debug("HASH CT: %d", HASH_COUNT(paths->pkgs)); */
    /* struct obzl_meta_package *p; */
    /* HASH_FIND_STR(paths->pkgs, pkg->name, p); */
    /* if (p) */
    /*     log_debug("found: %s", p->name); */
    /* else */
    /*     log_debug("NOT found: %s", pkg->name); */

    /* log_debug("iterating"); */
    /* struct obzl_meta_package *s; */
    /* for (s = paths->pkgs; s != NULL; s = s->hh.next) { */
    /*     log_debug("\tpkg ptr %p", s); */
    /*     log_debug("\tpkg name %s", s->name); */
    /*     log_debug("\tpkg path %s (%p)", s->path, s->path); */
    /* } */


    /* log_debug("coswitch_lib: %s", utstring_body(coswitch_lib)); */

    /** emit workspace, module files for opam pkg **/
    // WORKSPACE.bazel
    UT_string *ws_root;
    utstring_new(ws_root);
    utstring_printf(ws_root, "%s/%s",
                    utstring_body(coswitch_lib),
                    pkg_name);
    mkdir_r(utstring_body(ws_root));

    UT_string *bazel_file;
    utstring_new(bazel_file);
    utstring_printf(bazel_file, "%s/WORKSPACE.bazel",
                    utstring_body(ws_root));
    emit_workspace_file(bazel_file, pkg_name);

    // MODULE.bazel emitted later, after all pkgs parsed

    if (!empty_pkg) {

    // then emit the BUILD.bazel files for the opam pkg
    utstring_new(imports_path);
    utstring_printf(imports_path, "%s", pkg_name);
                    /* obzl_meta_package_name(pkg)); */

    utstring_new(pkg_parent);

    // for now:
    UT_string *switch_lib;
    utstring_new(switch_lib);
    utstring_printf(switch_lib, "%s", site_lib);

    emit_build_bazel(switch_lib, // site_lib,
                     utstring_body(coswitch_lib),
                     /* utstring_body(ws_root), */
                     0,         /* indent level */
                     pkg_name, // pkg_root
                     pkg_parent, /* needed for handling subpkgs */
                     NULL, // "buildfiles",        /* _pkg_prefix */
                     utstring_body(imports_path),
                     /* "",      /\* pkg-path *\/ */
                     pkg);
                     /* opam_pending_deps, */
                     /* opam_completed_deps); */
    }
    // this will emit one BUILD.bazel file per pkg & subpkg
    // and put them in <switch>/lib/<repo>/lib/<subpkg> dirs
    // e.g. <switch>/lib/ppxlib/lib/ast for ppxlib.ast

    /* ******************************** */
    // finally, the registry record
    // emit registry files
    /* emit_registry_record(registry, */
    /*                      meta_path, */
    /*                      pkg_dir, */
    /*                      pkg, */
    /*                      default_version // (char*)version */
    /*                      ); */

    emit_pkg_bindir(switch_pfx, utstring_body(coswitch_lib),
                    pkg->name);
}

/**********************************
   Method:
   1. Iterate over switch, converting META to BUILD.bazel
      and collecting toplevel packages in pkgs utarray
   2. Iterate over pkgs array, emitting
      - MODULE.bazel to coswitch
      - registry record

    runtime cwd:
      in-bazel: BUILD_WORKSPACE_DIRECTORY
        (so we can detect local opam switch)
      in-opam: launch dir (not relevant)
 */

/*
  File system:
  switch:
      in-bazel: default to current switch, accept -s
      in-opam: always current switch, reject -s

  coswitch:
      in-bazel: xdg or local
      in-opam:
        root:     <switch-pfx>/share/obazl
        lib:      <switch-pfx>/share/obazl/lib
        bin?
        registry: <switch-pfx>/share/obazl/modules
        templates: <switch-pfx>/share/obazl/templates
 */

extern char **environ;

int main(int argc, char *argv[])
{
    argc = gopt(argv, options);
    (void)argc;

    gopt_errors (argv[0], options);

    _set_options(options);

    /* dump env vars: */
    /* log_debug("ENV"); */
    /* int i = 0; */
    /* while(environ[i]) { */
    /*     log_info("%s", environ[i++]); */
    /* } */

    char *bws_dir = getenv("BUILD_WORKSPACE_DIRECTORY");
    if (bws_dir) {
        bazel_env = true;
    } else {
        bazel_env = false;
    }

    /* char *launch_dir = getcwd(NULL,0); */
    if (bazel_env) {
        if (options[OPT_SWITCH].count) {
            switch_name = options[OPT_SWITCH].argument;
        } else {
            switch_name = opam_switch_name();
        }
    } else{
        if (options[OPT_SWITCH].count) {
            log_warn("Ignoring -s %s",
                     options[OPT_SWITCH].argument);
        }
        // chdir to switch root?
        switch_name = opam_switch_name();
    }
    if (options[FLAG_QUIET].count) {
        quiet = true;
    }

    /* utstring_new(coswitch_runfiles_root); */
    /* utstring_printf(coswitch_runfiles_root, */
    /*                 "%s", */
    /*                 getcwd(NULL, 0)); */

    /* s7_scheme *s7 = coswitch_s7_init(); */

    /* for reading dune-project files */
    s7 = libs7_init();
    libs7_load_plugin(s7, "dune");

    /* coswitch_s7_init2(NULL, // options[OPT_MAIN].argument, */
    /*              NULL); // options[OPT_WS].argument); */

    /* coswitch_configure(); */
    /* config_mibl.c reads miblrc, may call s7 */
    /* utstring_new(setter); */

    UT_array *opam_include_pkgs;
    utarray_new(opam_include_pkgs,&ut_str_icd);

    UT_array *opam_exclude_pkgs;
    utarray_new(opam_exclude_pkgs,&ut_str_icd);

    // globals in config_bazel.c:
    // bzl_mode
    // coswitch_runfiles_root, runtime_data_dir
    // obazl_ini_path
    // rootws, ews_root
    // traversal_root
    // build_ws_dir, build_wd, launch_dir
    // may call config_xdg_dirs()
    /* bazel_configure(NULL);      /\* run by coswitch_s7_init??? *\/ */

    /* if (options[FLAG_SHOW_CONFIG].count) { */
    /*     show_bazel_config(); */
    /*     show_coswitch_config(); */
    /*     show_s7_config(); */
    /*     exit(0); */
    /* } */

    char *compiler_version = opam_switch_base_compiler_version(switch_name);

    char *switch_pfx = opam_switch_prefix(switch_name);

    UT_string *runfiles_root;
    utstring_new(runfiles_root);
    if (bazel_env) {
        if (strlen(BAZEL_CURRENT_REPOSITORY) == 0) {
            utstring_printf(runfiles_root, "%s",
                            realpath("new", NULL));
        } else {
            char *rp = realpath("external/"
                                BAZEL_CURRENT_REPOSITORY,
                                NULL);
            /* log_debug("PWD: %s", getcwd(NULL,0)); */
            utstring_printf(runfiles_root, "%s/new", rp);
        }
    } else {
        // running outside of bazel
        utstring_printf(runfiles_root,
                        "%s/obazl",
                        xdg_data_home());
    }
    /* log_debug("RUNFILES_ROOT: %s", utstring_body(runfiles_root)); */
    chdir(bws_dir);

    /* if (!bazel_env) { */
    /*     // verify that we're running in opam context */
    /*     // cwd should be subdir of <switch-prefix>? */
    /*     char *cwd = getcwd(NULL, 0); */
    /*     /\* log_debug("CWD: %s", cwd); *\/ */
    /*     size_t length = cwk_path_get_intersection(cwd, */
    /*                                               switch_pfx); */
    /*     /\* printf("The common portion is: '%.*s'\n", *\/ */
    /*     /\*        (int)length, switch_pfx); *\/ */
    /*     if (length != strlen(switch_pfx)) { */
    /*         fprintf(stderr, RED "ERROR: " CRESET */
    /*                 "Must be launched by opam; not designed to be launched by other methods\n"); */
    /*         exit(EXIT_FAILURE); */
    /*     } */
    /* } */

    char *switch_lib = opam_switch_lib(switch_name);

    /* chdir(launch_dir); */

    UT_string *coswitch_root;
    utstring_new(coswitch_root);
    if (bazel_env) {
        //in-bazel:
        //  shared: ~/.local/share/obazl
        //  local:  .config/obazl
        if (options[OPT_SWITCH].count) {
            // --switch arg overrides local switch
            coswitch_name = switch_name;
            if (xdg_install) {
                utstring_printf(coswitch_root,
                                "%s/obazl",
                                xdg_data_home());
            } else {
                utstring_printf(coswitch_root,
                                "%s/share/obazl",
                                switch_pfx);
            }
        } else {
            // no --switch arg
            // cwd is build ws, does ./_opam exist?
            utstring_printf(coswitch_root,
                            "%s/_opam",
                            bws_dir);
            /* log_debug("CHECK FOR LOCAL: %s", utstring_body(coswitch_root)); */
            int rc = access(utstring_body(coswitch_root), F_OK);
            if (rc == 0) {
                // found ./_opam - local switch
                /* log_debug("FOUND LOCAL"); */
                coswitch_name = "";
                utstring_renew(coswitch_root);
                utstring_printf(coswitch_root,
                                "%s/.config/obazl",
                                bws_dir);
            } else {
                /* log_debug("NOTFOUND LOCAL"); */
                coswitch_name = switch_name;
                utstring_renew(coswitch_root);
                if (xdg_install) {
                    utstring_printf(coswitch_root,
                                    "%s/obazl",
                                    xdg_data_home());
                } else {
                    utstring_printf(coswitch_root,
                                    "%s/share/obazl",
                                    switch_pfx);
                }
            }
        }
    } else {
        // not running under bazel
        //in-opam: <switch-pfx>/share/obazl
        utstring_renew(coswitch_root);
        if (xdg_install) {
            coswitch_name = switch_name;
            utstring_printf(coswitch_root,
                            "%s/obazl",
                            xdg_data_home());
        } else {
            coswitch_name = "";
            utstring_printf(coswitch_root,
                            "%s/share/obazl",
                            switch_pfx);
        }
    }
    /* log_debug("coswitch_root: %s", */
    /*           utstring_body(coswitch_root)); */

    UT_string *coswitch_pfx;
    utstring_new(coswitch_pfx);
    if (bazel_env) {
        //in-bazel:
        //  shared: ~/.local/share/obazl/opam/<switch-name>
        //  local:  .config/obazl/opam/lib
        if (xdg_install) {
            utstring_printf(coswitch_pfx,
                            "%s/opam/%s",
                            utstring_body(coswitch_root),
                            coswitch_name);
        } else {
            utstring_printf(coswitch_pfx,
                            "%s",
                            utstring_body(coswitch_root));
        }
    } else {
        // running outside of bazel
        if (xdg_install) {
            utstring_printf(coswitch_pfx,
                            "%s/opam/%s",
                            utstring_body(coswitch_root),
                            coswitch_name);
        } else {
            utstring_printf(coswitch_pfx,
                            "%s",
                            utstring_body(coswitch_root));
        }
    }

    UT_string *coswitch_lib;
    utstring_new(coswitch_lib);
    if (bazel_env) {
        //in-bazel:
        //  shared: ~/.local/share/obazl/opam/<switch-name>/lib
        //  local:  .config/obazl/opam/lib
        if (xdg_install) {
            utstring_printf(coswitch_lib,
                            "%s/opam/%s/lib",
                            utstring_body(coswitch_root),
                            coswitch_name);
        } else {
            //in-opam: <switch-pfx>/share/obazl/lib
            utstring_printf(coswitch_lib,
                            "%s/lib",
                            utstring_body(coswitch_root));
        }
    } else {
        if (xdg_install) {
            utstring_printf(coswitch_lib,
                            "%s/opam/%s/lib",
                            utstring_body(coswitch_root),
                            coswitch_name);
        } else {
            //in-opam: <switch-pfx>/share/obazl/lib
            utstring_printf(coswitch_lib,
                            "%s/lib",
                            utstring_body(coswitch_root));
        }
    }

    /* registry:
       in-bazel:
         shared: .local/share/obazl/registry
         local:  .config/obazl/registry
       out-of-bazel: .opam or xdg <switch-pfx>/share/obazl
    */
    UT_string *registry = config_bzlmod_registry(coswitch_name,
                                                 coswitch_root,
                                                 bazel_env);

    utstring_new(meta_path);

    if (verbosity > 0) {
        log_info("opam root: %s", opam_root());
        log_info("switch name:   %s", switch_name);
        log_info("switch prefix: %s", switch_pfx);
        log_info("switch lib:    %s", switch_lib);

        log_info("coswitch root: %s", utstring_body(coswitch_root));
        log_info("coswitch name: %s", coswitch_name);
        log_info("coswitch lib: %s", utstring_body(coswitch_lib));
        log_info("registry: %s", utstring_body(registry));
    }

    /* struct obzl_meta_package *pkgs = NULL; */
    struct paths_s paths = {
        .registry = registry,
        .coswitch_lib = coswitch_lib
        /* .pkgs = pkgs */
    };

    //FIXME: add extras arg to pass extra info to pkg_handler
    findlib_map(opam_include_pkgs,
                opam_exclude_pkgs,
                switch_pfx,
                switch_lib,
                pkg_handler,
                (void*)&paths);
    /* log_debug("FINAL HASH CT: %d", HASH_COUNT(paths.pkgs)); */

    // now the registry records
    struct obzl_meta_package *pkg;
    char *pkg_name; // , *p;
    for (pkg = paths.pkgs; pkg != NULL; pkg = pkg->hh.next) {
        pkg_name = strdup(pkg->name);
        /* log_debug("regging PKG: %s", pkg_name); */
        /* for (p = pkg_name; *p; ++p) *p = tolower(*p); */
        /* if (verbosity > log_writes) */
        /*     log_debug(BLU " PKG %s" CRESET, pkg_name); */
        semver_t *version = findlib_pkg_version(pkg);
        if (verbosity > 2) {
            log_debug("    version %d.%d.%d",
                      version->major, version->minor,
                      version->patch);
            log_debug("    compat: %d", version->major);
            log_debug("    path %s", pkg->path);
            log_debug("    path ptr %p", pkg->path);
            log_debug("    dir %s", pkg->directory);
            log_debug("    meta %s", pkg->metafile);
            log_debug("    entries ptr %p", pkg->entries);
        }
        /* ******************************** */
        emit_registry_record(registry, compiler_version,
                             pkg, paths.pkgs);
                             /* s->metafile, */
                             /* s->directory, */
                             /* s, */
                             /* default_version // (char*)version */
                             /* ); */

        UT_string *mfile;
        utstring_new(mfile);
        utstring_printf(mfile, "%s/%s",
                        utstring_body(coswitch_lib),
                        /* version->major, version->minor, */
                        /* version->patch, */
                        pkg_name);
        free(version);
        mkdir_r(utstring_body(mfile));
        utstring_printf(mfile, "/MODULE.bazel");
                        /* utstring_body(mfile)); */
        emit_module_file(mfile, compiler_version, pkg, paths.pkgs);
        utstring_free(mfile);
        free(pkg_name);
    }
    /* FIXME: free opam_include_pkgs, opam_exclude_pkgs */
    /* log_warn("EMITTING OCAML WS"); */
    emit_ocaml_workspace(registry,
                         compiler_version,
                         paths.pkgs,
                         switch_name,
                         switch_pfx,
                         utstring_body(coswitch_lib),
                         utstring_body(runfiles_root));
    utstring_free(meta_path);

    if (bazel_env) {
        write_registry_directive(utstring_body(registry));
    }

    if (!quiet) {
        fprintf(stdout,
                GRN "Switch name:      " CRESET
                "%s\n", switch_name);
        fprintf(stdout,
                GRN "Compiler version: " CRESET
                "%s\n", compiler_version);
        fprintf(stdout,
                GRN "Switch prefix:    " CRESET
                "%s\n", switch_pfx);
        /* fprintf(stdout, */
        /*         GRN "Coswitch root:    " CRESET */
        /*         "%s\n", utstring_body(coswitch_root)); */
        fprintf(stdout,
                GRN "Coswitch:         " CRESET
                "%s\n", utstring_body(coswitch_pfx));
        fprintf(stdout,
                GRN "Registry:         " CRESET
                "%s\n", utstring_body(registry));
    }

    free(switch_lib);
    free(coswitch_root);
    free(coswitch_pfx);
    free(coswitch_lib);

#if defined(TRACING)
    log_debug("exiting new:coswitch");
#endif
    /* dump_nodes(result); */
}
