#!/bin/sh
# paperboi.c -> size-optimized wasm -> deflated+base64 -> minified single-file index.html
set -e
clang --target=wasm32 -Oz -flto -nostdlib -fno-builtin -Wall -Wextra \
  -Wl,--no-entry -Wl,--lto-O3 -Wl,--strip-all -o paperboi.wasm paperboi.c
python3 - <<'EOF'
import base64, zlib
wasm = open("paperboi.wasm","rb").read()
defl = zlib.compress(wasm, 9)[2:-4]          # raw deflate stream
cl = sum(1 for _ in open("paperboi.c"))
src = open("shell.html").read()
sl = src.count("\n")
mini = "\n".join(l.strip() for l in src.splitlines() if l.strip())
html = (mini.replace("__WASM__", base64.b64encode(defl).decode())
            .replace("__CL__", str(cl)).replace("__SL__", str(sl))
            .replace("__WK__", str(len(wasm)//1024)).replace("__DK__", str(len(defl)//1024)))
html = html.replace("__TK__", str((len(html)+1023)//1024))
open("index.html","w").write(html)
print(f"index.html: {(len(html)+1023)//1024} KB total "
      f"(wasm {len(wasm)//1024} KB -> {len(defl)//1024} KB deflated; C {cl} lines, shell {sl} lines)")
EOF
