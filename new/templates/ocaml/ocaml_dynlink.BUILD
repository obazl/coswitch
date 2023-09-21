# generated file - DO NOT EDIT

load("@rules_ocaml//build:rules.bzl", "ocaml_import")

ocaml_import(
    name       = "dynlink",
    version    = "[distributed with OCaml]",
    sigs       = glob(["*.cmi"]),
    archive    =  select({
        "@ocaml//platform/emitter:vm" : "dynlink.cma",
        "@ocaml//platform/emitter:sys": "dynlink.cmxa",
    }, no_match_error="Bad platform"),
    afiles   = select({
        "@ocaml//platform/emitter:vm" : [],
        "@ocaml//platform/emitter:sys": glob(["*.a"],
                                         exclude=["*_stubs.a"])
    }, no_match_error="Bad platform"),
    astructs = select({
        "@ocaml//platform/emitter:vm" : [],
        "@ocaml//platform/emitter:sys": glob(["*.cmx"])
    }, no_match_error="Bad platform"),
    ofiles   = select({
        "@ocaml//platform/emitter:vm" : [],
        "@ocaml//platform/emitter:sys": glob(["*.o"])
    }, no_match_error="Bad platform"),
    cmts       = glob(["*.cmt"]),
    cmtis      = glob(["*.cmti"]),
    srcs       = glob(["*.ml", "*.mli"]),
    all        = glob(["dyn*.*"]),
    visibility = ["//visibility:public"],
);

