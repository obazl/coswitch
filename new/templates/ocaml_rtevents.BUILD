# generated file - DO NOT EDIT

load("@rules_ocaml//build:rules.bzl", "ocaml_import")

ocaml_import(
    name       = "runtime_events",
    version    = """[distributed with Ocaml]""",
    doc        = """Runtime events""",
    sigs       = glob(["*.cmi"]),
    archive    =  select({
        "@ocaml//platforms/target:vm?": "runtime_events.cma",
        "//conditions:default":         "runtime_events.cmxa",
    }),
    afiles     = glob(["*.a"]),
    astructs   = glob(["*.cmx"]),
    # cma        = "runtime_events.cma",
    # cmxa       = "runtime_events.cmxa",
    # cmo        = glob(["*.cmo"]),
    # cmx        = glob(["*.cmx"]),
    ofiles     = glob(["*.o"]),
    cmts       = glob(["*.cmt"]),
    cmtis      = glob(["*.cmti"]),
    srcs       = glob(["*.ml", "*.mli"]),
    all        = glob(["runtime_events.*"]),
    visibility = ["//visibility:public"]
)

ocaml_import(
    name       = "plugin",
    plugin     =  select({
        "@ocaml//platforms/target:vm?": "runtime_events.cma",
        "//conditions:default":         "runtime_events.cmxs",
    }),
    # cmxs       = "runtime_events.cmxs",
    # cma        = "runtime_events.cma",
    visibility = ["//visibility:public"]
)
