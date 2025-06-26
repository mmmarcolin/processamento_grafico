# Processamento Grafico

## Steps to reproduce

1. `Clone repository`
2. `Ctrl+Shift+P > Select a Kit > GCC`
4. `cd build`
5. `cmake --build .`
6. `cmd /c {fileName}.exe`

## Files
| FileName       | Description                     | Colleagues                                                      |
|----------------|---------------------------------|-----------------------------------------------------------------|
| `tarefa02`     | Triângulos personalizados       | Matheus Trindade                                                |
| `tarefa03`     | Jogo das cores                  | Matheus Trindade, Lucas Locatelli                               |
| `tarefa04`     | Hitboxes em sprites             | Matheus Trindade                                                |
| `tarefa05`     | Animação                        | Matheus Trindade                                                |
| `vivencial01`  | Triângulos de tamanhos variados | Matheus Trindade, Lucas Locatelli                               |
| `vivencial02`  | Fundo em Parallax               | Matheus Trindade, Mariana Sales, Lucas Locatelli, Bruno Gerling |
| `vivencial03`  | Tilemap Isométrico              | Matheus Trindade, Mariana Sales, Lucas Locatelli, Bruno Gerling |
| `grauB`        | Jogo Tilemap Isométrico         | Matheus Trindade, Mariana Sales                                 |

# grauB - Jogo Tilemap Isométrico

Alunos: Mariana Machado Sales e Matheus Marcolin Trindade

Este projeto é um jogo de coleta de moedas em um mapa isométrico, desenvolvido em C++ com OpenGL. O objetivo é coletar todas as moedas do mapa sem pisar na lava.

![alt text](image.png)

## Funcionalidades

- **Mapa isométrico** carregado de arquivo texto (`../assets/map.txt`)
- **Tileset** customizável (definido no arquivo de mapa)
- **Jogador**
- **Moedas animadas** 
- **Controles por teclado** (setas)
- **Estados do jogo:** jogando, vitória, derrota
- **Reinício rápido** (tecla R)
- **Detecção de colisão** com lava e moedas

## Controles

| Tecla         | Ação                        |
|-------------- |-----------------------------|
| Setas         | Move o personagem           |
| R             | Reinicia o jogo             |

## Como jogar

1. Compile o projeto (requer GLFW, GLAD, stb_image, glm).
2. Execute o binário gerado.
3. Use as setas para mover o personagem.
4. Colete todas as moedas sem pisar na lava.
5. Ao vencer ou perder, pressione R para reiniciar.

## Recursos

- `../assets/tilesets/`: tilesets
- `../assets/sprites/`: sprites do jogador e moedas
- `../assets/map.txt`: mapa do jogo

## Observações

- O personagem inicia no centro do mapa.
- O tile laranja representa lava (game over).
- O tile azul escuro  representa água (não é possível caminhar sobre ele).
- É possível caminhar sobre os demais tiles, coletando as moedas presentes nos mesmos.
