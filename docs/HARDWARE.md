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

A pinagem física do ILI9341 não está definida dentro do sketch. Ela depende da configuração local da biblioteca `TFT_eSPI` usada no hardware.

Para manter compatibilidade com sua montagem, use a mesma configuração física já validada no seu ESP32-S3/ILI9341.
