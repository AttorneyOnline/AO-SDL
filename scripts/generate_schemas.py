#!/usr/bin/env python3
"""Generate C++ JsonSchema definitions from an OpenAPI 3.x spec.

Reads the OpenAPI YAML file, extracts request body schemas from all
endpoints (keyed by operationId) and all named schemas from
components/schemas, then emits a C++ source file with static
JsonSchema definitions.

Usage:
    python3 generate_schemas.py <openapi.yaml> <output.cpp>

The generated file provides:
    const JsonSchema& aonx_request_schema(const std::string& operation_id);
    const JsonSchema& aonx_component_schema(const std::string& name);
"""

import sys
from pathlib import Path

try:
    import yaml
except ImportError:
    print("ERROR: PyYAML is required. Install with: pip install pyyaml", file=sys.stderr)
    sys.exit(1)


def fix_yaml_booleans(obj):
    """Recursively fix PyYAML's boolean coercion of keys like 'on', 'off', 'yes', 'no'.

    YAML 1.1 (used by PyYAML safe_load) treats these as booleans,
    but JSON Schema property names are always strings. Convert any
    non-string dict key to str (True -> "on" via the reverse map,
    False -> "off").
    """
    BOOL_TO_STR = {True: "on", False: "off"}

    if isinstance(obj, dict):
        return {
            (BOOL_TO_STR.get(k, k) if not isinstance(k, str) else k): fix_yaml_booleans(v)
            for k, v in obj.items()
        }
    if isinstance(obj, list):
        return [
            BOOL_TO_STR.get(item, item) if isinstance(item, bool)
            else fix_yaml_booleans(item)
            for item in obj
        ]
    return obj


def resolve_ref(spec: dict, ref: str) -> dict:
    """Resolve a $ref string like '#/components/schemas/Foo'."""
    parts = ref.lstrip("#/").split("/")
    node = spec
    for p in parts:
        node = node[p]
    return node


def ref_name(ref: str) -> str:
    """Extract schema name from $ref string."""
    return ref.split("/")[-1]


def schema_to_cpp(spec: dict, schema: dict, indent: int = 0) -> str:
    """Convert a JSON Schema node to a C++ JsonSchema builder expression."""
    pad = "    " * indent

    # Handle $ref
    if "$ref" in schema:
        name = ref_name(schema["$ref"])
        resolved = resolve_ref(spec, schema["$ref"])
        return schema_to_cpp(spec, resolved, indent)

    typ = schema.get("type", "object")

    # String with enum constraint
    if typ == "string" and "enum" in schema:
        values = ", ".join(f'"{v}"' for v in schema["enum"])
        return f"JsonSchema::string_enum({{{values}}})"

    # Leaf types
    if typ == "string":
        return "JsonSchema::string_type()"
    if typ == "integer":
        return "JsonSchema::integer_type()"
    if typ == "number":
        return "JsonSchema::number_type()"
    if typ == "boolean":
        return "JsonSchema::boolean_type()"

    # Array
    if typ == "array":
        items = schema.get("items", {})
        item_expr = schema_to_cpp(spec, items, indent)
        return f"JsonSchema::array({item_expr})"

    # Object with additionalProperties (string_map)
    if typ == "object" and "additionalProperties" in schema and "properties" not in schema:
        val_schema = schema["additionalProperties"]
        val_expr = schema_to_cpp(spec, val_schema, indent)
        return f"JsonSchema::string_map({val_expr})"

    # Object with properties
    if typ == "object":
        props = schema.get("properties", {})
        required_set = set(schema.get("required", []))

        if not props:
            # Unconstrained object (e.g. moderation params) — no validation
            return "JsonSchema::object().build()"

        lines = [f"JsonSchema::object()"]
        for prop_name, prop_schema in props.items():
            is_req = prop_name in required_set
            method = "required" if is_req else "optional"
            prop_expr = schema_to_cpp(spec, prop_schema, indent + 1)
            lines.append(f'{pad}    .{method}("{prop_name}", {prop_expr})')
        lines.append(f"{pad}    .build()")
        return "\n".join(lines)

    # Fallback — unknown type, skip validation
    return "JsonSchema()"


def collect_request_schemas(spec: dict) -> dict[str, dict]:
    """Collect all request body schemas keyed by operationId."""
    result = {}
    for path, path_item in spec.get("paths", {}).items():
        for method in ("get", "post", "put", "patch", "delete"):
            op = path_item.get(method)
            if not op:
                continue
            op_id = op.get("operationId")
            if not op_id:
                continue
            req_body = op.get("requestBody")
            if not req_body:
                continue
            content = req_body.get("content", {})
            json_content = content.get("application/json", {})
            schema = json_content.get("schema")
            if schema:
                result[op_id] = schema
    return result


def generate_cpp(spec: dict, output_path: str, prefix: str = "aonx") -> None:
    """Generate the C++ source file.

    Args:
        prefix: Function name prefix. Default "aonx" produces
                aonx_request_schema() / aonx_component_schema().
                Use a different prefix to avoid symbol collisions
                when multiple specs are compiled into the same binary.
    """
    request_schemas = collect_request_schemas(spec)
    component_schemas = spec.get("components", {}).get("schemas", {})

    lines = []
    lines.append("// Auto-generated by scripts/generate_schemas.py — do not edit.")
    lines.append("// Source: doc/aonx/openapi.yaml")
    lines.append("")
    lines.append('#include "utils/JsonSchema.h"')
    lines.append("")
    lines.append("#include <stdexcept>")
    lines.append("#include <string>")
    lines.append("#include <unordered_map>")
    lines.append("")

    # Forward-declare component schemas as functions to handle circular refs
    # (none expected, but safe)
    lines.append("// -- Component schemas (from components/schemas) --")
    lines.append("")

    # Generate component schemas
    for name, schema in component_schemas.items():
        cpp_expr = schema_to_cpp(spec, schema, 0)
        lines.append(f"static const JsonSchema& schema_{name}() {{")
        lines.append(f"    static const JsonSchema s =")
        # Indent the expression
        for i, line in enumerate(cpp_expr.split("\n")):
            suffix = ";" if i == cpp_expr.count("\n") else ""
            lines.append(f"        {line}{suffix}")
        lines.append(f"    return s;")
        lines.append(f"}}")
        lines.append("")

    # Generate request body schemas
    lines.append("// -- Request body schemas (keyed by operationId) --")
    lines.append("")

    for op_id, schema in request_schemas.items():
        # Resolve top-level $ref for request schemas
        if "$ref" in schema:
            ref = ref_name(schema["$ref"])
            lines.append(f"// {op_id} -> $ref {ref}")
            lines.append(f"static const JsonSchema& request_{op_id}() {{")
            lines.append(f"    return schema_{ref}();")
            lines.append(f"}}")
        else:
            cpp_expr = schema_to_cpp(spec, schema, 0)
            lines.append(f"static const JsonSchema& request_{op_id}() {{")
            lines.append(f"    static const JsonSchema s =")
            for i, line in enumerate(cpp_expr.split("\n")):
                suffix = ";" if i == cpp_expr.count("\n") else ""
                lines.append(f"        {line}{suffix}")
            lines.append(f"    return s;")
            lines.append(f"}}")
        lines.append("")

    # Lookup functions
    lines.append("// -- Public lookup functions --")
    lines.append("")
    lines.append(f"const JsonSchema& {prefix}_request_schema(const std::string& operation_id) {{")
    lines.append("    static const std::unordered_map<std::string, const JsonSchema& (*)()> map = {")
    for op_id in request_schemas:
        lines.append(f'        {{"{op_id}", &request_{op_id}}},')
    lines.append("    };")
    lines.append("    auto it = map.find(operation_id);")
    lines.append('    if (it == map.end()) throw std::runtime_error("Unknown operation: " + operation_id);')
    lines.append("    return it->second();")
    lines.append("}")
    lines.append("")
    lines.append(f"const JsonSchema& {prefix}_component_schema(const std::string& name) {{")
    lines.append("    static const std::unordered_map<std::string, const JsonSchema& (*)()> map = {")
    for name in component_schemas:
        lines.append(f'        {{"{name}", &schema_{name}}},')
    lines.append("    };")
    lines.append("    auto it = map.find(name);")
    lines.append('    if (it == map.end()) throw std::runtime_error("Unknown schema: " + name);')
    lines.append("    return it->second();")
    lines.append("}")
    lines.append("")

    Path(output_path).parent.mkdir(parents=True, exist_ok=True)
    Path(output_path).write_text("\n".join(lines) + "\n")
    print(f"Generated {output_path} ({len(request_schemas)} request schemas, "
          f"{len(component_schemas)} component schemas)")


def main():
    import argparse
    parser = argparse.ArgumentParser(description="Generate C++ JsonSchema definitions from an OpenAPI spec.")
    parser.add_argument("spec", help="Path to OpenAPI YAML file")
    parser.add_argument("output", help="Path to output C++ file")
    parser.add_argument("--prefix", default="aonx",
                        help="Function name prefix (default: aonx)")
    args = parser.parse_args()

    with open(args.spec) as f:
        spec = fix_yaml_booleans(yaml.safe_load(f))

    generate_cpp(spec, args.output, prefix=args.prefix)


if __name__ == "__main__":
    main()
