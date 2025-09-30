#include "./game.h"
#include "./windowManager.h"
#include "./constants.h"

// Variável global para controlar o loop do jogo
int game_is_running = FALSE;

int main(int argc, char** argv)
{
    // Desabilita GStreamer se estiver causando problemas
    setenv("OPENCV_VIDEOIO_PRIORITY_GSTREAMER", "0", 1);

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
        update(renderer);  // Atualiza lógica do jogo (movimento, colisão, etc)
        render(renderer);  // Desenha tudo na tela
    }

    // Libera recursos usados pelas texturas
    cleanup_textures();
    cleanup_camera();

    // Destroi a janela e encerra SDL
    destroy_window();
    printrecord();

    return 0;
}
