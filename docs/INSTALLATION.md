# Instalação — SPACE SHOOTER v1.1.0

## 1. Ambiente

Use a Arduino IDE com suporte ao ESP32-S3.

## 2. Dependências

Instale:

- TFT_eSPI
- DabbleESP32

O sketch também utiliza APIs padrão do Arduino/ESP32, incluindo `esp_random()`.

## 3. Display / TFT_eSPI

O repositório inclui a configuração de hardware validada em:

```text
config/TFT_eSPI/User_Setup.h
```

Copie esse arquivo para a biblioteca `TFT_eSPI`, substituindo o `User_Setup.h` ativo antes de compilar.

A configuração incluída usa:

- ILI9341 via SPI
- TFT MISO: GPIO 13
- TFT MOSI: GPIO 11
- TFT SCLK: GPIO 12
- TFT CS: GPIO 10
- TFT DC/RS: GPIO 9
- TFT RESET: GPIO 4
- XPT2046 TOUCH CS: GPIO 14
- TOUCH IRQ: GPIO 17 (reservado; não utilizado atualmente)
- SPI do TFT: 40 MHz
- SPI de leitura: 20 MHz
- SPI do touch: 2.5 MHz

O firmware espera uma área gráfica de 320x240 e usa rotação 2.

## 4. Abrir o firmware

Abra:

```text
firmware/SPACE_SHOOTER_ESP32S3/SPACE_SHOOTER_ESP32S3.ino
```

## 5. Compilar e gravar

Selecione sua placa ESP32-S3, porta serial e configurações compatíveis com o módulo utilizado. Compile e faça upload.

## 6. Bluetooth

Após iniciar, o firmware executa:

```cpp
Dabble.begin("ESP32-S3-GAMEPAD");
```

No aplicativo Dabble:

1. Conecte-se a `ESP32-S3-GAMEPAD`.
2. Abra o módulo GamePad.
3. Pressione START para iniciar.

## 7. Serial

Baud rate:

```text
115200
```

O firmware imprime informações de inicialização e mudanças de estado no Monitor Serial.
