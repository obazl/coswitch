# generated file - DO NOT EDIT

exports_files(["BUILD.bazel"])

load("@rules_ocaml//toolchain:BUILD.bzl", "ocaml_toolchain_adapter")

########################
ocaml_toolchain_adapter(
    name                   = "ocamlopt.opt",
    host                   = "sys",
    target                 = "sys",
    repl                   = "@ocaml//bin:ocaml",
    compiler               = "@ocaml//bin:ocamlopt.opt",
    version              = "@ocaml//version",
    profiling_compiler   = "@ocaml//bin:ocamloptp",
    ocamllex             = "@ocaml//bin:ocamllex.opt",
    ocamlyacc            = "@ocaml//bin:ocamlyacc",
    linkmode             = "dynamic",
    # stdlib             = "@ocaml//runtime:stdlib.cma",
    # std_exit           = "@ocaml//runtime:std_exit.cmo",

    default_runtime      = "@ocaml//runtime:libasmrun.a",
    std_runtime          = "@ocaml//runtime:libasmrun.a",
    dbg_runtime          = "@ocaml//runtime:libasmrund.a",
    instrumented_runtime = "@ocaml//runtime:libasmruni.a",
    pic_runtime          = "@ocaml//runtime:libasmrun_pic.a",
    shared_runtime       = "@ocaml//runtime:libasmrun_shared.so",

    ## omit vm stuff for >sys toolchains?
    # vmruntime            = "@ocaml//bin:ocamlrun",
    # vmruntime_debug        = "@ocaml//bin:ocamlrund",
    # vmruntime_instrumented = "@ocaml//bin:ocamlruni",
    # vmlibs                 = "@stublibs//lib/stublibs",
)

ocaml_toolchain_adapter(
    ## debug syssys tc
    name                   = "ocamlopt.opt.d",
    host                   = "sys",
    target                 = "sys",
    repl                   = "@ocaml//bin:ocaml",
    compiler               = "@ocaml//bin:ocamlopt.opt",
    version                = "@ocaml//version",
    profiling_compiler     = "@ocaml//bin:ocamloptp",
    ocamllex               = "@ocaml//bin:ocamllex.opt",
    ocamlyacc              = "@ocaml//bin:ocamlyacc",
    linkmode               = "dynamic",

    default_runtime      = "@ocaml//runtime:libasmrund.a",
    std_runtime          = "@ocaml//runtime:libasmrun.a",
    dbg_runtime          = "@ocaml//runtime:libasmrund.a",
    instrumented_runtime = "@ocaml//runtime:libasmruni.a",
    pic_runtime          = "@ocaml//runtime:libasmrun_pic.a",
    shared_runtime       = "@ocaml//runtime:libasmrun_shared.so",

    ## omit vm stuff for >sys toolchains?
    # vmruntime              = "@ocaml//bin:ocamlrun",
    # vmruntime_debug        = "@ocaml//bin:ocamlrund",
    # vmruntime_instrumented = "@ocaml//bin:ocamlruni",
    # vmlibs                 = "@stublibs//lib/stublibs",
)

########################
ocaml_toolchain_adapter(
    name                   = "ocamlc.opt",
    host                   = "sys",
    target                 = "vm",
    repl                   = "@ocaml//bin:ocaml",
    compiler               = "@ocaml//bin:ocamlc.opt",
    version                = "@ocaml//version",
    profiling_compiler     = "@ocaml//bin:ocamlcp",
    ocamllex               = "@ocaml//bin:ocamllex.opt",
    ocamlyacc              = "@ocaml//bin:ocamlyacc",
    linkmode               = "dynamic",

    default_runtime      = "@ocaml//runtime:libcamlrun.a",
    std_runtime          = "@ocaml//runtime:libcamlrun.a",
    dbg_runtime          = "@ocaml//runtime:libcamlrund.a",
    instrumented_runtime = "@ocaml//runtime:libcamlruni.a",
    pic_runtime          = "@ocaml//runtime:libcamlrun_pic.a",
    shared_runtime       = "@ocaml//runtime:libcamlrun_shared.so",

    # vmruntime              = "@ocaml//bin:ocamlrun",
    # vmruntime_debug        = "@ocaml//bin:ocamlrund",
    # vmruntime_instrumented = "@ocaml//bin:ocamlruni",
    # vmlibs                 = "@stublibs//lib/stublibs",
)

########################
ocaml_toolchain_adapter(
    name                   = "ocamlc.byte",
    host                   = "vm",
    target                 = "vm",
    repl                   = "@ocaml//bin:ocaml",
    compiler               = "@ocaml//bin:ocamlc.byte",
    version                = "@ocaml//version",
    profiling_compiler     = "@ocaml//bin:ocamlcp",
    ocamllex               = "@ocaml//bin:ocamllex.byte",
    ocamlyacc              = "@ocaml//bin:ocamlyacc",
    linkmode               = "dynamic",

    default_runtime      = "@ocaml//runtime:libcamlrun.a",
    std_runtime          = "@ocaml//runtime:libcamlrun.a",
    dbg_runtime          = "@ocaml//runtime:libcamlrund.a",
    instrumented_runtime = "@ocaml//runtime:libcamlruni.a",
    pic_runtime          = "@ocaml//runtime:libcamlrun_pic.a",
    shared_runtime       = "@ocaml//runtime:libcamlrun_shared.so",

    # vmruntime              = "@ocaml//bin:ocamlrun",
    # vmruntime_debug        = "@ocaml//bin:ocamlrund",
    # vmruntime_instrumented = "@ocaml//bin:ocamlruni",
    # vmlibs                 = "@stublibs//lib/stublibs",
)

########################
ocaml_toolchain_adapter(
    name                   = "ocamlopt.byte",
    host                   = "vm",
    target                 = "sys",
    repl                   = "@ocaml//bin:ocaml",
    compiler               = "@ocaml//bin:ocamlopt.byte",
    version                = "@ocaml//version",
    profiling_compiler     = "@ocaml//bin:ocamloptp",
    ocamllex               = "@ocaml//bin:ocamllex.byte",
    ocamlyacc              = "@ocaml//bin:ocamlyacc",
    linkmode               = "dynamic",

    default_runtime      = "@ocaml//runtime:libasmrun.a",
    std_runtime          = "@ocaml//runtime:libasmrun.a",
    dbg_runtime          = "@ocaml//runtime:libasmrund.a",
    instrumented_runtime = "@ocaml//runtime:libasmruni.a",
    pic_runtime          = "@ocaml//runtime:libasmrun_pic.a",
    shared_runtime       = "@ocaml//runtime:libasmrun_shared.so",

    # vmruntime              = "@ocaml//bin:ocamlrun",
    # vmruntime_debug        = "@ocaml//bin:ocamlrund",
    # vmruntime_instrumented = "@ocaml//bin:ocamlruni",
    # vmlibs                 = "@stublibs//lib/stublibs",
)

ocaml_toolchain_adapter(
    name                   = "ocamlopt.byte.d",
    host                   = "vm",
    target                 = "sys",
    repl                   = "@ocaml//bin:ocaml",
    compiler               = "@ocaml//bin:ocamlopt.byte",
    version                = "@ocaml//version",
    profiling_compiler     = "@ocaml//bin:ocamloptp",
    ocamllex               = "@ocaml//bin:ocamllex.byte",
    ocamlyacc              = "@ocaml//bin:ocamlyacc",
    linkmode               = "dynamic",

    default_runtime      = "@ocaml//runtime:libasmrund.a",
    std_runtime          = "@ocaml//runtime:libasmrun.a",
    dbg_runtime          = "@ocaml//runtime:libasmrund.a",
    instrumented_runtime = "@ocaml//runtime:libasmruni.a",
    pic_runtime          = "@ocaml//runtime:libasmrun_pic.a",
    shared_runtime       = "@ocaml//runtime:libasmrun_shared.so",

    # vmruntime              = "@ocaml//bin:ocamlrun",
    # vmruntime_debug        = "@ocaml//bin:ocamlrund",
    # vmruntime_instrumented = "@ocaml//bin:ocamlruni",
    # vmlibs                 = "@stublibs//lib/stublibs",
)
