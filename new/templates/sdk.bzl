load("@rules_ocaml//ocaml:providers.bzl",
     "OcamlRuntimeMarker")

def _ocaml_debug_sdk_impl(ctx):

    cc_info = CcInfo(
        compilation_context = cc_common.create_compilation_context(
            headers = depset(ctx.files.hdrs)
        ),
    )

    return [
        cc_info,
        OcamlRuntimeMarker(variant = "d")
    ]

#########################
ocaml_debug_sdk = rule(
  implementation = _ocaml_debug_sdk_impl,
    doc = "",
    attrs = dict(
        hdrs = attr.label_list(
            allow_files = [".h", ".tbl"]
        )
    )
)
