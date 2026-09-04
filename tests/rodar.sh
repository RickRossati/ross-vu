#!/usr/bin/env bash
# Extrai o codigo real do plugin e mede. Nao usa copia, usa o fonte que compila.
set -euo pipefail
cd "$(dirname "$0")"
# os testes agora vivem dentro do repositorio, um nivel abaixo do fonte
SRC=../RossVUPlugin.cpp

python3 - "$SRC" <<'PY'
import sys
src = open(sys.argv[1], encoding='utf-8').read()
for name, out in (('struct Movement {', 'mov_extracted.h'),
                  ('struct TruePeak {', 'tp_extracted.h'),
                  ('struct PeakLamp {', 'lamp_extracted.h')):
    i = src.index(name); j = src.index('\n};', i) + 3
    open(out, 'w', encoding='utf-8').write(src[i:j])
print("estruturas extraidas do plugin")
PY

g++ -O2 -o vutest2 vutest2.cpp -lm && ./vutest2
echo
g++ -O2 -o tptest  tptest.cpp  -lm && ./tptest
echo
g++ -O2 -o lamptest lamptest.cpp -lm && ./lamptest
