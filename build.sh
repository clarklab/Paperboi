#!/bin/sh
# paperboi.c -> paperboi.wasm -> single-file index.html (wasm embedded as base64,
# real line/byte counts injected into the in-game about panel)
set -e
clang --target=wasm32 -O2 -nostdlib -fno-builtin -Wall -Wextra -Wl,--no-entry -o paperboi.wasm paperboi.c
python3 - <<'EOF'
import base64
wasm = open("paperboi.wasm","rb").read()
cl = sum(1 for _ in open("paperboi.c"))
shell = open("shell.html").read()
sl = shell.count("\n")
html = (shell.replace("__WASM__", base64.b64encode(wasm).decode())
             .replace("__CL__", str(cl)).replace("__SL__", str(sl))
             .replace("__WK__", str(len(wasm)//1024)))
html = html.replace("__TK__", str((len(html)+1023)//1024))
open("index.html","w").write(html)
print(f"index.html: {len(html)//1024} KB (wasm {len(wasm)//1024} KB, C {cl} lines, shell {sl} lines)")
EOF
