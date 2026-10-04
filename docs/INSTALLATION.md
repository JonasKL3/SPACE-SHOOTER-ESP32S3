# Instalação — SPACE SHOOTER v1.0.0

## 1. Ambiente

Use a Arduino IDE com suporte ao ESP32-S3.

## 2. Dependências

Instale:

- TFT_eSPI
- DabbleESP32

O sketch também utiliza APIs padrão do Arduino/ESP32, incluindo `esp_random()`.

## 3. Display

Configure `TFT_eSPI` para o display ILI9341 e para a pinagem utilizada na sua placa.

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
