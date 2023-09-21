# generated file - DO NOT EDIT

load("@rules_ocaml//build:rules.bzl", "ocaml_import")

ocaml_import(
    name       = "runtime_events",
    version    = """[distributed with Ocaml]""",
    doc        = """Runtime events""",
    sigs       = glob(["*.cmi"]),
    archive    =  select({
        "@ocaml//platform/emitter:vm" : "runtime_events.cma",
        "@ocaml//platform/emitter:sys": "runtime.cmxa",
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
    all        = glob(["runtime_events.*"]),
    visibility = ["//visibility:public"]
)

ocaml_import(
    name       = "plugin",
    plugin     =  select({
        "@ocaml//platform/emitter:vm": "runtime_events.cma",
        "//conditions:default":         "runtime_events.cmxs",
    }),
    # cmxs       = "runtime_events.cmxs",
    # cma        = "runtime_events.cma",
    visibility = ["//visibility:public"]
)
