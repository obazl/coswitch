# generated file - DO NOT EDIT

load("@rules_ocaml//build:rules.bzl", "ocaml_import")

ocaml_import(
    name       = "str",
    version    = """[distributed with Ocaml]""",
    doc        = """Regular expressions and string processing""",
    sigs       = glob(["*.cmi"]),
    archive    =  select({
        "@ocaml//platform/emitter:vm" : "str.cma",
        "@ocaml//platform/emitter:sys": "str.cmxa",
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
    all        = glob(["str.*"]),
    visibility = ["//visibility:public"]
)

ocaml_import(
    name       = "plugin",
    plugin     =  select({
        "@ocaml//platform/emitter:vm": "str.cma",
        "//conditions:default":         "str.cmxs",
    }),
    # cmxs       = "str.cmxs",
    # cma        = "str.cma",
    visibility = ["//visibility:public"]
)
