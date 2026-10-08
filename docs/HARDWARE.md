# Hardware

Configuração validada para o projeto SPACE-SHOOTER-ESP32S3.

## Placa

- ESP32-S3 N16R8

## Display

- ILI9341 2.8\"
- 240 x 320 pixels
- SPI
- Uso no jogo em orientação 320 x 240

## Pinagem TFT

| Função | GPIO |
|---|---:|
| MISO / SDO | 13 |
| MOSI / SDI | 11 |
| SCK | 12 |
| CS | 10 |
| DC / RS | 9 |
| RESET | 4 |
| LED / BL | 3.3V |

## Touch XPT2046

| Função | GPIO |
|---|---:|
| T_CLK | 12 |
| T_DIN | 11 |
| T_DO | 13 |
| T_CS | 14 |
| T_IRQ | 17 |

O touch compartilha o barramento SPI do display. O jogo atualmente não depende do touch para controle.

## TFT_eSPI

A configuração testada está em:

`config/TFT_eSPI/User_Setup.h`

Parâmetros principais utilizados:

- ILI9341_DRIVER
- SPI_FREQUENCY 40000000
- SPI_READ_FREQUENCY 20000000
- SPI_TOUCH_FREQUENCY 2500000
- USE_HSPI_PORT
- USE_DMA_TO_TFT

Use o arquivo fornecido no repositório para reproduzir a configuração utilizada nos testes.
