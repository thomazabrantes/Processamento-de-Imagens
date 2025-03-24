#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h> // Para usar strings

#ifdef WIN32
#include <windows.h> // Apenas para Windows
#endif

#ifdef __APPLE__
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <GLUT/glut.h>
#else
#include <GL/gl.h>   // Funções da OpenGL
#include <GL/glu.h>  // Funções da GLU
#include <GL/glut.h> // Funções da FreeGLUT
#endif

// SOIL é a biblioteca para leitura das imagens
#include "SOIL.h"

// Um pixel Pixel (24 bits)
typedef struct
{
    unsigned char r, g, b;
} Pixel;

// Uma imagem Pixel
typedef struct
{
    int width, height;
    Pixel *img;
} Img;

// Protótipos
void load(char *name, Img *pic);
void processa();

// Funções da interface gráfica e OpenGL
void init();
void draw();
void keyboard(unsigned char key, int x, int y);

// Largura e altura da janela
int width, height;

// Fator de multiplicação do ruído
int fator;

// Identificadores de textura
GLuint tex[2];

// As 2 imagens
Img pic[2];

// Imagem selecionada (0,1)
int sel;

// Carrega uma imagem para a struct Img
void load(char *name, Img *pic)
{
    int chan;
    pic->img = (Pixel *)SOIL_load_image(name, &pic->width, &pic->height, &chan, SOIL_LOAD_RGB);
    if (!pic->img)
    {
        printf("SOIL loading error: '%s'\n", SOIL_last_result());
        exit(1);
    }
    printf("Load: %d x %d x %d\n", pic->width, pic->height, chan);
}

int main(int argc, char **argv)
{
    if (argc < 1)
    {
        printf("seeing [im. entrada]\n");
        exit(1);
    }
    glutInit(&argc, argv);

    // Define do modo de operacao da GLUT
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);

    // pic[0] -> imagem de entrada
    // pic[1] -> imagem de saida

    // Carrega a imagem
    load(argv[1], &pic[0]);

    fator = 5;
    if (argc == 3)
        fator = atoi(argv[2]);

    width = pic[0].width;
    height = pic[0].height;

    // A largura e altura da imagem de saída são iguais às da imagem de entrada (0)
    pic[1].width = pic[0].width;
    pic[1].height = pic[0].height;
    pic[1].img = calloc(pic[1].width * pic[1].height, 3); // W x H x 3 bytes (Pixel)

    // Especifica o tamanho inicial em pixels da janela GLUT
    glutInitWindowSize(width, height);

    // Cria a janela passando como argumento o titulo da mesma
    glutCreateWindow("Analise Forense de Imagens");

    // Registra a funcao callback de redesenho da janela de visualizacao
    glutDisplayFunc(draw);

    // Registra a funcao callback para tratamento das teclas ASCII
    glutKeyboardFunc(keyboard);

    // Exibe as dimensões na tela, para conferência
    printf("Entrada  : %s %d x %d\n", argv[1], pic[0].width, pic[0].height);
    sel = 0; // entrada

    // Define a janela de visualizacao 2D
    glMatrixMode(GL_PROJECTION);
    gluOrtho2D(0.0, width, height, 0.0);
    glMatrixMode(GL_MODELVIEW);

    // Cria texturas em memória a partir dos pixels das imagens
    tex[0] = SOIL_create_OGL_texture((unsigned char *)pic[0].img, width, height, SOIL_LOAD_RGB, SOIL_CREATE_NEW_ID, 0);
    tex[1] = SOIL_create_OGL_texture((unsigned char *)pic[1].img, width, height, SOIL_LOAD_RGB, SOIL_CREATE_NEW_ID, 0);

    // Aplica o algoritmo
    processa();

    // Entra no loop de eventos, não retorna
    glutMainLoop();
}

int comparar(const void *a, const void *b) {
    return (*(unsigned char *)a - *(unsigned char *)b);
}

unsigned char calcular_mediana_vizinhos(int x, int y, Pixel(*in)[width])
{

    unsigned char tons_de_cinza[9];
    int contador = 0;

    float fatorFloatR = 0.3;
    float fatorFloatG = 0.59;
    float fatorFloatB = 0.11;

    for (int j = -1; j <= 1; j++) {
        for (int i = -1; i <= 1; i++) {
            int nx = x + i;
            int ny = y + j;

            if (nx >= 0 && nx < width && ny >= 0 && ny < height) {

                unsigned char gray = (unsigned char)(
                    in[ny][nx].r * fatorFloatR +
                    in[ny][nx].g * fatorFloatG +
                    in[ny][nx].b * fatorFloatB
                );
                tons_de_cinza[contador++] = gray;
            }
        }
    }

    for (int i = 0; i < contador - 1; i++) {
        for (int j = i + 1; j < contador; j++) {
            if (tons_de_cinza[i] > tons_de_cinza[j]) {
                unsigned char temp = tons_de_cinza[i];
                tons_de_cinza[i] = tons_de_cinza[j];
                tons_de_cinza[j] = temp;
            }
        }
    }

    return tons_de_cinza[contador / 2];
}


void processa()
{
    Pixel(*in)[width] = (Pixel(*)[width])pic[0].img;
    Pixel(*out)[width] = (Pixel(*)[width])pic[1].img;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {

            unsigned char mediana_gray = calcular_mediana_vizinhos(x, y, in);


            Pixel pixel_mediana_rgb;
            for (int j = -1; j <= 1; j++) {
                for (int i = -1; i <= 1; i++) {
                    int nx = x + i;
                    int ny = y + j;

                    if (nx >= 0 && nx < width && ny >= 0 && ny < height) {

                        unsigned char gray = (unsigned char)(
                            in[ny][nx].r * 0.3 +
                            in[ny][nx].g * 0.59 +
                            in[ny][nx].b * 0.11
                        );
                        if (gray == mediana_gray) {

                            pixel_mediana_rgb = in[ny][nx];
                        }
                    }
                }
            }

            unsigned char original_r = in[y][x].r;
            unsigned char original_g = in[y][x].g;
            unsigned char original_b = in[y][x].b;

            int novo_valor_r = (original_r - pixel_mediana_rgb.r) * fator;
            int novo_valor_g = (original_g - pixel_mediana_rgb.g) * fator;
            int novo_valor_b = (original_b - pixel_mediana_rgb.b) * fator;

            novo_valor_r = novo_valor_r < 0 ? 0 : (novo_valor_r > 255 ? 255 : novo_valor_r);
            novo_valor_g = novo_valor_g < 0 ? 0 : (novo_valor_g > 255 ? 255 : novo_valor_g);
            novo_valor_b = novo_valor_b < 0 ? 0 : (novo_valor_b > 255 ? 255 : novo_valor_b);

            out[y][x].r = (unsigned char)novo_valor_r;
            out[y][x].g = (unsigned char)novo_valor_g;
            out[y][x].b = (unsigned char)novo_valor_b;
        }
    }

    // Faz upload da nova textura na GPU - NÃO ALTERAR
    glBindTexture(GL_TEXTURE_2D, tex[1]);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, out);
}



// Gerencia eventos de teclado
void keyboard(unsigned char key, int x, int y)
{
    if (key == 27)
    {
        // ESC: libera memória e finaliza
        free(pic[0].img);
        free(pic[1].img);
        exit(1);
    }
    if (key >= '1' && key <= '2')
        // 1-2: seleciona a imagem correspondente (origem ou destino)
        sel = key - '1';

    if(key == '=') {
        fator += 5;
        processa();
    }
    if(key == '-') {
        fator -= 5;
        processa();
    }
    glutPostRedisplay();
}

// Callback de redesenho da tela
void draw()
{
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Preto
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Para outras cores, veja exemplos em /etc/X11/Pixel.txt

    glColor3ub(255, 255, 255); // branco

    // Ativa a textura corresponde à imagem desejada
    glBindTexture(GL_TEXTURE_2D, tex[sel]);
    // E desenha um retângulo que ocupa toda a tela
    glEnable(GL_TEXTURE_2D);
    glBegin(GL_QUADS);

    glTexCoord2f(0, 0);
    glVertex2f(0, 0);

    glTexCoord2f(1, 0);
    glVertex2f(pic[sel].width, 0);

    glTexCoord2f(1, 1);
    glVertex2f(pic[sel].width, pic[sel].height);

    glTexCoord2f(0, 1);
    glVertex2f(0, pic[sel].height);

    glEnd();
    glDisable(GL_TEXTURE_2D);

    // Exibe a imagem
    glutSwapBuffers();
}
