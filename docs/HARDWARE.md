# Hardware

## Plataforma

- ESP32-S3
- Display TFT ILI9341 2.8\"
- Resolução lógica utilizada pelo jogo: 320x240
- Rotação configurada no firmware: `2`
- Controle sem fio via BLE/Dabble

## TFT_eSPI

O sketch cria o display com:

```cpp
TFT_eSPI tft = TFT_eSPI(320, 240);
```

E configura:

```cpp
tft.setRotation(2);
```

A pinagem física do ILI9341 não está definida dentro do sketch. Ela é configurada pela biblioteca `TFT_eSPI`.

O setup validado para este projeto está versionado em:

```text
config/TFT_eSPI/User_Setup.h
```

## Pinagem validada

| Sinal | GPIO |
|---|---:|
| TFT MISO | 13 |
| TFT MOSI | 11 |
| TFT SCLK | 12 |
| TFT CS | 10 |
| TFT DC/RS | 9 |
| TFT RESET | 4 |
| TOUCH CS | 14 |
| TOUCH IRQ | 17 |

O XPT2046 compartilha SCLK, MOSI e MISO com o display. O backlight do módulo está ligado diretamente a 3.3 V.

A configuração validada usa `SPI_FREQUENCY` de 40 MHz, leitura a 20 MHz e touch a 2.5 MHz.
