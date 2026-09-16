#!/usr/bin/env bash
# Build Linux que roda em distro velha.
#
# POR QUE ISSO EXISTE: a maquina de build e Fedora 44, glibc 2.43. Todo binario
# compilado aqui passa a exigir glibc 2.43, e no primeiro dia publico um usuario
# de Ardour 9 no Reddit recebeu:
#
#   /lib/x86_64-linux-gnu/libm.so.6: version `GLIBC_2.43' not found
#
# Ubuntu 22.04 tem 2.35, Ubuntu 24.04 tem 2.39, Debian 12 tem 2.36. Ou seja,
# o binario da 1.4 so abria em Fedora de 2026 e quase mais nada.
#
# A saida e compilar dentro de um Ubuntu 20.04 (glibc 2.31). Binario Linux e
# compativel pra frente: o que roda em 2.31 roda em qualquer glibc mais nova.
#
# Nao e Debian 11 porque o repositorio de seguranca dele saiu do deb.debian.org
# quando o suporte acabou: o apt da 404 em meio pacote e o build nem comeca.
#
# Mesmas flags do Makefile (-O2 -ffast-math): a balistica foi medida com elas.
#
# Saida em ../bin-compat, separada do ../bin de sempre.
set -euo pipefail

AQUI="$(cd "$(dirname "$(readlink -f "$0")")" && pwd)"
RAIZ="$(dirname "$AQUI")"            # a pasta que tem DPF/ e ROSS-VU/ lado a lado
IMAGEM="${IMAGEM:-docker.io/library/ubuntu:20.04}"

echo ">> compilando em $IMAGEM, fontes de $RAIZ"

# Caminho sem espaco dentro do container: o DPF nao poe aspas no TARGET_DIR.
# label=disable em vez de :Z para NAO reetiquetar os arquivos do projeto no host.
podman run --rm --security-opt label=disable \
  -v "$RAIZ":/src \
  "$IMAGEM" bash -euo pipefail -c '
    export DEBIAN_FRONTEND=noninteractive
    apt-get update -qq
    apt-get install -y -qq --no-install-recommends \
        build-essential pkg-config \
        libgl-dev libx11-dev libxext-dev libxrandr-dev libxcursor-dev \
        libjack-jackd2-dev >/dev/null
    echo ">> glibc do container: $(ldd --version | head -1)"
    cd /src/ROSS-VU
    make BUILD_DIR_SUFFIX=-compat DPF_TARGET_DIR=/src/bin-compat vst3 clap lv2 jack -j"$(nproc)"
  '

# O LV2 so e achado pela DAW com manifest.ttl, ROSSVU.ttl e presets.ttl ao lado
# do .so, e o `make lv2` do DPF NAO gera esses arquivos: e um passo separado.
# Sem isso o pacote passa em todo teste que carrega o .so e o plugin some do
# host. O gerador roda aqui fora, contra o .so de dentro; o texto e o mesmo.
echo
echo ">> gerando os .ttl do LV2"
( cd "$RAIZ/DPF" && ./utils/generate-ttl.sh "../bin-compat" )
for f in manifest.ttl ROSSVU.ttl presets.ttl; do
  [ -s "$RAIZ/bin-compat/ROSSVU.lv2/$f" ] || { echo "FALTOU $f no bundle LV2"; exit 1; }
done

echo
echo ">> versao de glibc exigida por cada binario (tem que ser 2.31 ou menor):"
find "$RAIZ/bin-compat" -type f \( -name "*.so" -o -name "*.clap" -o -perm -u+x \) | while read -r f; do
  v=$(objdump -T "$f" 2>/dev/null | grep -oE 'GLIBC_[0-9.]+' | sort -V | tail -1)
  [ -n "$v" ] && printf '   %-12s %s\n' "$v" "${f#$RAIZ/}"
done
