# ROSS VU
### ANALOG ENERGY METER

Medidor VU de agulha para Linux. VST3, CLAP, LV2 e standalone, do mesmo código.
Feito com DPF (DISTRHO Plugin Framework) + NanoVG.

---

## Quem fez isso, e por quê

Meu nome é Ricardo Rossati, mais conhecido como Mister RickRoss. Sou baixista há
29 anos, moro no Brasil. Produzo, gravo, já fui professor de contrabaixo na
School of Rock, já toquei em bandas, acompanhando artistas nacionais famosos.
Na minha carreira tive experiências com rádio, televisão, palco, viagens,
gravações.

E mesmo tendo atingido esse nível, aqui no Brasil eu sinto que a música é um
caminho pra poucos, e que os músicos não têm o espaço que merecem. Eu usava
Studio One no Windows, e era sempre tudo pago. Na escola de música onde eu
dava aula, o dono resolveu instalar Linux. Minha história com tecnologia vem
desde o Windows 3.11, onde eu instalava o Doom.

O Windows nunca me ofereceu a praticidade que o Linux oferecia. E quando
cheguei no Linux, senti falta de diversos plugins nativos, e eu não queria
ficar instalando ou craqueando plugin. Com o conhecimento que eu fui tendo
através de IA, comecei a codificar, já são 3, 4 anos codificando, e agora com
o Claude algumas coisas ficaram bem mais fáceis.

Além do ROSS VU eu tenho o Substance, em contínuo progresso de produção, tenho
um aplicativo chamado BandMate, e fui o primeiro brasileiro a lançar um clipe
com inteligência artificial. Você pode conferir tudo isso em
[misterrickross.com](https://misterrickross.com).

A ideia desse plugin veio, e sim, o Claude me ajudou a codificá-lo. O modelo
da agulha e os testes que provam os números são meus, o Claude me ajudou com
o código ao redor. Toda a prototipagem foi feita com muito carinho, os
cálculos estão todos no site. Estou convidando todo mundo pra usar o plugin e
também contribuir pra esse novo modelo, essa nova era, onde cada pessoa pode
contribuir de dentro de casa, sem gastar muito tempo e energia.

*Mister RickRoss / Ricardo Rossati*
*[misterrickross.com](https://misterrickross.com)*

---


## O que ele faz de verdade

Não é um ponteiro decorativo animado por RMS. O movimento é um sistema de segunda
ordem real, com retificação de média, exatamente como o instrumento eletromecânico:

| medida | norma VU | ROSS VU (medido) |
|---|---|---|
| tempo para 99% da deflexão | 300 ms | 299 a 302 ms |
| overshoot | 1% a 1,5% | 0,86% a 1,00% |
| senoide no nível de referência | 0,00 VU | 0,000 VU |
| tom 6 dB abaixo da referência | -6,00 VU | -6,012 VU |

Constantes: zeta = 0,826 e omega0 = 14 rad/s. Conferido em 44,1 kHz, 48 kHz e 96 kHz.

A escala do mostrador é desenhada pela deflexão linear do movimento,
`p = 10^(VU/20) / 10^(3/20)`, que é o que produz a compressão à esquerda e a
abertura perto do zero. Não é uma escala inventada para parecer vintage.

Por ser de **média retificada** e não RMS, ele lê material com muito crest factor
abaixo do RMS, igual a um VU de ferro. Isso é o comportamento correto, não um bug.

## Pico verdadeiro

O LED e a leitura são **dBTP**, por sobreamostragem 4x polifásica (12 coeficientes
por fase, sinc janelado por Blackman-Harris, no espírito da ITU-R BS.1770-4).
O caso clássico, senoide de fundo de escala em fs/4 com fase de 45 graus:

| | leitura |
|---|---|
| pico de amostra (o que quase todo medidor mostra) | -3,01 dBFS |
| ROSS VU | **-0,20 dBTP** |
| valor real | 0,00 dB |

Erro dentro da tolerância de 4x da BS.1770. Um medidor de pico de amostra erraria
3 dB nesse sinal, e é exatamente aí que o limiter estoura.

## Qual calibração usar

`-18 dBFS = 0 VU` é convenção de **gravação e mixagem**, herdada do `0 VU = +4 dBu`
analógico. Master pronto não vive lá: fica 4 a 6 dB acima, então bate no vermelho e
trava a agulha. Isso é o instrumento certo dizendo a verdade, não defeito.

| o que você está olhando | calibração |
|---|---|
| gravando, ganho de entrada | -20 ou -18 |
| mixando, barramento e canais | -18 ou -16 |
| mix pronta antes da masterização | -14 |
| master comercial, referência de A/B | -12, -10 ou -8 |

Um mesmo master de -14 dBFS RMS lê `+4,0 VU` no CAL -18, `0,0 VU` no CAL -14 e
`-4,0 VU` no CAL -10. Medido, não estimado.

**No REAPER a cadeia de FX é pré-fader.** Se você abaixou o fader da faixa de
referência para casar volume no A/B, o medidor continua vendo o nível cheio,
não o que o seu ouvido está escutando.

## A lâmpada de pico

Uma lâmpada que só acende quando já estourou não serve para nada. O trabalho dela
é mostrar o transiente que a agulha lenta esconde, então ela precisa disparar
**antes** do clipe. O limiar é escolhido no clique, em dBTP:

`-12` · `-9` · `-6` · `-3` · `-1` · `-0,1`

Padrão em **-6 dBTP**. A retenção é curta de propósito, **400 ms**, para ler como
piscada e não como lâmpada travada. Medido: estouro de 30 ms acima do limiar
acende por 430 ms, seis estouros dão seis piscadas, e nada abaixo do limiar acende.

## Controles

| controle | faixa | observação |
|---|---|---|
| CAL | -20 / -18 / -16 / -14 / -12 / -10 / -8 dBFS | clique cicla; define o que é 0 VU |
| RESPONSE | 600 ms a 100 ms | ao meio dá os 300 ms normativos |
| PEAK | -12 / -9 / -6 / -3 / -1 / -0,1 dBTP | clique cicla; pisca 430 ms |
| INPUT | -24 a +24 dB | **altera o áudio**, não só o medidor |
| OUTPUT | -24 a +24 dB | ganho de saída |
| MODE | L+R / LEFT / RIGHT / SIDE / L \| R | clique no rótulo no canto inferior esquerdo do mostrador |

No modo **L | R** aparece um segundo ponteiro, mais fino e claro, com o canal
direito. O da esquerda continua sendo o ponteiro cheio.

## Onde escolher preset e modo

Os dois ficam na própria interface, não escondidos na lista de parâmetros do host.

- **Preset:** o seletor `‹ MIX BUS -18 ›` no meio do cabeçalho. As setas andam pela
  lista. Se você mexer em qualquer controle depois, ele passa a mostrar `CUSTOM`.
- **Modo:** o botão no canto inferior esquerdo do mostrador, o que mostra `L+R`.
  Clique cicla entre `L+R`, `LEFT`, `RIGHT`, `SIDE` e `L | R`.

**CAL** e **PEAK** também são clicáveis e ciclam. Tudo que responde ao clique
acende quando o mouse passa por cima, então dá para descobrir sem manual.

No VST3 os presets também aparecem no host como um parâmetro chamado
`Current Program`, e no LV2 como presets do próprio formato.

## Presets de fábrica

| preset | 0 VU | response | modo | lâmpada |
|---|---|---|---|---|
| MIX BUS -18 | -18 dBFS | 300 ms | L+R | -6 dBTP |
| TRACKING -20 | -20 dBFS | 100 ms | L+R | -12 dBTP |
| MIX READY -14 | -14 dBFS | 300 ms | L+R | -3 dBTP |
| MASTER -10 | -10 dBFS | 300 ms | L+R | -1 dBTP |
| REFERENCE -12 | -12 dBFS | 300 ms | L+R | -1 dBTP |
| STEREO L \| R | -18 dBFS | 300 ms | L \| R | -6 dBTP |
| PROGRAM SLOW | -18 dBFS | 600 ms | L+R | -6 dBTP |

## Compilar e instalar

O ROSS VU compila contra o DPF, que fica ao lado como pasta irmã. O DPF tem um
submódulo próprio (pugl), então é o clone do DPF que precisa de `--recursive`,
não este repositório (este aqui não tem submódulo).

```
git clone https://github.com/RickRossati/ross-vu
git clone --recursive https://github.com/DISTRHO/DPF
cd ross-vu
make            # VST3 + CLAP + LV2 + standalone, em ../bin
./instalar.sh   # ou este: compila e copia para ~/.vst3, ~/.clap, ~/.lv2
```

Commit do DPF que funciona: `4238e1c` (set/2026). O ramo `develop` do DPF anda;
se um commit posterior quebrar o build, use esse dentro do DPF.

No REAPER, depois de instalar: Options > Preferences > Plug-ins > VST >
Re-scan. O CLAP em `~/.clap` é varrido junto.

**Build de release para Linux é outro comando.** O `make` liga o binário na
glibc da máquina onde compila: feito aqui no Fedora 44, ele exige glibc 2.43 e
não abre no Ubuntu 22.04, no Debian 12 nem em quase nada. Foi o que aconteceu
com a 1.4 no primeiro dia no Reddit. `./build-linux-compat.sh` compila dentro de
um Ubuntu 20.04 pelo podman, exige só glibc 2.27, gera os `.ttl` do LV2 (passo
separado no DPF; sem eles nenhum host acha o LV2) e sai em `../bin-compat`.

## Estrutura

```
ROSS-VU/
├── DistrhoPluginInfo.h   identidade, formatos, lista de parâmetros
├── RossVUPlugin.cpp      DSP: balística, calibração, pico, ganho
├── RossVUUI.cpp          desenho NanoVG e interação
├── Makefile
└── instalar.sh
```

## Windows

`make windows` faz o build cruzado com mingw-w64 e cospe VST3 + CLAP em
`../bin-win`. O binário é PE32+ x64 e **não depende de nenhuma DLL do mingw**,
só das DLLs do próprio Windows.

O instalador `.exe` está pronto em `windows/ROSS-VU-1.4.0-windows-x64-setup.exe`
(NSIS). Grava em `Common Files\VST3` e `Common Files\CLAP`, registra
desinstalação em Programas e Recursos. Para recompilar depois de mudar a versão:

```
sudo dnf install mingw64-nsis
makensis windows/ROSSVU.nsi
```

**Gotcha do pacote Fedora:** o `mingw64-nsis` desta versão vem sem o stub padrão
`zlib-x86-unicode` (só traz os `-amd64-unicode`), então `makensis` falha antes de
processar qualquer script, mesmo `-VERSION` sozinho. Corrigir sem root: montar um
`NSISDIR` local todo em symlinks para `/usr/share/nsis/`, exceto `Stubs/`, que
ganha um `zlib-x86-unicode` symlinkado para `zlib-amd64-unicode`. Depois
`NSISDIR=/caminho/local makensis windows/ROSSVU.nsi` funciona normal.

**Ressalva honesta:** nem o `.exe` nem o `.zip` foram abertos numa DAW em Windows real,
porque a máquina onde ele nasce não tem Windows. Foi conferido como PE válido
com as exportações certas (`GetPluginFactory` no VST3, `clap_entry` no CLAP).
A tentativa de testar no Wine falhou antes do plugin rodar: o
`wglChoosePixelFormatARB` do Wine devolve zero formatos, e o pugl desiste aí.
Isso atinge qualquer app DPF no Wine desta máquina, não é defeito do ROSS VU.

## As fontes são embutidas

`RossVUFonts.h` carrega Liberation Sans (OFL) reduzida para ASCII imprimível,
17 KB no total, gerada por `gerar-fontes.py`. Antes o código abria a fonte por
caminho absoluto do sistema, o que deixaria o plugin sem texto nenhum no Windows
e quebraria em qualquer distro sem a Liberation instalada.

## O que ainda não tem

- Build para macOS (o código é portátil, falta a toolchain)
- Validação do build Windows numa DAW de verdade
- Histórico de nível ao longo do tempo
- Redimensionamento com passos fixos (hoje é livre, mantendo a proporção)

## Como isso foi verificado

Não é papo. Os testes ficam em `tests/`, e `tests/rodar.sh` roda os tres:

- `vutest2.cpp` extrai o `struct Movement` do próprio `RossVUPlugin.cpp` e mede
  tempo de subida, overshoot e nível de repouso em 44,1k, 48k e 96k.
- `tptest.cpp` extrai o `struct TruePeak` real e compara com o pico analítico
  em seis sinais, incluindo o caso de fs/4.
- `lamptest.cpp` extrai o `struct PeakLamp` real e conta piscadas e duração em
  seis combinações de estouro e limiar.
- Teste de ponta a ponta: tom de 1 kHz gerado em -18,01 dBFS RMS, roteado direto
  para as portas de entrada do plugin pelo PipeWire, com a agulha parando em
  0,0 VU e o true peak em -15,0 dBTP.

GPL-3.0-or-later. Mister RickRoss, 2026.
