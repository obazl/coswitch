# generated file - DO NOT EDIT

package(default_visibility=["//visibility:public"])

cc_library(
    name = "sdk",
    hdrs = glob(["caml/**"])
)

cc_library(
    name = "dbg",
    hdrs = glob(["caml/**"]),
    defines = ["RUNTIME_VARIANT_DEBUG"]
)

cc_library(
    name = "instrumented",
    hdrs = glob(["caml/**"]),
    defines = ["RUNTIME_VARIANT_INSTRUMENTED"]
)
