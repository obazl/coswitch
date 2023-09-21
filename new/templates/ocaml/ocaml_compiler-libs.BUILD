# generated file - DO NOT EDIT

load("@rules_ocaml//build:rules.bzl", "ocaml_import")

ocaml_import(
    name       = "common",
    doc        = """Common compiler routines""",
    sigs       = glob(["*.cmi"]),
    archive    =  select({
        "@ocaml//platform/emitter:vm" : "ocamlcommon.cma",
        "@ocaml//platform/emitter:sys": "ocamlcommon.cmxa",
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
    all        = glob(["*"]),
    visibility = ["//visibility:public"]
)

ocaml_import(
    name       = "bytecomp",
    doc        = """Common compiler routines""",
    sigs       = glob(["*.cmi"]),
    archive    =  select({
        "@ocaml//platform/emitter:vm" : "ocamlbytecomp.cma",
        "@ocaml//platform/emitter:sys": "ocamlbytecomp.cmxa",
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
    cmts        = glob(["*.cmt"]),
    cmtis       = glob(["*.cmti"]),
    srcs       = glob(["*.ml", "*.mli"]),
    all        = glob(["*"]),
    visibility = ["//visibility:public"]
)

ocaml_import(
    name       = "optcomp",
    doc        = """optcomp compiler routines""",
    sigs       = glob(["*.cmi"]),
    archive    =  select({
        "@ocaml//platform/emitter:vm" : "ocamloptcomp.cma",
        "@ocaml//platform/emitter:sys": "ocamloptcomp.cmxa",
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
    cmts        = glob(["*.cmt"]),
    cmtis       = glob(["*.cmti"]),
    srcs       = glob(["*.ml", "*.mli"]),
    all        = glob(["*"]),
    visibility = ["//visibility:public"]
)

ocaml_import(
    name = "toplevel",
    doc = """Toplevel interactions""",
    sigs       = glob(["*.cmi"]),
    archive    =  select({
        "@ocaml//platform/emitter:vm" : "ocamltoplevel.cma",
        "@ocaml//platform/emitter:sys": "ocamltoplevel.cmxa",
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
    cmts        = glob(["*.cmt"]),
    cmtis       = glob(["*.cmti"]),
    srcs = glob(["*.ml", "*.mli"]),
    all = glob(["*.cmx", "*.cmi"]),
    deps = [":bytecomp"],
    visibility = ["//visibility:public"]
)

# exports_files([
#     "ocamlcommon.cma",
#     "ocamlcommon.cmxa",
#     "ocamlcommon.a",

#     "ocamlbytecomp.cma",
#     "ocamlbytecomp.cmxa",
#     "ocamlbytecomp.a",

#     "ocamloptcomp.cma",
#     "ocamloptcomp.cmxa",
#     "ocamloptcomp.a",

#     "ocamlmiddleend.cma",
#     "ocamlmiddleend.cmxa",
#     "ocamlmiddleend.a",

#     "ocamltoplevel.cma",
#     "ocamltoplevel.cmxa",
#     "ocamltoplevel.a",
# ])

# filegroup(
#     name = "all",
#     srcs = glob([
#         "*.cmx", "*.o", "*.cmo",
#         "*.cmt", "*.cmti",
#         "*.mli", "*.cmi",
#     ]),
#     visibility = ["@ocaml//compiler-libs:__subpackages__"]
# )
