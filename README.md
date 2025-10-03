## Surucursed with OpenCV

Uma extensão do projeto Surucursed original que adiciona controle por detecção facial usando OpenCV e C++, além dos controles tradicionais por teclado via SDL2.
 
--- 

## Diagrama UML
![Diagrama UML](.png)

---

## 📋 Sobre o Projeto
Este projeto expande o jogo Surucursed com funcionalidades de visão computacional, permitindo que os jogadores controlem a cobra tanto pelo teclado tradicional quanto através de movimentos da cabeça detectados pela webcam usando OpenCV.

---

## ✨ Funcionalidades
Controle por Teclado: Mantém todas as funcionalidades originais do Surucursed usando SDL2

Controle por Movimento Facial: Use movimentos da cabeça para controlar a direção da cobra via OpenCV

Detecção em Tempo Real: Utiliza OpenCV para detectar rostos e seus movimentos

Interface SDL2: Renderização gráfica eficiente com SDL2 e SDL_image

Alternância de Modos: Troque entre teclado e controle facial durante o jogo

---

## 🛠️ Tecnologias Utilizadas
C++17

mPlayer - Biblioteca para áudios

SDL2 - Biblioteca multimídia

SDL_image - Carregamento de imagens

SLD_ttf - Carregamento de textos

OpenCV - Visão computacional e detecção facial

--- 

## 📦 Pré-requisitos
Ubuntu/Debian\
bash\
sudo apt-get update\
sudo apt-get install -y \
    g++ \
    cmake \
    mplayer\
    libsdl2-dev \
    libsdl2-image-dev \
    libsdl2-ttf-dev\
    libopencv-dev


Windows (vcpkg)\
bash\
vcpkg install sdl2\
sdl2-image \
sdl2-ttf
opencv4

macOS (Homebrew)\
bash\
brew install sdl2 \
sdl2-image \
sdl2-ttf\
opencv

---

## 🚀 Como Compilar e Executar
Compilação com CMake\
Clone o repositório:

bash\
git clone [URL_DO_REPOSITORIO]\
cd surucursed_opencv\
Crie o diretório de build e compile:

./compile.sh
