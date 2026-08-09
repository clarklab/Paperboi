#!/bin/sh
# paperboi.c -> paperboi.wasm -> single-file index.html (wasm embedded as base64)
set -e
clang --target=wasm32 -O2 -nostdlib -fno-builtin -Wall -Wextra -Wl,--no-entry -o paperboi.wasm paperboi.c
python3 - <<'EOF'
import base64
wasm = base64.b64encode(open("paperboi.wasm","rb").read()).decode()
html = open("shell.html").read().replace("__WASM__", wasm)
open("index.html","w").write(html)
print(f"index.html: {len(html)//1024} KB (wasm {len(wasm)*3//4//1024} KB)")
EOF
