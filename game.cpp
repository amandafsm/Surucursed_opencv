#include "./game.h"
#include "./constants.h"
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_surface.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <stdlib.h>

// --- OpenCV integration (added) ---
#include <opencv2/opencv.hpp>
// Use uma flag para garantir inicialização única da câmera
static cv::VideoCapture g_cap;
static bool g_cv_initialized = false;

// O tamanho do "Tabuleiro" em quantidade de quadrados que a cobra está
#define MATRIX_WIDTH 20
#define MATRIX_HEIGHT 20

//Tamanho "ideal" que vai servir como base para comparar estimadamente o tamanho de cada quadradinho na tela
#define IDEAL_CELL_SIZE WINDOW_WIDTH/MATRIX_WIDTH

// MACROS com as coordenadas iniciais da cabeça e da cauda da cobra
#define SNAKE_TAILX 5
#define SNAKE_TAILY 4
#define SNAKE_HEADX 5
#define SNAKE_HEADY 3

#define INITIAL_SNAKE_SIZE 3
#define INITIAL_FRUIT_QUANTITY 5

// Variaveis para gerir o tempo de jogo para movimentação da cobrinha
Uint32 last_update_time = 0;
const Uint32 update_interval = 400;  // Intervalo em milissegundos (400ms)
int slashcount = 0;

// MACROS com os parametros de cores (RGB alpha)
#define RED 255,0,0,255
#define GREEN 0,255,0,255
#define MANGENTA 255,0,255,255
#define BLACK 0,0,0,255

//Variavel da pontuação
int score = 0;
#define HIGHSCORE_FILE "highscore.txt"

// Variáveis para o viewport ajustado
static SDL_Rect game_viewport = {0};
static int cell_size = 0;  // Tamanho real das células (quadradas)

//Definindo variáveis para os sprites
SDL_Texture* CabecaBaixo_texture = NULL;
SDL_Texture* CabecaCima_texture = NULL;
SDL_Texture* CabecaDireita_texture = NULL;
SDL_Texture* CabecaEsquerda_texture = NULL;

SDL_Texture* CaudaBaixo_texture = NULL;
SDL_Texture* CaudaCima_texture = NULL;
SDL_Texture* CaudaDireita_texture = NULL;
SDL_Texture* CaudaEsquerda_texture = NULL;

SDL_Texture* CurvaBaixoDireita_texture  = NULL;
SDL_Texture* CurvaBaixoEsquerda_texture  = NULL;
SDL_Texture* CurvaCimaDireita_texture  = NULL;
SDL_Texture* CurvaCimaEsquerda_texture  = NULL;

SDL_Texture* retoDireitaEsquerda_texture  = NULL;
SDL_Texture* retoEsquerdaDireita_texture  = NULL;
SDL_Texture* retoVertical_texture  = NULL;

SDL_Texture* pitu_texture  = NULL;
SDL_Texture* manga_texture  = NULL;
SDL_Texture* caju_texture  = NULL;
SDL_Texture* limao_texture  = NULL;

SDL_Texture* background_texture = NULL;
SDL_Texture* menu_texture = NULL;
SDL_Texture* mangabyte_texture = NULL;
SDL_Texture* gameover_texture = NULL;
TTF_Font* game_font = NULL;



// Definido na main, quando 0 ou FALSE o jogo para após executar a renderização
extern int game_is_running;

// Estados do jogo
typedef enum {
  GAME_STATE_MENU,
  GAME_STATE_PLAYING,
  GAME_STATE_SPLASH,
  GAME_STATE_GAMEOVER,
} GameState;


static GameState game_state = GAME_STATE_MENU; // Inicia o jogo como tela do Mangabyte
// Definindo o enum indica o que
// cada celula do mapa pode assumir
typedef enum mapTileType
{
  EMPTY_TILE,
  SNAKE_TILE,
  FRUIT_TILE
} mapTileType;

// Definindo o enum que indica as direções
typedef enum direction {UP,DOWN,LEFT,RIGHT} direction;

// Definindo o struct que indica o que cada
// celula do tabuleiro que a cobra ocupa deve ter
typedef struct snakeTile
{
  mapTileType type;
  direction forwardDirection;
} snakeTile;

typedef struct fruitTile
{
  mapTileType type;
  char sprite;
} fruitTile;


// Definindo o union que indica o que
// uma certa celula do mapa tem
typedef union mapTile
{
  mapTileType type;
  snakeTile snake;
  fruitTile fruit;

} mapTile;

// Matriz que armazena todas as celulas
// do mapa, a posição [0][0] é o canto
// inferior esquerdo
mapTile mapMatrix[MATRIX_WIDTH][MATRIX_HEIGHT];

// Definição da variavel que delega o delay do movimento da cobra
unsigned int last_movement_time = 0;

// Definição da posição da cabeça da cobra na matrix
int snake_headX;
int snake_headY;

// Definição da posição da cauda da cobra na matrix
int snake_tailX;
int snake_tailY;

// Head direction
direction head_dir = UP;

// Tamanho da cobra em células
int snake_size;

// Enum de verificação do tipo de célula da cobra
typedef enum {
  SEGMENT_STRAIGHT,
  SEGMENT_CURVE,
  SEGMENT_TAIL,
  SEGMENT_HEAD
} SegmentType;

// Função para determinar tipo da célula
SegmentType determine_segment_type(int x, int y, int is_head, int is_tail, int prev_dir) {
  if (is_head) return SEGMENT_HEAD;
  if (is_tail) return SEGMENT_TAIL;

  direction current_dir = mapMatrix[x][y].snake.forwardDirection;

  // Verifica se há mudança de direção em relação ao segmento anterior
  if (prev_dir != -1 && current_dir != prev_dir) {
      return SEGMENT_CURVE;
  }

  return SEGMENT_STRAIGHT;
}

// --Funções para renderização --

// Função para calcular e inicializar a área vizualizada contida na janela
void initialize_viewport() {
  // Calcula o tamanho máximo possível mantendo a proporção
  int max_cell_width = WINDOW_WIDTH / MATRIX_WIDTH;
  int max_cell_height = WINDOW_HEIGHT / MATRIX_HEIGHT;

  // Usa o menor valor para manter células quadradas
  cell_size = (max_cell_width < max_cell_height) ? max_cell_width : max_cell_height;

  // Calcula a área útil do jogo
  int game_width = MATRIX_WIDTH * cell_size;
  int game_height = MATRIX_HEIGHT * cell_size;

  // Colocando no struct as coordenadas de onde será feita a área útil
  game_viewport.x = (WINDOW_WIDTH - game_width) / 2;
  game_viewport.y = (WINDOW_HEIGHT - game_height) / 2;

  // Colocando o tamanho da tela útil no struct
  game_viewport.w = game_width;
  game_viewport.h = game_height;
}
// Função para carregar os sprites
void load_textures(SDL_Renderer* renderer) {
  // Carrega a textura da cabeça da cobra
  SDL_Surface* surface = IMG_Load("assets/CabecaBaixo.png");
  if (!surface) {
      fprintf(stderr, "Erro ao carregar CabecaBaixo.png: %s\n", IMG_GetError());
      return;
  }
  CabecaBaixo_texture = SDL_CreateTextureFromSurface(renderer, surface);
  SDL_FreeSurface(surface);

  surface = IMG_Load("assets/CabecaCima.png");
  if (!surface) {
      fprintf(stderr, "Erro ao carregar CabecaCima.png: %s\n", IMG_GetError());
      return;
  }
  CabecaCima_texture = SDL_CreateTextureFromSurface(renderer, surface);
  SDL_FreeSurface(surface);

  surface = IMG_Load("assets/CabecaDireita.png");
  if (!surface) {
      fprintf(stderr, "Erro ao carregar CabecaDireita.png: %s\n", IMG_GetError());
      return;
  }
  CabecaDireita_texture = SDL_CreateTextureFromSurface(renderer, surface);
  SDL_FreeSurface(surface);

  surface = IMG_Load("assets/CabecaEsquerda.png");
  if (!surface) {
      fprintf(stderr, "Erro ao carregar CabecaEsquerda.png: %s\n", IMG_GetError());
      return;
  }
  CabecaEsquerda_texture = SDL_CreateTextureFromSurface(renderer, surface);
  SDL_FreeSurface(surface);

  // Carrega Cauda
  surface = IMG_Load("assets/CaudaBaixo.png");
  if (!surface) {
      fprintf(stderr, "Erro ao carregar CaudaBaixo.png: %s\n", IMG_GetError());
      return;
  }
  CaudaBaixo_texture = SDL_CreateTextureFromSurface(renderer, surface);
  SDL_FreeSurface(surface);

  surface = IMG_Load("assets/CaudaCima.png");
  if (!surface) {
      fprintf(stderr, "Erro ao carregar CaudaCima.png: %s\n", IMG_GetError());
      return;
  }
  CaudaCima_texture = SDL_CreateTextureFromSurface(renderer, surface);
  SDL_FreeSurface(surface);

  surface = IMG_Load("assets/CaudaDireita.png");
  if (!surface) {
      fprintf(stderr, "Erro ao carregar CaudaDireita.png: %s\n", IMG_GetError());
      return;
  }
  CaudaDireita_texture = SDL_CreateTextureFromSurface(renderer, surface);
  SDL_FreeSurface(surface);

  surface = IMG_Load("assets/CaudaEsquerda.png");
  if (!surface) {
      fprintf(stderr, "Erro ao carregar CaudaEsquerda.png: %s\n", IMG_GetError());
      return;
  }
  CaudaEsquerda_texture = SDL_CreateTextureFromSurface(renderer, surface);
  SDL_FreeSurface(surface);

  // Curvas
  surface = IMG_Load("assets/CurvaBaixoDireita.png");
  if (!surface) {
      fprintf(stderr, "Erro ao carregar CurvaBaixoDireita.png: %s\n", IMG_GetError());
      return;
  }
  CurvaBaixoDireita_texture = SDL_CreateTextureFromSurface(renderer, surface);
  SDL_FreeSurface(surface);

  surface = IMG_Load("assets/CurvaBaixoEsquerda.png");
  if (!surface) {
      fprintf(stderr, "Erro ao carregar CurvaBaixoEsquerda.png: %s\n", IMG_GetError());
      return;
  }
  CurvaBaixoEsquerda_texture = SDL_CreateTextureFromSurface(renderer, surface);
  SDL_FreeSurface(surface);

  surface = IMG_Load("assets/CurvaCimaDireita.png");
  if (!surface) {
      fprintf(stderr, "Erro ao carregar CurvaCimaDireita.png: %s\n", IMG_GetError());
      return;
  }
  CurvaCimaDireita_texture = SDL_CreateTextureFromSurface(renderer, surface);
  SDL_FreeSurface(surface);

  surface = IMG_Load("assets/CurvaCimaEsquerda.png");
  if (!surface) {
      fprintf(stderr, "Erro ao carregar CurvaCimaEsquerda.png: %s\n", IMG_GetError());
      return;
  }
  CurvaCimaEsquerda_texture = SDL_CreateTextureFromSurface(renderer, surface);
  SDL_FreeSurface(surface);

  //Reto
  surface = IMG_Load("assets/RetoDireitaEsquerda.png");
  if (!surface) {
      fprintf(stderr, "Erro ao carregar RetoDireitaEsquerda.png: %s\n", IMG_GetError());
      return;
  }
  retoDireitaEsquerda_texture = SDL_CreateTextureFromSurface(renderer, surface);
  SDL_FreeSurface(surface);

  surface = IMG_Load("assets/RetoEsquerdaDireita.png");
  if (!surface) {
      fprintf(stderr, "Erro ao carregar RetoEsquerdaDireita.png: %s\n", IMG_GetError());
      return;
  }
  retoEsquerdaDireita_texture = SDL_CreateTextureFromSurface(renderer, surface);
  SDL_FreeSurface(surface);

  surface = IMG_Load("assets/RetoVertical.png");
  if (!surface) {
      fprintf(stderr, "Erro ao carregar RetoVertical.png: %s\n", IMG_GetError());
      return;
  }
  retoVertical_texture = SDL_CreateTextureFromSurface(renderer, surface);
  SDL_FreeSurface(surface);

  // Frutas
  surface = IMG_Load("assets/limao.png");
  if (!surface) {
      fprintf(stderr, "Erro ao carregar limao.png: %s\n", IMG_GetError());
      return;
  }
  limao_texture = SDL_CreateTextureFromSurface(renderer, surface);
  SDL_FreeSurface(surface);

  surface = IMG_Load("assets/manga.png");
  if (!surface) {
      fprintf(stderr, "Erro ao carregar manga.png: %s\n", IMG_GetError());
      return;
  }
  manga_texture = SDL_CreateTextureFromSurface(renderer, surface);
  SDL_FreeSurface(surface);

  surface = IMG_Load("assets/pitu.png");
  if (!surface) {
      fprintf(stderr, "Erro ao carregar pitu.png: %s\n", IMG_GetError());
      return;
  }
  pitu_texture = SDL_CreateTextureFromSurface(renderer, surface);
  SDL_FreeSurface(surface);

  surface = IMG_Load("assets/caju.png");
  if (!surface) {
      fprintf(stderr, "Erro ao carregar caju.png: %s\n", IMG_GetError());
      return;
  }
  caju_texture = SDL_CreateTextureFromSurface(renderer, surface);
  SDL_FreeSurface(surface);

  // Carrega a textura do Background
  surface = IMG_Load("assets/background.png");
  if (!surface) {
      fprintf(stderr, "Erro ao carregar background.png: %s\n", IMG_GetError());
      return;
  }
  background_texture = SDL_CreateTextureFromSurface(renderer, surface);
  SDL_FreeSurface(surface);

  // Carrega a textura do Menu (TROCAR AQUI PARA MODIFICAR A TELA INICIAL)
  surface = IMG_Load("assets/menu.png");
  if (!surface) {
      fprintf(stderr, "Erro ao carregar menu.png: %s\n", IMG_GetError());
      return;
  }
  menu_texture = SDL_CreateTextureFromSurface(renderer, surface);
  SDL_FreeSurface(surface);

  // Carrega a textura da Splash Screen
  surface = IMG_Load("assets/mangabyte.png");
  if (!surface) {
      fprintf(stderr, "Erro ao carregar mangabyte.png: %s\n", IMG_GetError());
      return;
  }
  mangabyte_texture = SDL_CreateTextureFromSurface(renderer, surface);
  SDL_FreeSurface(surface);

  // Carrega a textura da Tela de Game Over
  surface = IMG_Load("assets/gameover.png");
  if (!surface) {
      fprintf(stderr, "Erro ao carregar gameover.png: %s\n", IMG_GetError());
      return;
  }
  gameover_texture = SDL_CreateTextureFromSurface(renderer, surface);
  SDL_FreeSurface(surface);
}
void cleanup_textures() {
  if(CabecaBaixo_texture) SDL_DestroyTexture(CabecaBaixo_texture);
  if(CabecaCima_texture) SDL_DestroyTexture(CabecaCima_texture);
  if(CabecaDireita_texture) SDL_DestroyTexture(CabecaDireita_texture);
  if(CabecaEsquerda_texture) SDL_DestroyTexture(CabecaEsquerda_texture);
  if(CaudaBaixo_texture) SDL_DestroyTexture(CaudaBaixo_texture);
  if(CaudaCima_texture) SDL_DestroyTexture(CaudaCima_texture);
  if(CaudaDireita_texture) SDL_DestroyTexture(CaudaDireita_texture);
  if(CaudaEsquerda_texture) SDL_DestroyTexture(CaudaEsquerda_texture);
  if(CurvaBaixoDireita_texture) SDL_DestroyTexture(CurvaBaixoDireita_texture);
  if(CurvaBaixoEsquerda_texture) SDL_DestroyTexture(CurvaBaixoEsquerda_texture);
  if(CurvaCimaDireita_texture) SDL_DestroyTexture(CurvaCimaDireita_texture);
  if(CurvaCimaEsquerda_texture) SDL_DestroyTexture(CurvaCimaEsquerda_texture);
  if(retoDireitaEsquerda_texture) SDL_DestroyTexture(retoDireitaEsquerda_texture);
  if(retoEsquerdaDireita_texture) SDL_DestroyTexture(retoEsquerdaDireita_texture);
  if(retoVertical_texture) SDL_DestroyTexture(retoVertical_texture);
  if(pitu_texture) SDL_DestroyTexture(pitu_texture);
  if(manga_texture) SDL_DestroyTexture(manga_texture);
  if(caju_texture) SDL_DestroyTexture(caju_texture);
  if(limao_texture) SDL_DestroyTexture(limao_texture);
  if(background_texture) SDL_DestroyTexture(background_texture);
  if(menu_texture) SDL_DestroyTexture(menu_texture);
  if(mangabyte_texture) SDL_DestroyTexture(mangabyte_texture);
  system ("mplayer fecharjogo.mp3 &");//tocar ao fechar o jogo 
  if(gameover_texture) SDL_DestroyTexture(gameover_texture);
  if (game_font) {
    TTF_CloseFont(game_font);
    game_font = NULL;
}
TTF_Quit();

  IMG_Quit();
}
// Definição de uma função que dá as específicações
// de desenho de um retângulo
SDL_Rect rectFromCellPos(int cell_posX, int cell_posY) {
  return (SDL_Rect){
      cell_posX * cell_size,
      (MATRIX_HEIGHT - 1 - cell_posY) * cell_size,  // Inverte Y
      cell_size,
      cell_size
  };
}

// --Game Loop--

void setup()
{
  srand(SDL_GetTicks()); // iniciando número aleatório para geração de fruta
  initialize_viewport();
  // Inicializa SDL_image
  int imgFlags = IMG_INIT_PNG;
  if (!(IMG_Init(imgFlags) & imgFlags)) {
      fprintf(stderr, "SDL_image não pôde inicializar! SDL_image Error: %s\n", IMG_GetError());
      game_is_running = FALSE;
      return;
  }
  //Pegando o tick inicial, como um "millis" do sdl
  last_update_time = SDL_GetTicks();

  // Iniciando toda a matriz como vazia
  for(int i = 0; i < MATRIX_WIDTH; i++)
  {
    for(int j = 0; j < MATRIX_HEIGHT; j++)
    {
      mapMatrix[i][j].type = EMPTY_TILE;
    }
  }

  // Iniciando posição da cabeça da cobra
  snake_headX = SNAKE_HEADX;
  snake_headY = SNAKE_HEADY;

  // Iniciando posição da cauda da cobra
  snake_tailX = SNAKE_HEADX;
  snake_tailY = SNAKE_HEADY-2;

  // Iniciando as posições iniciais da cobra
  mapMatrix[snake_headX][snake_headY].snake = (snakeTile){SNAKE_TILE,UP};
  mapMatrix[snake_headX][snake_headY-1].snake = (snakeTile){SNAKE_TILE,UP};
  mapMatrix[snake_headX][snake_headY-2].snake = (snakeTile){SNAKE_TILE,UP};

  head_dir = UP; // Coloca direção para cima para garantir que quando o jogo reiniciar pós gameover a direção voltará

  snake_size = INITIAL_SNAKE_SIZE;

  // Criando as primeiras frutas em posições aleatórias
  for (int i = 0; i < INITIAL_FRUIT_QUANTITY; i++) { // i < x = quantas frutas existirão no mapa
    int fruitX, fruitY;
    do {
        fruitX = rand() % MATRIX_WIDTH;
        fruitY = rand() % MATRIX_HEIGHT;
    } while (mapMatrix[fruitX][fruitY].type != EMPTY_TILE);

    mapMatrix[fruitX][fruitY].fruit = (fruitTile){FRUIT_TILE, (char)(rand() % 7)};
  }
  if (TTF_Init() == -1) {
    fprintf(stderr, "Não foi possível inicializar SDL_ttf: %s\n", TTF_GetError());
    game_is_running = FALSE;
    return;
}
game_font = TTF_OpenFont("assets/arial.ttf", 24); // Arial tamanho 24
if (!game_font) {
    fprintf(stderr, "Não foi possível carregar fonte: %s\n", TTF_GetError());
    game_is_running = FALSE;
    return;
}

  load_high_score();
}

// process_input com suporte a teclado (SDL) e controle por câmera (OpenCV)
// Assume que exista uma função/variável para ajustar a direção da cobra.
// Esta implementação tenta detectar símbolos comuns e chama a lógica apropriada.
//
// Se o seu projeto usa nomes diferentes para a variável de direção, ajuste as linhas
// marcadas com "ADAPTAR_AQUI" para corresponder ao seu código.
// process_input com suporte a teclado (SDL) e controle por câmera (OpenCV) - VERSÃO CORRIGIDA
// Adicione estas variáveis globais no topo do arquivo, junto com as outras
SDL_Window* camera_window = NULL;
SDL_Renderer* camera_renderer = NULL;
SDL_Texture* camera_texture = NULL;
bool camera_window_open = false;
int camera_frame_width = 320;
int camera_frame_height = 240;

cv::CascadeClassifier face_cascade;
bool face_cascade_loaded = false;

// Função para carregar o classificador de faces
bool load_face_cascade() {
    if (face_cascade_loaded) return true;
    
    // Tenta carregar o classificador Haar para faces
    std::string cascade_paths[] = {
        "/usr/share/opencv4/haarcascades/haarcascade_frontalface_alt.xml",
        "/usr/local/share/opencv4/haarcascades/haarcascade_frontalface_alt.xml",
        "haarcascade_frontalface_alt.xml",
        "/usr/share/OpenCV/haarcascades/haarcascade_frontalface_alt.xml",
        "/opt/opencv/share/opencv4/haarcascades/haarcascade_frontalface_alt.xml"
    };
    
    for (const auto& path : cascade_paths) {
        if (face_cascade.load(path)) {
            printf("Classificador de faces carregado: %s\n", path.c_str());
            face_cascade_loaded = true;
            return true;
        }
    }
    
    printf("ERRO: Não foi possível carregar o classificador de faces!\n");
    printf("Instale com: sudo apt-get install libopencv-contrib-dev\n");
    printf("Ou baixe haarcascade_frontalface_alt.xml do repositório OpenCV\n");
    return false;
}

// Função para inicializar a janela da câmera
bool initialize_camera_window() {
    if (camera_window_open) return true;
    
    // Criar janela da câmera
    camera_window = SDL_CreateWindow(
        "Snake Game - Camera Feed",
        SDL_WINDOWPOS_UNDEFINED + WINDOW_WIDTH + 50, // Posiciona ao lado da janela principal
        SDL_WINDOWPOS_UNDEFINED,
        camera_frame_width,
        camera_frame_height,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );
    
    if (!camera_window) {
        printf("Erro ao criar janela da câmera: %s\n", SDL_GetError());
        return false;
    }
    
    // Criar renderer para a janela da câmera
    camera_renderer = SDL_CreateRenderer(
        camera_window, 
        -1, 
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );
    
    if (!camera_renderer) {
        printf("Erro ao criar renderer da câmera: %s\n", SDL_GetError());
        SDL_DestroyWindow(camera_window);
        camera_window = NULL;
        return false;
    }
    
    // Criar textura para o frame da câmera (formato BGR para OpenCV)
    camera_texture = SDL_CreateTexture(
        camera_renderer,
        SDL_PIXELFORMAT_BGR24,
        SDL_TEXTUREACCESS_STREAMING,
        camera_frame_width,
        camera_frame_height
    );
    
    if (!camera_texture) {
        printf("Erro ao criar textura da câmera: %s\n", SDL_GetError());
        SDL_DestroyRenderer(camera_renderer);
        SDL_DestroyWindow(camera_window);
        camera_renderer = NULL;
        camera_window = NULL;
        return false;
    }
    
    camera_window_open = true;
    printf("Janela da câmera criada com sucesso!\n");
    return true;
}

// Função para fechar a janela da câmera
void cleanup_camera_window() {
    if (camera_texture) {
        SDL_DestroyTexture(camera_texture);
        camera_texture = NULL;
    }
    if (camera_renderer) {
        SDL_DestroyRenderer(camera_renderer);
        camera_renderer = NULL;
    }
    if (camera_window) {
        SDL_DestroyWindow(camera_window);
        camera_window = NULL;
    }
    camera_window_open = false;
    printf("Janela da câmera fechada.\n");
}

// Função para renderizar o frame da câmera na janela separada
void render_camera_frame(const cv::Mat& frame, const cv::Mat& mask) {
    if (!camera_window_open || !camera_texture || frame.empty()) return;
    
    // Redimensiona o frame se necessário
    cv::Mat display_frame;
    if (frame.cols != camera_frame_width || frame.rows != camera_frame_height) {
        cv::resize(frame, display_frame, cv::Size(camera_frame_width, camera_frame_height));
    } else {
        display_frame = frame.clone();
    }
    flip(display_frame, display_frame, 1);
    
    // Detectar faces se o classificador estiver carregado
    if (face_cascade_loaded) {
        cv::Mat gray;
        cv::cvtColor(display_frame, gray, cv::COLOR_BGR2GRAY);
        cv::equalizeHist(gray, gray);
        
        std::vector<cv::Rect> faces;
        face_cascade.detectMultiScale(gray, faces, 1.1, 3, 0, cv::Size(30, 30));
        
        // Desenhar faces detectadas
        for (const auto& face : faces) {
            cv::rectangle(display_frame, face, cv::Scalar(0, 255, 0), 3);
            
            // Desenhar centro da face
            cv::Point face_center(face.x + face.width/2, face.y + face.height/2);
            cv::circle(display_frame, face_center, 5, cv::Scalar(0, 0, 255), -1);
            
            // Determinar direção baseada na posição da face
            int w = display_frame.cols;
            int h = display_frame.rows;
            
            std::string direction_text;
            if (face_center.y < h / 3) {
                direction_text = "UP";
            } else if (face_center.y > 2 * h / 3) {
                direction_text = "DOWN";
            } else if (face_center.x < w / 3) {
                direction_text = "LEFT";
            } else if (face_center.x > 2 * w / 3) {
                direction_text = "RIGHT";
            } else {
                direction_text = "CENTER";
            }
            
            // Texto indicando direção detectada
            cv::putText(display_frame, direction_text, 
                       cv::Point(10, 30), cv::FONT_HERSHEY_SIMPLEX, 1, 
                       cv::Scalar(0, 255, 255), 2);
            
            // Mostrar área da face
            cv::putText(display_frame, "Face: " + std::to_string((int)(face.width * face.height)), 
                       cv::Point(10, 60), cv::FONT_HERSHEY_SIMPLEX, 0.5, 
                       cv::Scalar(255, 255, 255), 1);
        }
        
        // Desenhar zonas de controle
        int w = display_frame.cols;
        int h = display_frame.rows;
        
        // Linhas divisórias das zonas (mais visíveis)
        cv::line(display_frame, cv::Point(0, h/3), cv::Point(w, h/3), cv::Scalar(255, 255, 0), 2);
        cv::line(display_frame, cv::Point(0, 2*h/3), cv::Point(w, 2*h/3), cv::Scalar(255, 255, 0), 2);
        cv::line(display_frame, cv::Point(w/3, 0), cv::Point(w/3, h), cv::Scalar(255, 255, 0), 2);
        cv::line(display_frame, cv::Point(2*w/3, 0), cv::Point(2*w/3, h), cv::Scalar(255, 255, 0), 2);
        
        // Labels das zonas
        cv::putText(display_frame, "UP", cv::Point(w/2-15, 25), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255, 255, 255), 2);
        cv::putText(display_frame, "DOWN", cv::Point(w/2-25, h-10), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255, 255, 255), 2);
        cv::putText(display_frame, "LEFT", cv::Point(5, h/2), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255, 255, 255), 2);
        cv::putText(display_frame, "RIGHT", cv::Point(w-60, h/2), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255, 255, 255), 2);
    }
    
    // Adicionar texto de instruções
    cv::putText(display_frame, "Controle com o rosto", 
               cv::Point(10, camera_frame_height - 40), cv::FONT_HERSHEY_SIMPLEX, 0.5, 
               cv::Scalar(255, 255, 255), 1);
    cv::putText(display_frame, "Pressione 'V' para fechar", 
               cv::Point(10, camera_frame_height - 20), cv::FONT_HERSHEY_SIMPLEX, 0.5, 
               cv::Scalar(255, 255, 255), 1);
    
    // Atualizar textura SDL
    void* pixels;
    int pitch;
    SDL_LockTexture(camera_texture, NULL, &pixels, &pitch);
    memcpy(pixels, display_frame.data, display_frame.total() * display_frame.elemSize());
    SDL_UnlockTexture(camera_texture);
    
    // Renderizar na janela da câmera
    SDL_SetRenderDrawColor(camera_renderer, 0, 0, 0, 255);
    SDL_RenderClear(camera_renderer);
    SDL_RenderCopy(camera_renderer, camera_texture, NULL, NULL);
    SDL_RenderPresent(camera_renderer);
}

// Função process_input modificada com suporte à janela da câmera
// Função process_input modificada com detecção facial
void process_input()
{
    SDL_Event event;
    
    // ===== Entrada via teclado =====
    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_QUIT)
        {
            // Verifica qual janela foi fechada
            if (camera_window && event.window.windowID == SDL_GetWindowID(camera_window)) {
                cleanup_camera_window();
            } else {
                game_is_running = FALSE;
            }
            return;
        }
        
        if (event.type == SDL_KEYDOWN)
        {
            switch (event.key.keysym.sym)
            {
                case SDLK_UP:
                    if (game_state == GAME_STATE_PLAYING && head_dir != DOWN) head_dir = UP;
                    break;
                case SDLK_DOWN:
                    if (game_state == GAME_STATE_PLAYING && head_dir != UP) head_dir = DOWN;
                    break;
                case SDLK_LEFT:
                    if (game_state == GAME_STATE_PLAYING && head_dir != RIGHT) head_dir = LEFT;
                    break;
                case SDLK_RIGHT:
                    if (game_state == GAME_STATE_PLAYING && head_dir != LEFT) head_dir = RIGHT;
                    break;
                case SDLK_RETURN:
                    if (game_state == GAME_STATE_MENU) {
                        game_state = GAME_STATE_PLAYING;
                        system("mplayer iniciando.mp3 &");// tocar som ao iniciar o jogo
                    } else if (game_state == GAME_STATE_GAMEOVER) {
                        setup();
                        system ("mplayer reiniciando.mp3 &");// tocar som ao reiniciar o jogo
                        game_state = GAME_STATE_PLAYING;
                    }
                    break;
                case SDLK_v:
                    // Tecla 'V' para abrir/fechar janela da câmera
                    if (camera_window_open) {
                        cleanup_camera_window();
                    } else {
                        if (initialize_camera_window()) {
                            load_face_cascade(); // Carrega o classificador ao abrir a janela
                        }
                    }
                    break;
                case SDLK_c:
                    // Tecla 'C' para reconectar câmera
                    if (g_cv_initialized) {
                        g_cap.release();
                        g_cv_initialized = false;
                        printf("Câmera reiniciada. Tentando reconectar...\n");
                    }
                    break;
                case SDLK_r:
                    // Tecla 'R' para reiniciar jogo manualmente
                    if (game_state == GAME_STATE_PLAYING || game_state == GAME_STATE_GAMEOVER) {
                        setup();
                        game_state = GAME_STATE_PLAYING;
                        printf("Jogo reiniciado manualmente\n");
                    }
                    break;
                case SDLK_ESCAPE:
                    game_is_running = FALSE;
                    return;
                default:
                    break;
            }
        }
    }

    // ===== Entrada via câmera com detecção facial =====
    if (!g_cv_initialized)
    {
        printf("Tentando inicializar a câmera...\n");
        
        // Desabilita backends problemáticos
        setenv("OPENCV_VIDEOIO_PRIORITY_GSTREAMER", "0", 1);
        
        int backends[] = {
            cv::CAP_V4L2,
            cv::CAP_FFMPEG,
            cv::CAP_ANY
        };
        
        bool camera_opened = false;
        for (int i = 0; i < 3; i++) {
            printf("Tentando backend %d...\n", backends[i]);
            g_cap.open(0, backends[i]);
            
            if (g_cap.isOpened()) {
                printf("Câmera aberta com sucesso usando backend %d!\n", backends[i]);
                camera_opened = true;
                break;
            } else {
                printf("Falhou com backend %d\n", backends[i]);
            }
        }
        
        if (!camera_opened) {
            printf("ERRO: Não foi possível abrir a câmera.\n");
            printf("Pressione 'V' para abrir a janela da câmera (quando disponível)\n");
            g_cv_initialized = true;
            return;
        }
        
        // Configurações da câmera
        g_cap.set(cv::CAP_PROP_FRAME_WIDTH, camera_frame_width);
        g_cap.set(cv::CAP_PROP_FRAME_HEIGHT, camera_frame_height);
        g_cap.set(cv::CAP_PROP_FPS, 30);
        g_cap.set(cv::CAP_PROP_BUFFERSIZE, 1); // Reduz buffer para menos latência
        
        // Atualiza o tamanho real obtido da câmera
        camera_frame_width = (int)g_cap.get(cv::CAP_PROP_FRAME_WIDTH);
        camera_frame_height = (int)g_cap.get(cv::CAP_PROP_FRAME_HEIGHT);
        
        printf("Câmera configurada! Resolução: %dx%d\n", camera_frame_width, camera_frame_height);
        printf("Pressione 'V' para abrir/fechar a janela da câmera\n");
        
        g_cv_initialized = true;
        
        // Abre janela automaticamente e carrega classificador
        if (initialize_camera_window()) {
            load_face_cascade();
        }
    }

    if (!g_cap.isOpened()) return;

    cv::Mat frame;
    
    // Captura frame com retry
    bool frame_captured = false;
    for (int attempts = 0; attempts < 3; attempts++) {
        g_cap >> frame;
        if (!frame.empty()) {
            frame_captured = true;
            break;
        }
        SDL_Delay(5);
    }
    
    if (!frame_captured) {
        return;
    }

    // Renderizar frame da câmera (sem máscara pois não usamos mais detecção de cor)
    cv::Mat empty_mask;
    render_camera_frame(frame, empty_mask);

    // Detectar faces para controlar a cobra (apenas durante o jogo)
    if (game_state == GAME_STATE_PLAYING && face_cascade_loaded) {
        cv::Mat gray;
        cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
        cv::equalizeHist(gray, gray);
        
        std::vector<cv::Rect> faces;
        face_cascade.detectMultiScale(gray, faces, 1.1, 3, 0, cv::Size(30, 30));

        if (!faces.empty()) {
            // Usar a maior face detectada
            cv::Rect largest_face = *std::max_element(faces.begin(), faces.end(),
                [](const cv::Rect& a, const cv::Rect& b) {
                    return (a.width * a.height) < (b.width * b.height);
                });

            cv::Point face_center(largest_face.x + largest_face.width/2, 
                                 largest_face.y + largest_face.height/2);

            int w = frame.cols;
            int h = frame.rows;

            static direction last_detected_dir = UP;
            static Uint32 last_direction_change = 0;
            Uint32 current_time = SDL_GetTicks();
            
            direction new_dir = last_detected_dir;

            // Determinar direção baseada na posição da face
            if (face_center.y < h / 3 && head_dir != DOWN) {
                new_dir = UP;
            } else if (face_center.y > 2 * h / 3 && head_dir != UP) {
                new_dir = DOWN;
            } else if (face_center.x < w / 3 && head_dir != LEFT) {
                new_dir = RIGHT;
            } else if (face_center.x > 2 * w / 3 && head_dir != RIGHT) {
                new_dir = LEFT;
            }
            
            // Só muda direção se passou tempo suficiente (debounce)
            if (new_dir != last_detected_dir && current_time - last_direction_change > 500) {
                // Evita reversão direta (cobra não pode ir na direção oposta)
                bool valid_direction = true;
                switch (head_dir) {
                    case UP: if (new_dir == DOWN) valid_direction = false; break;
                    case DOWN: if (new_dir == UP) valid_direction = false; break;
                    case LEFT: if (new_dir == RIGHT) valid_direction = false; break;
                    case RIGHT: if (new_dir == LEFT) valid_direction = false; break;
                }
                
                if (valid_direction) {
                    head_dir = new_dir;
                    last_detected_dir = new_dir;
                    last_direction_change = current_time;
                    printf("Direção da face: %d\n", new_dir);
                }
            }
        }
    }
}

// Função de limpeza atualizada
void cleanup_camera() {
    cleanup_camera_window();
    if (g_cap.isOpened()) {
        printf("Fechando câmera...\n");
        g_cap.release();
    }
    cv::destroyAllWindows();
}

// Função update corrigida para evitar game overs espontâneos
void update(SDL_Renderer* renderer) {
    // Só atualiza durante o jogo ou splash screen
    if (game_state != GAME_STATE_PLAYING && game_state != GAME_STATE_SPLASH) return;
    
    // Timer da splash screen
    if (game_state == GAME_STATE_SPLASH) {
        if (slashcount >= 500) {
            game_state = GAME_STATE_MENU;
            return;
        }
        slashcount++;
        return;
    }
    
    // Verifica se já passou o tempo necessário para a próxima atualização
    Uint32 current_time = SDL_GetTicks();
    if (current_time - last_update_time < update_interval) {
        return;
    }
    last_update_time = current_time;
    // Exibir pontuação atual

    // Validação de integridade antes de mover
    if (snake_headX < 0 || snake_headX >= MATRIX_WIDTH || 
        snake_headY < 0 || snake_headY >= MATRIX_HEIGHT ||
        snake_tailX < 0 || snake_tailX >= MATRIX_WIDTH ||
        snake_tailY < 0 || snake_tailY >= MATRIX_HEIGHT) {
        printf("ERRO: Posição da cobra inválida! Head: (%d,%d) Tail: (%d,%d)\n", 
               snake_headX, snake_headY, snake_tailX, snake_tailY);
        setup(); // Reinicia o jogo
        return;
    }
    
    // Calcula nova posição da cabeça
    int new_headX = snake_headX;
    int new_headY = snake_headY;

    switch (head_dir) {
        case UP:
            new_headY++;
            break;
        case DOWN:
            new_headY--;
            break;
        case LEFT:
            new_headX--;
            break;
        case RIGHT:
            new_headX++;
            break;
    }

    // Verifica colisão com parede
    if (new_headX < 0 || new_headX >= MATRIX_WIDTH || 
        new_headY < 0 || new_headY >= MATRIX_HEIGHT) {
        record_score(score);
        printf("Game Over: Colisão com parede\n");
        game_state = GAME_STATE_GAMEOVER;
        system ("mplayer gameover.mp3 &");//tocar som de game over
        return;
    }

    // Verifica se comeu fruta
    bool ate_fruit = false;
    if (mapMatrix[new_headX][new_headY].type == FRUIT_TILE) {
        system("mplayer comendo.mp3 &");//tocar som ao comer fruta
        printf("Fruta comida! Tamanho: %d -> %d\n", snake_size, snake_size + 1);
        printf("Pontuação: %d\n", snake_size - 2);
        printrecord();
        snake_size++;
        score++;
        ate_fruit = true;

        // Remove a fruta comida
        mapMatrix[new_headX][new_headY].type = EMPTY_TILE;

        // Gera nova fruta
        int fruitX, fruitY;
        int attempts = 0;
        do {
            fruitX = rand() % MATRIX_WIDTH;
            fruitY = rand() % MATRIX_HEIGHT;
            attempts++;
        } while (mapMatrix[fruitX][fruitY].type != EMPTY_TILE && attempts < 100);
        
        if (attempts < 100) {
            mapMatrix[fruitX][fruitY].fruit = (fruitTile){FRUIT_TILE, (char)(rand() % 4)};
        }
    }

    // Se não comeu fruta, move a cauda
    if (!ate_fruit) {
        // Validação da cauda
        if (mapMatrix[snake_tailX][snake_tailY].type != SNAKE_TILE) {
            printf("ERRO: Cauda não encontrada em (%d,%d)!\n", snake_tailX, snake_tailY);
            setup();
            return;
        }
        
        // Encontra próxima posição da cauda
        int next_tailX = snake_tailX;
        int next_tailY = snake_tailY;

        switch (mapMatrix[snake_tailX][snake_tailY].snake.forwardDirection) {
            case UP:
                next_tailY++;
                break;
            case DOWN:
                next_tailY--;
                break;
            case LEFT:
                next_tailX--;
                break;
            case RIGHT:
                next_tailX++;
                break;
        }

        // Limpa posição antiga da cauda
        mapMatrix[snake_tailX][snake_tailY].type = EMPTY_TILE;

        // Atualiza posição da cauda
        snake_tailX = next_tailX;
        snake_tailY = next_tailY;
    }

    // Verifica colisão com próprio corpo (após mover a cauda)
    if (mapMatrix[new_headX][new_headY].type == SNAKE_TILE) {
        printf("Game Over: Colisão com próprio corpo\n");
        game_state = GAME_STATE_GAMEOVER;
        return;
    }

    // Atualiza posição da cabeça
    mapMatrix[snake_headX][snake_headY].snake.forwardDirection = head_dir;
    mapMatrix[new_headX][new_headY].snake = (snakeTile){SNAKE_TILE, head_dir};
    snake_headX = new_headX;
    snake_headY = new_headY;
}

void render(SDL_Renderer* renderer) {
  // Limpa toda a tela com preto
  SDL_SetRenderDrawColor(renderer, BLACK);
  SDL_RenderClear(renderer);

  // Define a área de renderização do jogo
  SDL_RenderSetViewport(renderer, &game_viewport);
  if (game_state==GAME_STATE_SPLASH){
    SDL_Rect screen_rect = {
      0,
      0,
      WINDOW_HEIGHT,
      WINDOW_HEIGHT
    };
    SDL_RenderCopy(renderer, mangabyte_texture, NULL, &screen_rect);
  }
  else if (game_state==GAME_STATE_MENU){
    SDL_Rect screen_rect = {
    0,
    0,
    WINDOW_HEIGHT,
    WINDOW_HEIGHT
  };
  SDL_RenderCopy(renderer, menu_texture, NULL, &screen_rect);}
  else if (game_state==GAME_STATE_GAMEOVER){
    SDL_Rect screen_rect = {
    0,
    0,
    WINDOW_HEIGHT,
    WINDOW_HEIGHT
  };
  SDL_RenderCopy(renderer, gameover_texture, NULL, &screen_rect);}
  else if (game_state==GAME_STATE_PLAYING){
  // ======= LAYERS (Camadas de renderização) =======

  /* Layers não são literalmente programadas, mas por consequência da dinâmica,
   uma render a frente se sobrepõe a uma anterior, portanto a separação em camadas
   é puramente para organização!
  */

  // ===== Layer 0 =====

  // Renderiza o background
  for(int x = 0; x < MATRIX_WIDTH; x++) {
    for(int y = 0; y < MATRIX_HEIGHT; y++) {
        SDL_Rect dest_rect = rectFromCellPos(x, y);
        SDL_RenderCopy(renderer, background_texture, NULL, &dest_rect);
    }
}

  // ====== Layer 1 ======

  // Renderiza a cobra
int cell_posX = snake_tailX;
int cell_posY = snake_tailY;
int prev_direction = -1; // Direção do segmento anterior

for(int i = 0; i < snake_size; i++) {
    if(mapMatrix[cell_posX][cell_posY].type != SNAKE_TILE) {
        fprintf(stderr, "Erro: Segmento de cobra faltando em [%d][%d]\n", cell_posX, cell_posY);
        game_is_running = FALSE;
        break;
    }

    SDL_Rect dest_rect = rectFromCellPos(cell_posX, cell_posY);

    // Determina o tipo de segmento
    SegmentType seg_type = determine_segment_type(
        cell_posX, cell_posY,
        (i == snake_size-1), // Verifica se o segmento é uma cabeça
        (i == 0),            // Verifica se o segmento é uma cauda
        prev_direction       // Direção do segmento anterior
    );
      SDL_Texture* to_render = NULL;
      // Renderiza o segmento apropriado
      switch(seg_type) {
        case SEGMENT_HEAD:
        {
          switch(mapMatrix[cell_posX][cell_posY].snake.forwardDirection)
          {
            case UP: to_render = CabecaBaixo_texture; break;
            case RIGHT: to_render = CabecaEsquerda_texture; break;
            case DOWN: to_render = CabecaCima_texture; break;
            case LEFT: to_render = CabecaDireita_texture; break;
          }
          break;
        }
        case SEGMENT_TAIL: {
            // Encontre a direção da cauda (oposto da direção do segmento anterior)
            direction tail_dir = mapMatrix[cell_posX][cell_posY].snake.forwardDirection;

            switch(tail_dir)
            {
              case UP: to_render = CaudaCima_texture; break;
              case RIGHT: to_render = CaudaDireita_texture; break;
              case DOWN: to_render = CaudaBaixo_texture; break;
              case LEFT: to_render = CaudaEsquerda_texture; break;
            }
            break;
        }
        case SEGMENT_CURVE:
          {
            switch(mapMatrix[cell_posX][cell_posY].snake.forwardDirection){
              case UP: // Direita - Cima
                to_render = prev_direction == LEFT ? CurvaCimaDireita_texture : CurvaCimaEsquerda_texture;
                break;
              case DOWN: // Esquerda - Cima
                to_render = prev_direction == LEFT ? CurvaBaixoDireita_texture : CurvaBaixoEsquerda_texture;
                break;
              case LEFT: // Direita - Baixo
                to_render = prev_direction == DOWN ? CurvaCimaEsquerda_texture : CurvaBaixoEsquerda_texture;
                break;
              case RIGHT: // Esquerda - Baixo
                to_render = prev_direction == DOWN ? CurvaCimaDireita_texture : CurvaBaixoDireita_texture;
                break;
            }
      break;
      }
        case SEGMENT_STRAIGHT: {
            switch(mapMatrix[cell_posX][cell_posY].snake.forwardDirection) {
                case UP: to_render = retoVertical_texture; break;
                case RIGHT: to_render = retoEsquerdaDireita_texture; break;
                case DOWN: to_render = retoVertical_texture; break;
                case LEFT: to_render = retoDireitaEsquerda_texture; break;
            }
            break;
        }

    }
    SDL_RenderCopyEx(renderer, to_render, NULL, &dest_rect, 0, NULL, SDL_FLIP_NONE);


    // Atualiza a direção anterior para o próximo segmento
    prev_direction = mapMatrix[cell_posX][cell_posY].snake.forwardDirection;

    // Move para o próximo segmento
    switch (mapMatrix[cell_posX][cell_posY].snake.forwardDirection) {
    case UP: cell_posY++; break;
    case DOWN: cell_posY--; break;
    case LEFT: cell_posX--; break;
    case RIGHT: cell_posX++; break;
  }
  }

  // Renderiza as frutas
  for(int x = 0; x < MATRIX_WIDTH; x++) {
      for(int y = 0; y < MATRIX_HEIGHT; y++) {
          if(mapMatrix[x][y].type == FRUIT_TILE)
          {
            SDL_Texture* to_render = NULL;
            switch (mapMatrix[x][y].fruit.sprite)
            {
              case 0:
              case 1:
                to_render = manga_texture;
                break;
              case 2:
              case 3:
                to_render = limao_texture;
                break;
              case 4:
              case 5:
                to_render = caju_texture;
                break;
              case 6:
                to_render = pitu_texture;
                break;

            }
            SDL_Rect dest_rect = rectFromCellPos(x, y);
            SDL_RenderCopy(renderer, to_render, NULL, &dest_rect);
          }
      }
  }
        char score_text[50];
        sprintf(score_text, "Score %d", score);
        SDL_Color white = {0, 0, 0, 255};
        render_text(renderer, score_text, 10, 10, white);

        // Exibir record
        int highscore = load_high_score(); // Função que você já tem
        char highscore_text[50];
        sprintf(highscore_text, "Record %d", highscore);
        render_text(renderer, highscore_text, 10, 40, white);

}

  // Restaura o viewport padrão para a janela toda
  SDL_RenderSetViewport(renderer, NULL);
  SDL_RenderPresent(renderer);
}


//Funções para pontuação

void save_score(int score) {
    FILE* file = fopen("highscore.txt", "w");
    if (file) {
        fprintf(file, "%d\n", score);
        fclose(file);
    } else {
        fprintf(stderr, "Erro ao salvar pontuação!\n");
    }
}
int load_high_score() {
    FILE* file = fopen("highscore.txt", "r");
    int high_score = 0;
    if (file) {
        fscanf(file, "%d", &high_score);
        fclose(file);
    }
    return high_score;
}
void record_score(int score) {
    int high_score = load_high_score();
    if (score > high_score) {
        save_score(score);
        printf("Novo recorde! Pontuação: %d\n", score);
    } else {
        printf("Pontuação final: %d. Recorde atual: %d\n", score, high_score);
    }
}
void printrecord(){
    printf("Record atual: %d\n", load_high_score());
}
void render_text(SDL_Renderer* renderer, const char* text, int x, int y, SDL_Color color) {
    if (!game_font) return;

    SDL_Surface* surface = TTF_RenderText_Blended(game_font, text, color);
    if (!surface) {
        fprintf(stderr, "Erro ao criar surface do texto: %s\n", TTF_GetError());
        return;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!texture) {
        fprintf(stderr, "Erro ao criar textura do texto: %s\n", SDL_GetError());
        SDL_FreeSurface(surface);
        return;
    }

    SDL_Rect dst_rect = {x, y, surface->w, surface->h};
    SDL_RenderCopy(renderer, texture, NULL, &dst_rect);

    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}