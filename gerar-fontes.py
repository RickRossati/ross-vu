#!/usr/bin/env python3
"""Regera RossVUFonts.h a partir da Liberation Sans do sistema.

Roda so quando precisar mudar a fonte. O .h fica versionado junto do codigo
para o build nao depender de ter pyftsubset instalado.
"""
import pathlib, subprocess

SRC = {'regular': '/usr/share/fonts/liberation-sans-fonts/LiberationSans-Regular.ttf',
       'bold':    '/usr/share/fonts/liberation-sans-fonts/LiberationSans-Bold.ttf'}

pathlib.Path('res').mkdir(exist_ok=True)
for name, path in SRC.items():
    subprocess.run(['pyftsubset', path, '--unicodes=U+0020-007E',
                    '--layout-features=', '--no-hinting', '--desubroutinize',
                    '--drop-tables+=DSIG', '--output-file=res/%s.ttf' % name], check=True)

def emit(var, path):
    d = pathlib.Path(path).read_bytes()
    body = '\n'.join('    ' + ''.join('0x%02x,' % b for b in d[i:i+20])
                     for i in range(0, len(d), 20))
    return ("static const unsigned char %s[] = {\n%s\n};\nstatic const unsigned int %s_size = %du;\n"
            % (var, body, var, len(d)))

out = ("/*\n * ROSS VU // fontes embutidas\n *\n"
       " * Liberation Sans (SIL OFL 1.1), reduzida para ASCII imprimivel com pyftsubset.\n"
       " * Embutir resolve dois problemas: o plugin nao depende mais de caminho de fonte\n"
       " * do sistema, e a versao Windows renderiza texto (la nao existe\n"
       " * /usr/share/fonts). Gerado por gerar-fontes.py, nao editar a mao.\n */\n#pragma once\n\n")
out += emit('kFontRegular', 'res/regular.ttf') + "\n" + emit('kFontBold', 'res/bold.ttf')
pathlib.Path('RossVUFonts.h').write_text(out)
print("RossVUFonts.h regerado")
