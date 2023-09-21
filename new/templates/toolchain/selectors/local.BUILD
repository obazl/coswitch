# generated file - DO NOT EDIT

load("@rules_ocaml//toolchain:BUILD.bzl", "toolchain_selector")

exports_files(glob(["*.bazel"]))

################ endocompilers first ################

###################
toolchain_selector(
    name      = "ocamlopt.opt.endo",
    toolchain = "@ocaml//toolchain/adapters/local:ocamlopt.opt",
    build_host_constraints  = [
        "@ocaml//platform/arch:sys",
        "@ocaml//platform/executor:sys",
        "@ocaml//platform/emitter:sys"
    ],
    target_host_constraints  = [
        "@ocaml//platform/arch:sys",
        "@ocaml//platform/executor:sys",
        # "@ocaml//platform/emitter:sys"
    ],
    visibility     = ["//visibility:public"],
)

##########
toolchain_selector(
    name      = "ocamlc.byte.endo",
    toolchain = "@ocaml//toolchain/adapters/local:ocamlc.byte",
    build_host_constraints  = [
        "@ocaml//platform/arch:sys",
        "@ocaml//platform/executor:vm",
        "@ocaml//platform/emitter:vm"
    ],
    target_host_constraints  = [
        "@ocaml//platform/arch:sys",
        "@ocaml//platform/executor:vm"
    ],
    visibility              = ["//visibility:public"],
)

################ now exocompilers ################

###################
toolchain_selector(
    name      = "ocamlc.opt.exo",
    toolchain = "@ocaml//toolchain/adapters/local:ocamlc.opt",
    build_host_constraints  = [
        "@ocaml//platform/arch:sys",
        "@ocaml//platform/executor:sys",
        "@ocaml//platform/emitter:vm"
    ],
    target_host_constraints  = [
        "@ocaml//platform/arch:sys",
        "@ocaml//platform/executor:vm",
    ],
    visibility              = ["//visibility:public"],
)

##########
toolchain_selector(
    name      = "ocamlopt.byte.exo",
    toolchain = "@ocaml//toolchain/adapters/local:ocamlopt.byte",
    build_host_constraints    = [
        "@ocaml//platform/arch:sys",
        "@ocaml//platform/executor:vm",
        "@ocaml//platform/emitter:sys"
    ],
    target_host_constraints  = [
        "@ocaml//platform/arch:sys",
        "@ocaml//platform/executor:sys"
    ],
    visibility              = ["//visibility:public"],
)

################ last, "tool" endocompilers ################
## These are exo-endo-compilers, so to speak.
## These will be selected for toolchain transition
## where buildhost is an exocompiler, no matter the targethost
toolchain_selector(
    name                    = "ocamlopt.byte.endo",
    toolchain               = "@ocaml//toolchain/adapters/local:ocamlopt.byte",
    build_host_constraints    = [
        "@ocaml//platform/arch:sys",
        "@ocaml//platform/executor:vm",
        "@ocaml//platform/emitter:sys"
    ],
    target_host_constraints  = [
        "@ocaml//platform/arch:sys",
        ## FAKE: target executor is sys (=build emitter), but
        ## toolchain transition makes build and target equal
        ## critical point is the emitter is used to select
        ## opam deps
        "@ocaml//platform/executor:vm",
        "@ocaml//platform/emitter:sys"
    ],
    visibility              = ["//visibility:public"],
)

###################
toolchain_selector(
    name      = "ocamlc.opt.endo",
    toolchain = "@ocaml//toolchain/adapters/local:ocamlc.opt",
    build_host_constraints  = [
        "@ocaml//platform/arch:sys",
        "@ocaml//platform/executor:sys",
        "@ocaml//platform/emitter:vm"
    ],
    target_host_constraints  = [
        "@ocaml//platform/arch:sys",
        "@ocaml//platform/executor:sys",
        "@ocaml//platform/emitter:vm"
    ],
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
