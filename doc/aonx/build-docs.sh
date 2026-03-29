#!/bin/bash
# Generates doc/aonx/index.html from doc/aonx/openapi.yaml using Scalar.
# No external tools required — the HTML loads Scalar from CDN.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
SPEC_FILE="$SCRIPT_DIR/openapi.yaml"
OUT_FILE="$SCRIPT_DIR/index.html"

# Inline the YAML spec into the HTML template
SPEC_CONTENT=$(cat "$SPEC_FILE")

cat > "$OUT_FILE" << 'HEADER'
<!DOCTYPE html>
<html>
<head>
  <title>AONX Protocol</title>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1" />
</head>
<body>
  <script id="api-reference" type="application/yaml">
HEADER

echo "$SPEC_CONTENT" >> "$OUT_FILE"

cat >> "$OUT_FILE" << 'FOOTER'
  </script>
  <script src="https://cdn.jsdelivr.net/npm/@scalar/api-reference"></script>
</body>
</html>
FOOTER

echo "Built: $OUT_FILE"
