#!/usr/bin/env bash
# ROSS VU // compila e instala nos diretorios de plugin do usuario
set -euo pipefail
cd "$(dirname "$0")"

make "$@"

# manifesto LV2 (o Makefile do DPF nao gera sozinho nesta versao)
if [ -d ../bin/ROSSVU.lv2 ]; then
  make -C ../DPF/utils/lv2-ttl-generator >/dev/null
  ( cd .. && DPF_PATH=./DPF ./DPF/utils/generate-ttl.sh >/dev/null )
fi

install -d "$HOME/.vst3" "$HOME/.clap" "$HOME/.lv2"

rm -rf "$HOME/.vst3/ROSSVU.vst3" "$HOME/.lv2/ROSSVU.lv2" "$HOME/.clap/ROSSVU.clap"
cp -r ../bin/ROSSVU.vst3 "$HOME/.vst3/"
cp -r ../bin/ROSSVU.lv2  "$HOME/.lv2/"
cp    ../bin/ROSSVU.clap "$HOME/.clap/"

echo
echo "instalado:"
echo "  VST3       $HOME/.vst3/ROSSVU.vst3"
echo "  CLAP       $HOME/.clap/ROSSVU.clap"
echo "  LV2        $HOME/.lv2/ROSSVU.lv2"
echo "  standalone $(cd .. && pwd)/bin/ROSSVU"
