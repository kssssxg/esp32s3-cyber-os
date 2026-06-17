// ##################################################################################
// #######   CUSTOM CONFIGURATION FOR 1.3" ST7789 240x240 (7Pin NO CS)   ############
// ##################################################################################

// 1. 驱动选择
#define ST7789_DRIVER

// 2. 屏幕分辨率
#define TFT_WIDTH  240
#define TFT_HEIGHT 240
#define USE_HSPI_PORT
// 3. ESP32 引脚定义 (7Pin 无CS)
#define TFT_MISO -1
#define TFT_MOSI 7      // SDA
#define TFT_SCLK 6      // SCL
#define TFT_CS   -1      // 无CS引脚，必须为 -1
#define TFT_DC    2      // 数据/命令选择
#define TFT_RST   3      // 复位引脚
#define TFT_BL    38     // 背光控制

// 4. 背光有效电平
#define TFT_BACKLIGHT_ON HIGH

// 5. SPI 通信参数
#define TFT_SPI_MODE SPI_MODE3
#define SPI_FREQUENCY  40000000

// 6. 颜色与字体
#define TFT_INVERSION_OFF
#define TFT_RGB_ORDER TFT_BGR

#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define LOAD_GFXFF
#define SMOOTH_FONT
#define USE_FSPI_PORT 
#define TFT_PSRAM_ENABLE      // 强制帧缓冲放到 PSRAM

// 启用 DMA 传输
#define ESP32_DMA
// DMA 缓冲区大小（可根据需要调整 128~512）
#define DMA_BUFFER_SIZE  256
