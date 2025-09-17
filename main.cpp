#include "./game.h"
#include "./windowManager.h"
#include "./constants.h"

// Variável global para controlar o loop do jogo
int game_is_running = FALSE;

int main(int argc, char** argv)
{
    // Inicializa a janela e o renderer SDL
    game_is_running = initialize_window();

    // Carrega as texturas necessárias para o jogo
    load_textures(renderer);

    // Inicializa variáveis e estado do jogo
    setup();

    // Loop principal do jogo
    while (game_is_running)
    {
        process_input();   // Lê teclado e câmera (OpenCV)
        update();          // Atualiza lógica do jogo (movimento, colisão, etc)
        render(renderer);  // Desenha tudo na tela
    }

    // Libera recursos usados pelas texturas
    cleanup_textures();

    // Destroi a janela e encerra SDL
    destroy_window();

    return 0;
}
