# generated file - DO NOT EDIT

load("@rules_ocaml//toolchain:BUILD.bzl", "toolchain_selector")

exports_files(glob(["*.bazel"]))

##########
toolchain_selector(
    name           = "ocamlopt.opt",
    toolchain      = "@ocaml//toolchain/adapters/local:ocamlopt.opt",
    build_host_constraints  = ["@ocaml//platform/executor:sys"],
    target_host_constraints  = ["@ocaml//platform/executor:sys"],
    visibility     = ["//visibility:public"],
)

##########
toolchain_selector(
    name                    = "ocamlc.opt",
    toolchain               = "@ocaml//toolchain/adapters/local:ocamlc.opt",
    build_host_constraints  = ["@ocaml//platform/executor:sys"],
    target_host_constraints  = ["@ocaml//platform/executor:vm"],
    visibility              = ["//visibility:public"],
)

##########
toolchain_selector(
    name                    = "ocamlopt.byte",
    toolchain               = "@ocaml//toolchain/adapters/local:ocamlopt.byte",
    build_host_constraints    = ["@ocaml//platform/executor:vm"],
    target_host_constraints  = ["@ocaml//platform/executor:sys"],
    visibility              = ["//visibility:public"],
)

##########
toolchain_selector(
    name                    = "ocamlc.byte",
    toolchain               = "@ocaml//toolchain/adapters/local:ocamlc.byte",
    build_host_constraints  = ["@ocaml//platform/executor:vm"],
    target_host_constraints  = ["@ocaml//platform/executor:vm"],
    visibility              = ["//visibility:public"],
)

# ##########
# toolchain_selector(
#     name           = "_vm", # *>vm
#     toolchain      = "@ocaml//toolchain/adapters/local:ocamlc.opt",
#     target_host_constraints  = ["@ocaml//platform/executor:vm"],
#     visibility     = ["//visibility:public"],
# )

# ##########
# toolchain_selector(
#     name           = "__", # *>*
#     toolchain      = "@ocaml//toolchain/adapters/local:ocamlopt.opt",
#     visibility     = ["//visibility:public"],
# )
