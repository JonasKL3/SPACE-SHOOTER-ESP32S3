//                            USER DEFINED SETTINGS
//
//   ESP32-S3 N16R8 + TFT 2.8" ILI9341 + XPT2046
//
//   TFT: 240 x RGB x 320
//   Interface: SPI
//
//   Pinagem:
//
//   TFT SDO/MISO -> GPIO 13
//   TFT SDI/MOSI -> GPIO 11
//   TFT SCK      -> GPIO 12
//   TFT CS       -> GPIO 10
//   TFT DC/RS    -> GPIO 9
//   TFT RESET    -> GPIO 4
//   TFT LED/BL   -> 3.3V
//
//   TOUCH:
//
//   T_CLK -> GPIO 12
//   T_DIN -> GPIO 11
//   T_DO  -> GPIO 13
//   T_CS  -> GPIO 14
//   T_IRQ -> GPIO 17
//
// ================================================================================


// ##################################################################################
//
// Section 1. DRIVER
//
// ##################################################################################

#define USER_SETUP_INFO "ESP32-S3_N16R8_ILI9341_XPT2046"


// -------------------------------------------------------------------------------
// DRIVER DO DISPLAY
// -------------------------------------------------------------------------------

#define ILI9341_DRIVER


// -------------------------------------------------------------------------------
// ORDEM DAS CORES
// -------------------------------------------------------------------------------
//
// Deixe comentado por enquanto.
// Se as cores aparecerem invertidas (céu laranja em vez de azul),
// descomente UMA das linhas abaixo e teste.
//

//#define TFT_RGB_ORDER TFT_RGB
//#define TFT_RGB_ORDER TFT_BGR


// -------------------------------------------------------------------------------
// RESOLUÇÃO
// -------------------------------------------------------------------------------
//
// ILI9341: 240 x 320
// Não definir TFT_WIDTH/TFT_HEIGHT aqui (o driver já define).


// ##################################################################################
//
// Section 2. PINOS DO TFT
//
// ##################################################################################


// -------------------------------------------------------------------------------
// SPI DO DISPLAY
// -------------------------------------------------------------------------------

#define TFT_MISO 13
#define TFT_MOSI 11
#define TFT_SCLK 12


// -------------------------------------------------------------------------------
// CONTROLE DO DISPLAY
// -------------------------------------------------------------------------------

#define TFT_CS   10
#define TFT_DC    9
#define TFT_RST   4


// -------------------------------------------------------------------------------
// BACKLIGHT
// -------------------------------------------------------------------------------
//
// LED/BL está ligado diretamente ao 3.3V.
// Portanto não usamos TFT_BL.

//#define TFT_BL 15
//#define TFT_BACKLIGHT_ON HIGH


// ##################################################################################
//
// Section 2B. TOUCH XPT2046
//
// ##################################################################################
//
// O touch compartilha:
//   SCK  -> GPIO 12
//   MOSI -> GPIO 11
//   MISO -> GPIO 13
//
// CS separado:
//   T_CS -> GPIO 14
//
// IRQ -> GPIO 17 (não usado por enquanto)
//

#define TOUCH_CS 14


// ##################################################################################
//
// Section 3. FONTES
//
// ##################################################################################

#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define LOAD_GFXFF
#define SMOOTH_FONT


// ##################################################################################
//
// Section 4. SPI
//
// ##################################################################################


// -------------------------------------------------------------------------------
// VELOCIDADE DO SPI DO TFT
// -------------------------------------------------------------------------------
//
// HISTÓRICO:
//   5 MHz  -> funcionava, sem estática, mas LENTO
//   40 MHz -> valor-alvo para ESP32-S3 + ILI9341
//
// SE APARECER ESTÁTICA / LIXO NA TELA:
//   1. Reduza para o valor anterior (tente 27 MHz, 20 MHz, 15 MHz)
//   2. Verifique se os fios MOSI/SCK estão curtos (< 10 cm ideal)
//   3. Adicione capacitor de 100nF entre 3.3V e GND perto do display
//
// Estratégia recomendada: subir em degraus e testar.
//   5 -> 15 -> 27 -> 40 MHz
//   Pare no primeiro que apresentar estática e volte um degrau.
//

#define SPI_FREQUENCY 40000000      // 40 MHz - valor recomendado
// Se 40 MHz der estática, troque por 27000000 (27 MHz).
// Se 27 MHz der estática, troque por 15000000 (15 MHz).


// -------------------------------------------------------------------------------
// VELOCIDADE DE LEITURA
// -------------------------------------------------------------------------------

#define SPI_READ_FREQUENCY 20000000


// -------------------------------------------------------------------------------
// VELOCIDADE DO TOUCH
// -------------------------------------------------------------------------------
//
// XPT2046 trabalha com frequência baixa.
// 2.5 MHz é o padrão recomendado.
//

#define SPI_TOUCH_FREQUENCY 2500000


// -------------------------------------------------------------------------------
// SPI PORT
// -------------------------------------------------------------------------------
//
// IMPORTANTE: manter USE_HSPI_PORT ativado.
// No seu ESP32-S3, isso eliminou o crash StoreProhibited.
// NÃO remover.
//

#define USE_HSPI_PORT


// -------------------------------------------------------------------------------
// DMA (opcional, mas recomendado no ESP32-S3)
// -------------------------------------------------------------------------------
//
// Habilita transferência DMA no push do sprite, liberando a CPU
// durante o envio SPI. Requer TFT_eSPI 2.5.43 ou superior.
//
// Se der erro de compilação, simplesmente comente esta linha.
//

#define USE_DMA_TO_TFT


// ##################################################################################
//
// FIM DA CONFIGURAÇÃO
//
// ##################################################################################
