# generated file - DO NOT EDIT

exports_files(["BUILD.bazel"])

load("@rules_ocaml//toolchain:BUILD.bzl", "ocaml_toolchain_adapter")

COMPILER_VERSION = "0.0.0"

########################
ocaml_toolchain_adapter(
    name                   = "syssys",
    host                   = "sys",
    target                 = "sys",
    repl                   = "@ocaml//bin:ocaml",
    compiler               = "@ocaml//bin:ocamlopt.opt",
    version                = COMPILER_VERSION,
    profiling_compiler     = "@ocaml//bin:ocamloptp",
    ocamllex               = "@ocaml//bin:ocamllex.opt",
    ocamlyacc              = "@ocaml//bin:ocamlyacc",
    linkmode               = "dynamic",
    # stdlib                 = "@ocaml//runtime:stdlib.cma",
    # std_exit               = "@ocaml//runtime:std_exit.cmo",

    ## cc runtime - 5 libs:
    # libasmrun.a
    # libasmrun_pic.a ## https://stackoverflow.com/questions/18026333/what-does-compiling-with-pic-dwith-pic-with-pic-actually-do
    # libasmrun_shared.so
    # libasmrund.a
    # libasmruni.a
    # runtime                = "@ocaml//runtime:runtime", # filegroup

    ## omit vm stuff for >sys toolchains?
    vmruntime              = "@ocaml//bin:ocamlrun",
    vmruntime_debug        = "@ocaml//bin:ocamlrund",
    vmruntime_instrumented = "@ocaml//bin:ocamlruni",
    vmlibs                 = "@stublibs//lib/stublibs",
)

########################
ocaml_toolchain_adapter(
    name                   = "sysvm",
    host                   = "sys",
    target                 = "vm",
    repl                   = "@ocaml//bin:ocaml",
    compiler               = "@ocaml//bin:ocamlc.opt",
    version                = COMPILER_VERSION,
    profiling_compiler     = "@ocaml//bin:ocamlcp",
    ocamllex               = "@ocaml//bin:ocamllex.opt",
    ocamlyacc              = "@ocaml//bin:ocamlyacc",
    linkmode               = "dynamic",
    vmruntime              = "@ocaml//bin:ocamlrun",
    vmruntime_debug        = "@ocaml//bin:ocamlrund",
    vmruntime_instrumented = "@ocaml//bin:ocamlruni",
    vmlibs                 = "@stublibs//lib/stublibs",
)

########################
ocaml_toolchain_adapter(
    name                   = "vmvm",
    host                   = "vm",
    target                 = "vm",
    repl                   = "@ocaml//bin:ocaml",
    compiler               = "@ocaml//bin:ocamlc.byte",
    version                = COMPILER_VERSION,
    profiling_compiler     = "@ocaml//bin:ocamlcp",
    ocamllex               = "@ocaml//bin:ocamllex.byte",
    ocamlyacc              = "@ocaml//bin:ocamlyacc",
    linkmode               = "dynamic",
    vmruntime              = "@ocaml//bin:ocamlrun",
    vmruntime_debug        = "@ocaml//bin:ocamlrund",
    vmruntime_instrumented = "@ocaml//bin:ocamlruni",
    vmlibs                 = "@stublibs//lib/stublibs",
)

########################
ocaml_toolchain_adapter(
    name                   = "vmsys",
    host                   = "vm",
    target                 = "sys",
    repl                   = "@ocaml//bin:ocaml",
    compiler               = "@ocaml//bin:ocamlopt.byte",
    version                = COMPILER_VERSION,
    profiling_compiler     = "@ocaml//bin:ocamloptp",
    ocamllex               = "@ocaml//bin:ocamllex.byte",
    ocamlyacc              = "@ocaml//bin:ocamlyacc",
    linkmode               = "dynamic",
    vmruntime              = "@ocaml//bin:ocamlrun",
    vmruntime_debug        = "@ocaml//bin:ocamlrund",
    vmruntime_instrumented = "@ocaml//bin:ocamlruni",
    vmlibs                 = "@stublibs//lib/stublibs",
)
