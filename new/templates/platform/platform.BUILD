# generated file - DO NOT EDIT
package(default_visibility = ["//visibility:public"])

## endocompilers
platform(name = "ocamlopt.opt",
         parents = ["@local_config_platform//:host"],
         constraint_values = [
             "@ocaml//platform/arch:sys",
             "@ocaml//platform/executor:sys",
             "@ocaml//platform/emitter:sys"
         ])

platform(name = "ocamlc.byte",
         parents = ["@local_config_platform//:host"],
         constraint_values = [
             "@ocaml//platform/arch:sys",
             "@ocaml//platform/executor:vm",
             "@ocaml//platform/emitter:vm"
         ])

## exocompilers
platform(name = "ocamlc.opt",
         parents = ["@local_config_platform//:host"],
         constraint_values = [
             "@ocaml//platform/arch:sys",
             "@ocaml//platform/executor:sys",
             "@ocaml//platform/emitter:vm"
         ])

platform(name = "ocamlopt.byte",
         parents = ["@local_config_platform//:host"],
         constraint_values = [
             "@ocaml//platform/arch:sys",
             "@ocaml//platform/executor:vm",
             "@ocaml//platform/emitter:sys"
         ])

