

// ----------------------------------------------------------------------------
// [001] 头文件依赖与核心系统库
// ----------------------------------------------------------------------------
#include <Arduino.h>
#include "FS.h"
#include "SD.h"
#include <NTPClient.h>   // 需要安装库：NTPClient by Fabrice Weinberg
#include "SPI.h"
#include <TFT_eSPI.h>
#include <TJpg_Decoder.h>
#include <NimBLEDevice.h>
#include <Audio.h>
#include <PNGdec.h>
// 修复 PNGdec 与 NimBLE 的宏冲突
#ifdef local
#undef local
#endif
#include <AnimatedGIF.h>
//#include <WiFi.h>
//#include <WebServer.h>
#include <vector>
#include "cJSON.h"                 // JSON解析库，依赖 cJSON.c
#include <set>
#include <map>
#include <WiFiClientSecure.h>
#include "124.h" // 用户提供的纯正数组中文字库 t18
#include <SdFat.h>
#include <MD_MIDIFile.h>
#include <Adafruit_NeoPixel.h> 
// [001] 头文件依赖 - 末尾新增
#include "devices.hpp"
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFiUdp.h>
// ... (文件结尾)        // 新增 WS2812 驱动
// NES模拟器头文件 (直接使用nes文件夹里的emucore现成代码)
#include "nes_config.h"
#include "emucore.h"
#include "nes_bridge.cpp"  // NES bridge implementation
void runNesEmulator(String romPath);
// 模板应用系统
#include "app_templates.h"
// FcGameEntry 必须在所有函数声明之前定义 (Arduino IDE原型生成)
struct FcGameEntry { String name; bool isDir; uint32_t size; };
// ... 原有头文件 ...
// ========== 太空人时钟专用头文件 ==========
#include "font/ZdyLwFont_20.h"
#include "font/FxLED_32.h"
#include <TimeLib.h>
#include "img/pangzi/i0.h"
#include "img/pangzi/i1.h"
#include "img/pangzi/i2.h"
#include "img/pangzi/i3.h"
#include "img/pangzi/i4.h"
#include "img/pangzi/i5.h"
#include "img/pangzi/i6.h"
#include "img/pangzi/i7.h"
#include "img/pangzi/i8.h"
#include "img/pangzi/i9.h"

#include "img/temperature.h"
#include "img/humidity.h"
#include "img/watch_top.h"
#include "img/watch_bottom.h"
#include <memory>   // 用于 std::unique_ptr
// [001] 头文件依赖 - 末尾新增
#include <WiFi.h>
#include <Preferences.h> // 用于NVS存储
// ESP8266Audio removed — all audio via ESP32-audioI2S

// [003] 全局系统对象 - 末尾新增
// preferences改为函数内局部变量，节省全局RAM
bool wifiConfigured = false; // WiFi配置标志
unsigned long lastBtnPress = 0;
bool longPressHandled = false;
// 灯光控制全局变量
#define WS2812_PIN     48
#define NUMPIXELS      1
Adafruit_NeoPixel pixels(NUMPIXELS, WS2812_PIN, NEO_GRB + NEO_KHZ800);
// ========== 输入法引擎全局变量 ==========
// 拼音字典加载状态
struct pinyin_ime_t {
    cJSON *dict_root;
    bool is_dict_loaded;
};
pinyin_ime_t g_pinyin_ime;
int candidateScrollOffset = 0;   // 候选列表绘制时的起始字符索引
// 输入法 UI 状态
String inputTextBuffer = "";        // 最终输出的文本
String pinyinBuffer = "";           // 当前拼写的拼音
String candidates = "";             // 候选汉字列表
int candidateIndex = 0;
// 音乐API源枚举
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "ntp.aliyun.com", 8 * 3600, 60000);  // 东八区，60秒更新一次
// ========== 闹钟系统 ==========
#define MAX_ALARMS 3
struct Alarm {
    uint8_t hour;       // 0-23
    uint8_t minute;     // 0-59
    uint8_t repeatDays; // bit0=周一..bit6=周日, 0=一次性
    bool enabled;
    char ringtone[64];  // 铃声文件路径 (空=默认蜂鸣)
};
Alarm alarms[MAX_ALARMS];
bool alarmTriggered = false;
unsigned long alarmTriggerTime = 0;
int alarmTriggeredIndex = -1;  // 哪个闹钟触发了
bool ntpSynced = false;
bool alarmFiredToday[MAX_ALARMS] = {false};
uint8_t alarmVolume = 50;  // 闹钟音量 0-100, 持久化到 flash
// ========== 番茄钟 ==========
bool pomodoroActive = false;
unsigned long pomodoroEndTime = 0;
bool pomodoroIsBreak = false;
uint8_t pomodoroSessions = 0;

// ========== 桌面系统 ==========
struct DesktopApp {
    const char* name;
    uint16_t iconColor;
    const char* action;
};

const DesktopApp desktopApps[] = {
    // 第1页 (9个)
    {"文件管理", 0x07FF,    "FILE_MANAGER"},   // CYAN
    {"云音乐",   0x07E0,    "ONLINE_MUSIC"},   // GREEN
    {"闹钟",     0xFD20,    "ALARM"},          // ORANGE
    {"番茄钟",   0xF800,    "POMODORO"},       // RED
    {"计算器",   0x001F,    "CALC"},           // BLUE
    {"AI助手",   0xF81F,    "AI"},             // MAGENTA
    {"FC游戏",   0xFFE0,    "FC"},             // YELLOW
    {"时钟",     0xFFFF,    "CLOCK"},          // WHITE
    {"秒表",     0x07FF,    "STOPWATCH"},      // CYAN
    // 第2页 (6个)
    {"计时器",   0xFFE0,    "COUNTDOWN"},      // YELLOW
    {"文件搜索", 0x07E0,    "FILESEARCH"},     // GREEN
    {"苹果诱骗", 0xF81F,    "APPLE_SPOOF"},    // MAGENTA
    {"系统仪表", 0x07FF,    "DASHBOARD"},      // CYAN
    {"日历",     0xFD20,    "CALENDAR"},       // ORANGE
    {"脚本编辑", 0x07E0,    "SCRIPT_EDITOR"},  // GREEN
    {"设置",     0xC618,    "SETTINGS"},       // LIGHTGREY
    {"随机音乐", 0x07E0,    "RANDOM_MUSIC"},   // GREEN
};
#define DESKTOP_BUILTIN 17  // 内置应用数量
#define DESKTOP_PER_PAGE 9
#define DESKTOP_COLS 3
#define DESKTOP_ROWS 3

// 脚本应用 (开机从 /apps/ 扫描)
std::vector<DesktopApp> scriptApps;
int desktopTotal() { return DESKTOP_BUILTIN + (int)scriptApps.size(); }

// 获取桌面应用 (内置+脚本)
const char* desktopAppName(int idx) {
    if (idx < DESKTOP_BUILTIN) return desktopApps[idx].name;
    return scriptApps[idx - DESKTOP_BUILTIN].name;
}
uint16_t desktopAppColor(int idx) {
    if (idx < DESKTOP_BUILTIN) return desktopApps[idx].iconColor;
    return scriptApps[idx - DESKTOP_BUILTIN].iconColor;
}
const char* desktopAppAction(int idx) {
    if (idx < DESKTOP_BUILTIN) return desktopApps[idx].action;
    return scriptApps[idx - DESKTOP_BUILTIN].action;
}

void scanApps() {
    scriptApps.clear();
    File dir = SD.open("/apps");
    if (!dir || !dir.isDirectory()) return;
    File f = dir.openNextFile();
    while (f) {
        if (f.isDirectory()) {
            String name = f.name();
            if (name != "." && name != "..") {
                String appDir = "/apps/" + name;
                String infoPath = appDir + "/app.info";
                if (SD.exists(infoPath)) {
                    AppConfig cfg;
                    if (cfg.load(infoPath)) {
                        DesktopApp app;
                        app.name = strdup(cfg.name.c_str());
                        app.iconColor = cfg.color;
                        app.action = strdup(appDir.c_str());
                        scriptApps.push_back(app);
                        Serial.printf("[APP] Found: %s -> %s\n", cfg.name.c_str(), appDir.c_str());
                    }
                }
            }
        }
        f = dir.openNextFile();
    }
    dir.close();
    Serial.printf("[APP] Total apps: %d\n", (int)scriptApps.size());
}

int desktopPage = 0;
int desktopCursorX = 0;
int desktopCursorY = 0;
bool onDesktop = true;

// 屏幕自动休眠 (仅桌面模式)
unsigned long lastDesktopActivity = 0;
bool screenSleeping = false;
uint8_t g_brightness = 255;    // 当前背光亮度(from NVS)
unsigned long g_screenSleepMs = 15000; // 自动熄屏超时(ms), 可从NVS加载
// 模式枚举（替换原来的 chineseMode 变量）
struct MusicInfo {
    String name;
    String singer;
    String url;        // CYAPI已有直链；网易云播放时再获取
    String id;         // 歌曲ID（网易云用）
    int duration;      // 时长ms
    bool isNetease;    // true=网易云, false=CYAPI(QQ)
};

enum InputMode { MODE_CHINESE, MODE_ENGLISH, MODE_NUMBER };
InputMode currentMode = MODE_CHINESE;
bool inputCompleted = false;        // 输入完成标志
// 数字/符号键盘布局（4行10列）
const char* keys_num[4] = {
    "1234567890",
    "!@#$%^&*() ",
    " -/?:;.,+=",        // 第三行：空格、减号、斜杠等
    "  \x01\x7f \x03"    // 第四行：模式切换、退格、空格、确认
};
// ========== 苹果 BLE 模拟器全局变量 ==========
namespace AppleBLE {
          // 引用主程序的屏幕对象

    uint32_t delayMilliseconds = 100;       // 广播间隔毫秒
    // pAdvertising 和 preferences 改为在appleJuiceControl()内创建（节省开机RAM）

    // 预设攻击模式——从原始代码中提取常用设备，方便菜单选择
    enum AttackMode {
        MODE_AIRPODS = 0,
        MODE_AIRPODS_GEN2,
        MODE_AIRPODS_MAX,
        MODE_APPLETV_SETUP,
        MODE_APPLETV_PAIR,
        MODE_HOMEPOD_SETUP,
        MODE_VISION_PRO,
        MODE_SOFTWARE_UPDATE,
        MODE_TRANSFER_NUMBER,
        MODE_RANDOM_DEVICE,      // 从所有设备中随机选择
        MODE_COUNT
    };

    const char* attackModeNames[MODE_COUNT] = {
        "AirPods",
        "AirPods Gen2",
        "AirPods Max",
        "AppleTV 设置",
        "AppleTV 配对",
        "HomePod 设置",
        "Vision Pro",
        "软件更新",
        "迁移号码",
        "随机设备"
    };

    int currentAttackMode = MODE_AIRPODS;   // 当前选择的模式
    bool bleActive = false;                 // 广播是否正在运行
}
// ========== 苹果 BLE 模拟器函数声明 ==========

// 原有的 keys 数组更名为 keys_abc（用于中/英文模式）
const char* keys_abc[4] = {
    "qwertyuiop",
    " asdfghjkl",
    "  zxcvbnm",
    "  \x01\x7f \x03"
};
int keyRow = 0, keyCol = 0;        // 当前高亮键坐标
// 焦点是否在候选字上（仅对中文模式有效）
bool focusOnCandidates = false;
// 灯光模式枚举
enum LedMode {
    LED_OFF,
    LED_STATIC,
    LED_BLINK,
    LED_BREATH
};
uint8_t ledMode = LED_STATIC;
uint8_t ledBrightness = 128;          // 0-255
uint32_t ledColor = pixels.Color(0, 255, 0); // 默认绿色
unsigned long ledLastUpdate = 0;
int ledBreathStep = 5;                // 呼吸步进
int ledBreathValue = 0;
bool ledBreathUp = true;
// [003] 全局系统对象 - 末尾新增
#define MUSIC_CACHE_DIR "/music_cache"

// ----------------------------------------------------------------------------
// [002] 硬件 IO 映射与总线配置区
// ----------------------------------------------------------------------------
#define SD_SCK  14
#define SD_MISO 19
#define SD_MOSI 13
#define SD_CS   15
#define TFT_BL  38

// 屏幕自动休眠参数 (仅在桌面模式生效)
// (运行时变量, 见全局区 g_screenSleepMs)
#define SCREEN_FADE_STEPS  6       // 渐变动画步数
#define SCREEN_FADE_DELAY  15      // 每步延迟(ms)
// 物理按键引脚 (使用内部上拉电阻)  【修订版 - 增加左右键，移除蜂鸣器】
#define BTN_UP     4
#define BTN_OK     5
#define BTN_DOWN   16
#define BTN_LEFT   17
#define BTN_RIGHT  47
// I2S 音频输出 (MAX98357A)
#define I2S_BCLK  20
#define I2S_LRC   21
#define I2S_DOUT  12
#define MAX98357A_SD 18  // 功放 Shutdown 控制脚
// Wi-Fi 软路由热点配置
//const char* ap_ssid = "ESP32_Cyber_OS_v5";
//const char* ap_password = "12345678";

// ----------------------------------------------------------------------------
// [003] 全局系统对象与核心状态机
// ----------------------------------------------------------------------------
TFT_eSPI tft = TFT_eSPI();
//WebServer server(80);
SPIClass sdSPI(VSPI);
// === RAM优化：gif, audio改为函数内局部变量；png改为static指针（回调需要全局访问） ===
static PNG* png = nullptr;
SdFs sdFat;
MD_MIDIFile SMF;
// 图像在屏幕上的偏移坐标
int pngXpos = 0;
int pngYpos = 0;

// 系统级任务调度队列变量
String pendingTask = "";
bool hasTask = false;
bool isFontLoaded = false;

// 菜单系统状态变量
bool inMenu = false;
String currentDir = "/";
std::vector<String> menuItems;
std::vector<bool> menuIsDir;
bool menuLoaded = false;  // RAM优化：标记菜单是否已加载
int menuIdx = 0;
int menuTop = 0;
// 文件管理相关
String copiedFilePath = "";
std::vector<String> clipBoard;
// Web 端上传文件句柄
File uploadFile; 
// ========== 太空人时钟全局变量（置于命名空间内） ==========
namespace SpaceClock {
             // 引用主程序的 tft 对象
    // === RAM优化：clk和clkb改为延迟初始化的static指针 ===
    static TFT_eSprite* clk = nullptr;
    static TFT_eSprite* clkb = nullptr;

    uint16_t bgColor = 0xFFFF;
    String cityCode = "101040100";        // 天气城市代码

    // NTP 相关
    static const char ntpServerName[] = "ntp6.aliyun.com";
    const int timeZone = 8;
    WiFiUDP Udp;
    unsigned int localPort = 8000;
    
    byte packetBuffer[48];

    // 滚动字幕
    String scrollText[6];
    int currentIndex = 0;
    int prevTime = 0;

    // 原版其他变量（已无用，但保留以尊重原作）
    uint32_t targetTime = 0;
    byte omm = 99;
    boolean initial = 1;
    byte xcolon = 0;
    unsigned int colour = 0;

    // 函数声明
    void digitalClockDisplay();
    String num2str(int digits);
    String week();
    String monthDay();
    String hourMinute();
    void getCityCode();
    void getCityWeater();
    void weaterData(String* cityDZ, String* dataSK, String* dataFC);
    void scrollBanner();
    void scrollTxt(int pos);
    void imgAnim();
    void printDigits(int digits);
    time_t getNtpTime();
    void sendNTPpacket(IPAddress& address);
    bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap);

    void run();  // 主循环入口
}
// ----------------------------------------------------------------------------
// [004] 工业级日志系统 (CyberLogger)
// ----------------------------------------------------------------------------
class CyberLogger {
public:
    static void info(const String& module, const String& msg) {
        printLog("INFO ", module, msg);
    }
    
    static void warn(const String& module, const String& msg) {
        printLog("WARN ", module, msg);
    }
    
    static void error(const String& module, const String& msg) {
        printLog("ERROR", module, msg);
    }
    
    static void debug(const String& module, const String& msg) {
        printLog("DEBUG", module, msg);
    }
    
    static void mem(const String& context) {
        uint32_t freeHeap = ESP.getFreeHeap();
        uint32_t maxBlock = ESP.getMaxAllocHeap();
        String msg = "Free: " + String(freeHeap / 1024) + " KB | MaxBlock: " + String(maxBlock / 1024) + " KB";
        printLog("MEM  ", context, msg);
    }

private:
    static void printLog(const String& level, const String& module, const String& msg) {
        unsigned long t = millis();
        unsigned long sec = t / 1000;
        unsigned long ms = t % 1000;
        
        char timeStr[16];
        sprintf(timeStr, "[%04lu.%03lu]", sec, ms);
        
        Serial.print(timeStr);
        Serial.print(" [");
        Serial.print(level);
        Serial.print("] [");
        Serial.print(module);
        Serial.print("] ");
        Serial.println(msg);
    }
};

// ----------------------------------------------------------------------------
// [005] 核心前置函数原型声明
// ----------------------------------------------------------------------------
void displayFileList();
// 输入法组件原型声明
void searchMusicOnline();
void displayMusicSearchResults(std::vector<MusicInfo>& musicList);
// 音乐API辅助函数
String buildCyApiSearchUrl(const String& keyword);   // 如果之前没声明的话
bool parseCyApiJson(const char* jsonData, std::vector<MusicInfo>& musicList);
bool searchMusicNetease(const String& keyword, std::vector<MusicInfo>& results);
String getMusicDirectUrl(const String& songId, int& httpCodeOut);
int checkUrlHttpStatus(const String& url);
void pinyin_ime_init();
String getHanzi(String pinyin);
void updateCandidates();
void handleInputKey(char ch);
// 网络诊断与下载工具声明
bool pingTest(const char* host);
void drawErrorScreen(const String& title, const String& msg);
bool downloadFileWithProgress(const String& url, String& savePath);
void drawInputUI();
String inputTextDialog(const String& prompt, const String& initial);
//void handleFileUpload();
void openAndDisplayFile(String path);
void displayTxtFile(File &file);
void displayLrcFile(String path);
void drawMarqueeText();
void playAudioFile(String path);       // MP3/WAV/AAC/M4A/FLAC/OGG 播放
//void displayJpgFile(String path);
bool smartDelay(unsigned long ms);
void displayPngFile(String path);
void displayGifFile(String path);
void displayBmpFile(String path);
void displayTgaFile(String path);
void displayPcxFile(String path);
// [005] 末尾新增
void showLongPressMenu(int menuIndex);
void renameFile(String oldPath);
void copyFile(String sourcePath, String destPath);
void createNewFile(String dirPath);
String getFilePath(int menuIndex);
void wifiSettings();
void connectToWiFi(String ssid, String password);
void loadWiFiCredentials();
void saveWiFiCredentials(String ssid, String password);
void clearWiFiCredentials();
void displayCsvFile(String path);
void displayWavVisualizer(String path);
void displayHexInspector(String path);
void displaySystemMonitor();
void runChip8Emulator(String romPath);
void runNesEmulator(String romPath);
void fcSettings();
void fcGameBrowser();
void systemDashboard();
void alarmSettings();
void pomodoroTimer();
String pickMp3File();
void drawDesktop();
void animateDesktopSlide(int fromPage, int toPage, int dir, int newCursorX, int newCursorY);
void launchDesktopApp(const char* action);
void stopwatchTimer();
void countdownTimer();
void calendarApp();
void scriptEditor();
void fileSearch();
void brightnessControl();
void checkScreenSleep();
void wakeScreen();
void animateTransition();
void animateRestore();
void sleepSettings();
void settingsMenu();
void playRandomMusic();
void loadAlarms();
void saveAlarms();
void checkAlarms();
void handleAlarmRinging();
void playAnimSequence(String animPath);
void playAviMjpeg(String path);
bool getFileExtension(String filename, String &ext);
String urlEncode(const String& url);
String urlDecode(String str);
std::vector<String> listJpgFiles(String folder);
bool compareFrameNames(const String& a, const String& b);
void midiCallback(midi_event *pev);
void playMidiFile(String path);
void drawMenu();
void loadMenu(String path);
bool checkStopAction();
void setupButtons();
bool isBtnPressed(uint8_t pin);
void loadFontSafe();
void unloadFontSafe();
void delayedDebounce(uint8_t pin);
void handleLedAnimation();
void applyLedUpdate();
void updateLEDFromSaved();
void ledControlScreen();
String sanitizeFileName(String input);
// ========== 苹果 BLE 模拟器控制界面 ==========
// ========== 苹果 BLE 模拟器控制界面 ==========
void appleJuiceControl() {
    using namespace AppleBLE;

    // === RAM优化：pAdvertising和preferences改为本地变量 ===
    NimBLEAdvertising* pAdvertising = nullptr;
    Preferences preferences;

    // 1. 断开 WiFi 释放射频资源
    if (WiFi.status() == WL_CONNECTED) {
        WiFi.disconnect(true);
        delay(100);
    }

    // 2. 初始化 BLE
    NimBLEDevice::init("");                // 空名称
    NimBLEDevice::setPower(ESP_PWR_LVL_P9);
    pAdvertising = NimBLEDevice::getAdvertising();
    if (!pAdvertising) {
        CyberLogger::error("BLE", "无法获取广播对象");
        loadWiFiCredentials();
        return;
    }

    // 3. 菜单选择模式
    int modeIdx = currentAttackMode;
    const int numModes = sizeof(attackModeNames) / sizeof(attackModeNames[0]);
    bool startBroadcast = false;   // true：启动广播；false：返回菜单

    auto drawModeMenu = [&]() {
        tft.fillScreen(TFT_BLACK);
        loadFontSafe();
        tft.setTextColor(TFT_GREEN, TFT_BLACK);
        tft.drawString("苹果蓝牙模拟器", 10, 5);
        tft.drawLine(0, 25, tft.width(), 25, TFT_DARKGREY);
        for (int i = 0; i < numModes; i++) {
            int y = 35 + i * 22;
            if (i == modeIdx) {
                tft.fillRect(0, y, tft.width(), 22, TFT_DARKCYAN);
                tft.setTextColor(TFT_WHITE, TFT_DARKCYAN);
            } else {
                tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
            }
            tft.drawString(String("[") + attackModeNames[i] + "]", 10, y);
        }
        tft.setTextColor(TFT_YELLOW, TFT_BLACK);
        tft.drawString("OK:启动  左:返回", 10, tft.height() - 25);
    };

    drawModeMenu();
    while (!startBroadcast) {
        if (digitalRead(BTN_UP) == LOW) {
            delayedDebounce(BTN_UP);
            modeIdx = (modeIdx - 1 + numModes) % numModes;
            drawModeMenu();
        }
        if (digitalRead(BTN_DOWN) == LOW) {
            delayedDebounce(BTN_DOWN);
            modeIdx = (modeIdx + 1) % numModes;
            drawModeMenu();
        }
        if (digitalRead(BTN_LEFT) == LOW) {
            delayedDebounce(BTN_LEFT);
            // 直接退出（不启动广播）
            goto cleanup;
        }
        if (digitalRead(BTN_OK) == LOW) {
            delayedDebounce(BTN_OK);
            startBroadcast = true;
            currentAttackMode = modeIdx;
        }
    }

    // 4. 构建数据包（变量提前声明，避免 goto 跨越初始化问题）
    {
        uint8_t advData[31] = {0};
        size_t advLen = 0;

        static const DeviceIndex modeToDevice[] = {
            AIRPODS,              // AirPods
            AIRPODS_GEN_2,        // AirPods Gen2
            AIRPODS_MAX,          // AirPods Max
            APPLETV_SETUP,        // AppleTV 设置
            APPLETV_PAIR,         // AppleTV 配对
            HOMEPOD_SETUP,        // HomePod 设置
            VISION_PRO,           // Vision Pro
            SOFTWARE_UPDATE,      // 软件更新
            TRANSFER_NUMBER,      // 迁移号码
        };

        if (modeIdx == MODE_RANDOM_DEVICE) {
            int r = random(NUM_DEVICES);
            generatePacket(ALL_DEVICES[r], advData, advLen);
        } else {
            generatePacket(ALL_DEVICES[modeToDevice[modeIdx]], advData, advLen);
        }

        // 设置广播数据
        NimBLEAdvertisementData advertisement;
        advertisement.addData(advData, advLen);
        pAdvertising->setAdvertisementData(advertisement);
        // 不需要扫描响应，苹果广播包本身已包含所需信息
    }

    // 5. 启动广播
    pAdvertising->start();
    CyberLogger::info("BLE", "广播已启动: " + String(attackModeNames[modeIdx]));

    // 6. 运行中界面
    tft.fillScreen(TFT_BLACK);
    loadFontSafe();
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.drawString("BLE 广播运行中", 10, 10);
    tft.drawString("模式: " + String(attackModeNames[modeIdx]), 10, 30);
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.drawString("按 OK 停止", 10, tft.height() - 25);

    // 7. 等待用户停止
    while (true) {
        if (digitalRead(BTN_OK) == LOW) {
            delayedDebounce(BTN_OK);
            break;
        }
        delay(10);
    }

    // 8. 停止广播并清理
    pAdvertising->stop();
    NimBLEDevice::deinit(true);
    pAdvertising = nullptr;

cleanup:
    // 9. 重新连接 WiFi
    tft.fillScreen(TFT_BLACK);
    loadFontSafe();
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString("正在恢复WiFi...", 10, 30);
    loadWiFiCredentials();
    delay(2000);
}
// 高敏雷达延时：在等待期间，每一毫秒都在疯狂侦测按键！
bool smartDelay(unsigned long ms) {
    unsigned long start = millis();
    while (millis() - start < ms) {
        if (checkStopAction()) return true; // 只要按下，立刻返回 true 打断！
        //server.handleClient();
        yield();
    }
    return false;
}
/**
 * 将本地文件上传到转换服务，返回转换后的 MP3 文件路径
 * @param localPath  原始音频文件路径 (如 /music_cache/xxx.m4a)
 * @return 转换后 MP3 的 SD 卡路径，失败返回空字符串
 */
 // ========== 太空人时钟实现 ==========
bool SpaceClock::tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
    if (y >= tft.height()) return 0;
    tft.pushImage(x, y, w, h, bitmap);
    return 1;
}

String SpaceClock::num2str(int digits) {
    String s = "";
    if (digits < 10) s = s + "0";
    s = s + digits;
    return s;
}

String SpaceClock::week() {
    String wk[7] = {"日","一","二","三","四","五","六"};
    String s = "周" + wk[weekday()-1];
    return s;
}

String SpaceClock::monthDay() {
    String s = String(month());
    s = s + "月" + day() + "日";
    return s;
}

String SpaceClock::hourMinute() {
    String s = num2str(hour());
    s = s + ":" + num2str(minute());
    return s;
}

void SpaceClock::printDigits(int digits) {
    Serial.print(":");
    if (digits < 10) Serial.print('0');
    Serial.print(digits);
}

void SpaceClock::digitalClockDisplay() {
    clk->setColorDepth(8);
    // 时分
    clk->createSprite(140, 48);
    clk->fillSprite(bgColor);
    clk->setTextDatum(CC_DATUM);
    clk->setTextColor(TFT_BLACK, bgColor);
    clk->drawString(hourMinute(), 70, 24, 7);
    clk->pushSprite(28, 40);
    clk->deleteSprite();

    // 秒
    clk->createSprite(40, 32);
    clk->fillSprite(bgColor);
    clk->loadFont(FxLED_32);
    clk->setTextDatum(CC_DATUM);
    clk->setTextColor(TFT_BLACK, bgColor);
    clk->drawString(num2str(second()), 20, 16);
    clk->unloadFont();
    clk->pushSprite(170, 60);
    clk->deleteSprite();

    // 星期
    clk->loadFont(ZdyLwFont_20);
    clk->createSprite(58, 32);
    clk->fillSprite(bgColor);
    clk->setTextDatum(CC_DATUM);
    clk->setTextColor(TFT_BLACK, bgColor);
    clk->drawString(week(), 29, 16);
    clk->pushSprite(1, 168);
    clk->deleteSprite();

    // 月日
    clk->createSprite(98, 32);
    clk->fillSprite(bgColor);
    clk->setTextDatum(CC_DATUM);
    clk->setTextColor(TFT_BLACK, bgColor);
    clk->drawString(monthDay(), 49, 16);
    clk->pushSprite(61, 168);
    clk->deleteSprite();
    clk->unloadFont();
}

void SpaceClock::getCityCode() {
    String URL = "http://wgeo.weather.com.cn/ip/?_=" + String(now());
    HTTPClient httpClient;
    httpClient.begin(URL);
    httpClient.setUserAgent("Mozilla/5.0 (iPhone; CPU iPhone OS 11_0 like Mac OS X) AppleWebKit/604.1.38 (KHTML, like Gecko) Version/11.0 Mobile/15A372 Safari/604.1");
    httpClient.addHeader("Referer", "http://www.weather.com.cn/");
    int httpCode = httpClient.GET();
    if (httpCode == HTTP_CODE_OK) {
        String str = httpClient.getString();
        int aa = str.indexOf("id=");
        if (aa > -1) {
            cityCode = str.substring(aa+4, aa+4+9);
            Serial.println(cityCode);
            getCityWeater();
        } else {
            Serial.println("获取城市代码失败");
        }
    } else {
        Serial.println("请求城市代码错误：");
        Serial.println(httpCode);
    }
    httpClient.end();
}

void SpaceClock::getCityWeater() {
    String URL = "http://d1.weather.com.cn/weather_index/" + cityCode + ".html?_=" + String(now());
    HTTPClient httpClient;
    httpClient.begin(URL);
    httpClient.setUserAgent("Mozilla/5.0 (iPhone; CPU iPhone OS 11_0 like Mac OS X) AppleWebKit/604.1.38 (KHTML, like Gecko) Version/11.0 Mobile/15A372 Safari/604.1");
    httpClient.addHeader("Referer", "http://www.weather.com.cn/");
    int httpCode = httpClient.GET();
    if (httpCode == HTTP_CODE_OK) {
        String str = httpClient.getString();
        int indexStart = str.indexOf("weatherinfo\":");
        int indexEnd = str.indexOf("};var alarmDZ");
        String jsonCityDZ = str.substring(indexStart+13, indexEnd);
        Serial.println(jsonCityDZ);

        indexStart = str.indexOf("dataSK =");
        indexEnd = str.indexOf(";var dataZS");
        String jsonDataSK = str.substring(indexStart+8, indexEnd);
        Serial.println(jsonDataSK);

        indexStart = str.indexOf("\"f\":[");
        indexEnd = str.indexOf(",{\"fa");
        String jsonFC = str.substring(indexStart+5, indexEnd);
        Serial.println(jsonFC);

        weaterData(&jsonCityDZ, &jsonDataSK, &jsonFC);
        Serial.println("获取成功");
    } else {
        Serial.println("请求城市天气错误：");
        Serial.print(httpCode);
    }
    httpClient.end();
}

void SpaceClock::weaterData(String* cityDZ, String* dataSK, String* dataFC) {
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, *dataSK);
    JsonObject sk = doc.as<JsonObject>();

    // 温度
    clk->setColorDepth(8);
    clk->loadFont(ZdyLwFont_20);
    clk->createSprite(54, 32);
    clk->fillSprite(bgColor);
    clk->setTextDatum(CC_DATUM);
    clk->setTextColor(TFT_BLACK, bgColor);
    clk->drawString(sk["temp"].as<String>()+"℃", 27, 16);
    clk->pushSprite(185, 168);
    clk->deleteSprite();

    // 城市名
    clk->createSprite(88, 32);
    clk->fillSprite(bgColor);
    clk->setTextDatum(CC_DATUM);
    clk->setTextColor(TFT_BLACK, bgColor);
    clk->drawString(sk["cityname"].as<String>(), 44, 16);
    clk->pushSprite(151, 1);
    clk->deleteSprite();

    // PM2.5
    uint16_t pm25BgColor = tft.color565(156,202,127);
    String aqiTxt = "优";
    int pm25V = sk["aqi"];
    if (pm25V > 200) {
        pm25BgColor = tft.color565(136,11,32);
        aqiTxt = "重度";
    } else if (pm25V > 150) {
        pm25BgColor = tft.color565(186,55,121);
        aqiTxt = "中度";
    } else if (pm25V > 100) {
        pm25BgColor = tft.color565(242,159,57);
        aqiTxt = "轻度";
    } else if (pm25V > 50) {
        pm25BgColor = tft.color565(247,219,100);
        aqiTxt = "良";
    }
    clk->createSprite(50, 24);
    clk->fillSprite(bgColor);
    clk->fillRoundRect(0,0,50,24,4,pm25BgColor);
    clk->setTextDatum(CC_DATUM);
    clk->setTextColor(0xFFFF);
    clk->drawString(aqiTxt, 25, 13);
    clk->pushSprite(5, 130);
    clk->deleteSprite();

    // 湿度
    clk->createSprite(56, 24);
    clk->fillSprite(bgColor);
    clk->setTextDatum(CC_DATUM);
    clk->setTextColor(TFT_BLACK, bgColor);
    clk->drawString(sk["SD"].as<String>(), 28, 13);
    clk->pushSprite(180, 130);
    clk->deleteSprite();

    scrollText[0] = "实时天气 " + sk["weather"].as<String>();
    scrollText[1] = "空气质量 " + aqiTxt;
    scrollText[2] = "风向 " + sk["WD"].as<String>() + sk["WS"].as<String>();

    deserializeJson(doc, *cityDZ);
    JsonObject dz = doc.as<JsonObject>();
    scrollText[3] = "今日" + dz["weather"].as<String>();

    deserializeJson(doc, *dataFC);
    JsonObject fc = doc.as<JsonObject>();
    scrollText[4] = "最低温度" + fc["fd"].as<String>() + "℃";
    scrollText[5] = "最高温度" + fc["fc"].as<String>() + "℃";

    clk->unloadFont();
}

void SpaceClock::scrollBanner() {
    if (millis() - prevTime > 2500) {
        if (scrollText[currentIndex]) {
            clkb->setColorDepth(8);
            clkb->loadFont(ZdyLwFont_20);
            for (int pos = 24; pos > 0; pos--) {
                scrollTxt(pos);
            }
            clkb->deleteSprite();
            clkb->unloadFont();
            if (currentIndex >= 5) currentIndex = 0;
            else currentIndex += 1;
        }
        prevTime = millis();
    }
}

void SpaceClock::scrollTxt(int pos) {
    clkb->createSprite(148, 24);
    clkb->fillSprite(bgColor);
    clkb->setTextWrap(false);
    clkb->setTextDatum(CC_DATUM);
    clkb->setTextColor(TFT_BLACK, bgColor);
    clkb->drawString(scrollText[currentIndex], 74, pos+12);
    clkb->pushSprite(2, 4);
}

void SpaceClock::imgAnim() {
    int x=80, y=94, dt=30;
    TJpgDec.drawJpg(x, y, i0, sizeof(i0)); delay(dt);
    TJpgDec.drawJpg(x, y, i1, sizeof(i1)); delay(dt);
    TJpgDec.drawJpg(x, y, i2, sizeof(i2)); delay(dt);
    TJpgDec.drawJpg(x, y, i3, sizeof(i3)); delay(dt);
    TJpgDec.drawJpg(x, y, i4, sizeof(i4)); delay(dt);
    TJpgDec.drawJpg(x, y, i5, sizeof(i5)); delay(dt);
    TJpgDec.drawJpg(x, y, i6, sizeof(i6)); delay(dt);
    TJpgDec.drawJpg(x, y, i7, sizeof(i7)); delay(dt);
    TJpgDec.drawJpg(x, y, i8, sizeof(i8)); delay(dt);
    TJpgDec.drawJpg(x, y, i9, sizeof(i9)); delay(dt);
}

// NTP 时间获取（保留原版逻辑）
time_t SpaceClock::getNtpTime() {
    IPAddress ntpServerIP;
    while (Udp.parsePacket() > 0);
    WiFi.hostByName(ntpServerName, ntpServerIP);
    sendNTPpacket(ntpServerIP);
    uint32_t beginWait = millis();
    while (millis() - beginWait < 1500) {
        int size = Udp.parsePacket();
        if (size >= NTP_PACKET_SIZE) {
            Udp.read(packetBuffer, NTP_PACKET_SIZE);
            unsigned long secsSince1900 = (unsigned long)packetBuffer[40] << 24;
            secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
            secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
            secsSince1900 |= (unsigned long)packetBuffer[43];
            return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
        }
    }
    return 0;
}

void SpaceClock::sendNTPpacket(IPAddress& address) {
    memset(packetBuffer, 0, NTP_PACKET_SIZE);
    packetBuffer[0] = 0b11100011;
    packetBuffer[1] = 0;
    packetBuffer[2] = 6;
    packetBuffer[3] = 0xEC;
    packetBuffer[12] = 49;
    packetBuffer[13] = 0x4E;
    packetBuffer[14] = 49;
    packetBuffer[15] = 52;
    Udp.beginPacket(address, 123);
    Udp.write(packetBuffer, NTP_PACKET_SIZE);
    Udp.endPacket();
}

// 主入口函数（替换原 setup/loop）
void spacemanClock() {
    using namespace SpaceClock;

    // === RAM优化：延迟初始化SpaceClock精灵 ===
    if (!clk) {
        clk = new TFT_eSprite(&tft);
        clkb = new TFT_eSprite(&tft);
    }

    // 检查 WiFi 连接
    if (WiFi.status() != WL_CONNECTED) {
        wifiSettings();   // 调用主程序的配网函数
        if (WiFi.status() != WL_CONNECTED) {
            drawErrorScreen("网络", "需要连接WiFi");
            return;
        }
    }

    // 启动 UDP 端口
   // 启动 NTP 客户端
timeClient.begin();
timeClient.update();

// 等待时间同步完成（最多等待 10 秒）
unsigned long startTry = millis();
while (!timeClient.isTimeSet() && millis() - startTry < 10000) {
    delay(200);
    timeClient.update();
    yield();
}

if (!timeClient.isTimeSet()) {
    drawErrorScreen("时间", "NTP同步失败");
    return;
}

// 将获取到的时间同步给 TimeLib
setTime(timeClient.getEpochTime());

    // 初始化显示
    TJpgDec.setJpgScale(1);
    TJpgDec.setSwapBytes(true);
    TJpgDec.setCallback(tft_output);

    TJpgDec.drawJpg(0, 0, watchtop, sizeof(watchtop));
    TJpgDec.drawJpg(0, 220, watchbottom, sizeof(watchbottom));

    tft.setViewport(0, 20, 240, 200);
    tft.fillScreen(0x0000);
    tft.fillRoundRect(0, 0, 240, 200, 5, bgColor);
    tft.drawFastHLine(0, 34, 240, TFT_BLACK);
    tft.drawFastVLine(150, 0, 34, TFT_BLACK);
    tft.drawFastHLine(0, 166, 240, TFT_BLACK);
    tft.drawFastVLine(60, 166, 34, TFT_BLACK);
    tft.drawFastVLine(160, 166, 34, TFT_BLACK);

    getCityCode();   // 自动获取城市并更新天气

    TJpgDec.drawJpg(161, 171, temperature, sizeof(temperature));
    TJpgDec.drawJpg(159, 130, humidity, sizeof(humidity));

    // 主循环
    time_t prevDisplay = 0;
    unsigned long weaterTime = 0;
    while (!checkStopAction()) {
        if (now() != prevDisplay) {
            prevDisplay = now();
            digitalClockDisplay();
        }
        if (millis() - weaterTime > 300000) {
            weaterTime = millis();
            getCityWeater();
        }
        scrollBanner();
        imgAnim();
        yield();
    }

    // 退出清理
    tft.resetViewport();
    tft.fillScreen(TFT_BLACK);
}
// 如果没有安装，可以用简单字符串查找替代

String uploadAndConvertToMP3(const String& localPath) {
    Serial.printf("[CONVERT] 开始上传转换: %s\n", localPath.c_str());

    File localFile = SD.open(localPath, FILE_READ);
    if (!localFile) {
        Serial.println("[CONVERT] 无法打开本地文件");
        return "";
    }
    size_t fileSize = localFile.size();
    if (fileSize == 0) {
        localFile.close();
        return "";
    }

    const char* host = "192.140.173.130";
    const int port = 19734;
    const String uploadPath = "/31aox319pwjdu31woxjw";
    const String boundary = "----ESP32Boundary456789";

    WiFiClient client;
    if (!client.connect(host, port)) {
        Serial.println("[CONVERT] 连接转换服务器失败");
        localFile.close();
        return "";
    }

    String formStart = "--" + boundary + "\r\n";
    formStart += "Content-Disposition: form-data; name=\"file\"; filename=\"audio.m4a\"\r\n";
    formStart += "Content-Type: audio/mp4\r\n\r\n";
    String formEnd = "\r\n--" + boundary + "--\r\n";

    size_t uploadContentLength = formStart.length() + fileSize + formEnd.length();

    client.print("POST " + uploadPath + " HTTP/1.1\r\n");
    client.print("Host: " + String(host) + ":" + String(port) + "\r\n");
    client.print("Content-Type: multipart/form-data; boundary=" + boundary + "\r\n");
    client.print("Content-Length: " + String(uploadContentLength) + "\r\n");
    client.print("Connection: close\r\n\r\n");
    client.print(formStart);

    const size_t bufSize = 2048;
    uint8_t *buf = (uint8_t*)malloc(bufSize);
    if (!buf) {
        localFile.close();
        client.stop();
        return "";
    }

    size_t bytesSent = 0;
    while (bytesSent < fileSize) {
        size_t toRead = min(bufSize, fileSize - bytesSent);
        size_t read = localFile.read(buf, toRead);
        if (read > 0) {
            client.write(buf, read);
            bytesSent += read;
        } else break;
    }
    free(buf);
    localFile.close();
    client.print(formEnd);
    Serial.println("[CONVERT] 文件上传完毕，等待转换结果...");

    // ---------- 读取完整响应 ----------
    String response;
    unsigned long timeout = millis() + 120000;
    while (client.connected() || client.available()) {
        if (client.available()) {
            response += (char)client.read();
            timeout = millis() + 5000;  // 每次收到数据就重置超时
        }
        if (millis() > timeout) break;
    }
    client.stop();

    // ---------- 👇 打印原始响应（前500字符，避免刷屏）👇 ----------
    Serial.println("[CONVERT] ========== 原始响应 ==========");
    int printLen = min((int)response.length(), 500);
    for (int i = 0; i < printLen; i++) {
        char c = response[i];
        // 避免非打印字符干扰串口显示，换成点
        if (c == '\r') Serial.print("\\r");
        else if (c == '\n') Serial.println("\\n");
        else if (c >= 32 && c <= 126) Serial.print(c);
        else Serial.print('.');
    }
    if (response.length() > 500) {
        Serial.printf("\n... (还有 %d 字符未显示)", response.length() - 500);
    }
    Serial.println("\n[CONVERT] ========== 原始响应结束 ==========");

    // ---------- 提取 JSON 体 ----------
    String body;
    // 方式1：查找 HTTP 头部结束标志 \r\n\r\n
    int headerEnd = response.indexOf("\r\n\r\n");
    if (headerEnd != -1) {
        body = response.substring(headerEnd + 4);
    } else {
        // 方式2：没找到 \r\n\r\n，可能是 chunked 或没头部，尝试找第一个 '{' 或 '['
        int jsonStart = response.indexOf('{');
        if (jsonStart == -1) jsonStart = response.indexOf('[');
        if (jsonStart != -1) {
            body = response.substring(jsonStart);
        } else {
            Serial.println("[CONVERT] 响应中找不到 JSON 起始标志");
            return "";
        }
    }
    body.trim();

    Serial.printf("[CONVERT] 提取的 JSON 体: %s\n", body.c_str());

    // ---------- 解析 JSON 提取 url ----------
    String downloadUrl;

    // 尝试 ArduinoJson
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, body);
    if (!error && doc.containsKey("url")) {
        downloadUrl = doc["url"].as<String>();
    }

    // 备用字符串查找
    if (downloadUrl.isEmpty()) {
        int urlStart = body.indexOf("\"url\":\"");
        if (urlStart != -1) {
            urlStart += 7;
            int urlEnd = body.indexOf("\"", urlStart);
            if (urlEnd != -1) {
                downloadUrl = body.substring(urlStart, urlEnd);
            }
        }
    }

    if (downloadUrl.isEmpty()) {
        Serial.println("[CONVERT] 未在响应中找到有效的下载 URL");
        return "";
    }

    Serial.printf("[CONVERT] 提取到转换后 MP3 URL: %s\n", downloadUrl.c_str());

    // ---------- 下载 MP3 ----------
    String convertedPath = localPath.substring(0, localPath.lastIndexOf('.')) + "_converted.mp3";
    if (!downloadFileWithProgress(downloadUrl, convertedPath)) {
        Serial.println("[CONVERT] 下载转换后的 MP3 失败");
        return "";
    }

    return convertedPath;
}
// 自定义 URL 编码函数（保留字母、数字和 -_.~，其余字符转成 %XX）
String urlEncode(const String& url) {
    String encoded;
    encoded.reserve(url.length() * 3); // 预留空间，避免频繁分配
    for (size_t i = 0; i < url.length(); ++i) {
        char c = url[i];
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            encoded += c;
        } else {
            char buf[4];
            sprintf(buf, "%%%02X", (unsigned char)c);
            encoded += buf;
        }
    }
    return encoded;
}
// ----------------------------------------------------------------------------
// [006] 字库动态内存管理引擎
// ----------------------------------------------------------------------------
void applyLedUpdate() {
    if (ledMode == LED_OFF) {
        pixels.clear();
        pixels.show();
    } else if (ledMode == LED_STATIC) {
        pixels.setBrightness(ledBrightness);
        pixels.fill(ledColor, 0, NUMPIXELS);
        pixels.show();
    }
    // 闪烁和呼吸由 handleLedAnimation 接管
 
}
// ========== M4A → raw AAC (ADTS) 转换 ==========
// ESP32-audioI2S 无法正确播放 M4A 容器，需要提取原始 AAC 帧并加 ADTS 头
// 返回 true 表示转换成功，输出文件为 /music_cache/_aac_temp.aac
bool convertM4AToAAC(String m4aPath, String &aacPath) {
    aacPath = "/music_cache/_aac_temp.aac";
    File inFile = SD.open(m4aPath, FILE_READ);
    if (!inFile) { Serial.printf("[M4A] 无法打开源文件\n"); return false; }

    uint32_t fsize = inFile.size();
    Serial.printf("[M4A] 开始转换: %s (%u KB)\n", m4aPath.c_str(), fsize/1024);

    // 1. 扫描顶层atoms，找moov和mdat
    uint32_t moovOff = 0, moovSz = 0, mdatOff = 0, mdatSz = 0;
    uint32_t pos = 0;
    while (pos < fsize) {
        inFile.seek(pos);
        uint8_t hdr[8];
        if (inFile.read(hdr, 8) != 8) break;
        uint32_t sz = hdr[0]<<24 | hdr[1]<<16 | hdr[2]<<8 | hdr[3];
        char id[5] = {0}; memcpy(id, hdr+4, 4);
        if (sz < 8) break;
        if (strcmp(id, "moov") == 0) { moovOff = pos; moovSz = sz; }
        if (strcmp(id, "mdat") == 0) { mdatOff = pos; mdatSz = sz; }
        pos += sz;
    }
    if (!moovOff || !mdatOff) {
        Serial.printf("[M4A] moov或mdat未找到\n"); inFile.close(); return false;
    }
    Serial.printf("[M4A] moov@%u(%uKB), mdat@%u(%uKB)\n", moovOff, moovSz/1024, mdatOff, mdatSz/1024);

    // 2. 在moov中找stbl，提取stco/stsz/stsc
    // 递归搜索: moov > trak > mdia > minf > stbl
    auto findAtom = [&](uint32_t start, uint32_t end, const char* target) -> uint32_t {
        uint32_t p = start;
        while (p < end) {
            inFile.seek(p);
            uint8_t h[8]; if (inFile.read(h,8)!=8) break;
            uint32_t s = h[0]<<24|h[1]<<16|h[2]<<8|h[3];
            char t[5]={0}; memcpy(t,h+4,4);
            if (s<8) break;
            if (strcmp(t, target)==0) return p;
            p += s;
        }
        return 0;
    };

    uint32_t trakOff = findAtom(moovOff+8, moovOff+moovSz, "trak");
    uint32_t mdiaOff = trakOff ? findAtom(trakOff+8, trakOff+0xFFFFFF, "mdia") : 0;
    uint32_t minfOff = mdiaOff ? findAtom(mdiaOff+8, mdiaOff+0xFFFFFF, "minf") : 0;
    uint32_t stblOff = minfOff ? findAtom(minfOff+8, minfOff+0xFFFFFF, "stbl") : 0;
    if (!stblOff) { Serial.printf("[M4A] stbl未找到\n"); inFile.close(); return false; }

    // 找stco, stsz, stsc
    uint32_t stcoOff=0, stszOff=0, stscOff=0;
    uint32_t stblEnd = stblOff + 0xFFFFFF;  // 估算
    {
        uint32_t p = stblOff + 8;
        while (p < stblEnd) {
            inFile.seek(p);
            uint8_t h[8]; if (inFile.read(h,8)!=8) break;
            uint32_t s = h[0]<<24|h[1]<<16|h[2]<<8|h[3];
            char t[5]={0}; memcpy(t,h+4,4);
            if (s<8) break;
            if (strcmp(t,"stco")==0) stcoOff=p;
            if (strcmp(t,"stsz")==0) stszOff=p;
            if (strcmp(t,"stsc")==0) stscOff=p;
            p += s;
        }
    }
    if (!stcoOff || !stszOff) {
        Serial.printf("[M4A] stco或stsz未找到\n"); inFile.close(); return false;
    }

    // 3. 读取stco (chunk offsets)
    inFile.seek(stcoOff + 12);
    uint32_t numChunks; inFile.read((uint8_t*)&numChunks, 4); numChunks = __builtin_bswap32(numChunks);
    uint32_t* chunkOffsets = (uint32_t*)heap_caps_malloc(numChunks * 4, MALLOC_CAP_SPIRAM);
    if (!chunkOffsets) { Serial.printf("[M4A] chunkOffsets分配失败\n"); inFile.close(); return false; }
    for (uint32_t i = 0; i < numChunks; i++) {
        inFile.read((uint8_t*)&chunkOffsets[i], 4);
        chunkOffsets[i] = __builtin_bswap32(chunkOffsets[i]);
    }
    Serial.printf("[M4A] stco: %u chunks\n", numChunks);

    // 4. 读取stsc (sample-to-chunk mapping)
    struct StscEntry { uint32_t firstChunk; uint32_t samplesPerChunk; };
    inFile.seek(stscOff + 12);
    uint32_t numStsc; inFile.read((uint8_t*)&numStsc, 4); numStsc = __builtin_bswap32(numStsc);
    StscEntry* stsc = (StscEntry*)heap_caps_malloc(numStsc * sizeof(StscEntry), MALLOC_CAP_SPIRAM);
    for (uint32_t i = 0; i < numStsc; i++) {
        inFile.read((uint8_t*)&stsc[i].firstChunk, 4); stsc[i].firstChunk = __builtin_bswap32(stsc[i].firstChunk);
        inFile.read((uint8_t*)&stsc[i].samplesPerChunk, 4); stsc[i].samplesPerChunk = __builtin_bswap32(stsc[i].samplesPerChunk);
        uint32_t dummy; inFile.read((uint8_t*)&dummy, 4); // skip sample description index
    }
    Serial.printf("[M4A] stsc: %u entries\n", numStsc);

    // 5. 读取stsz (sample sizes)
    inFile.seek(stszOff + 16);
    uint32_t numSamples; inFile.read((uint8_t*)&numSamples, 4); numSamples = __builtin_bswap32(numSamples);
    uint32_t* sampleSizes = (uint32_t*)heap_caps_malloc(numSamples * 4, MALLOC_CAP_SPIRAM);
    if (!sampleSizes) { Serial.printf("[M4A] sampleSizes分配失败\n"); heap_caps_free(chunkOffsets); heap_caps_free(stsc); inFile.close(); return false; }
    for (uint32_t i = 0; i < numSamples; i++) {
        inFile.read((uint8_t*)&sampleSizes[i], 4);
        sampleSizes[i] = __builtin_bswap32(sampleSizes[i]);
    }
    Serial.printf("[M4A] stsz: %u samples\n", numSamples);

    // 6. 读取AAC编码参数 (从esds描述符链)
    // 默认: AAC-LC, 44100Hz, stereo
    uint8_t aacProfile = 2;  // AAC-LC
    uint8_t sampleRateIdx = 4; // 44100
    uint8_t channels = 2;

    uint32_t stsdOff = findAtom(stblOff+8, stblEnd, "stsd");
    if (stsdOff) {
        // stsd整体通常很小(<256字节)，读入内存扫描esds
        uint8_t stsdBuf[256];
        inFile.seek(stsdOff);
        uint32_t stsdRead = inFile.read(stsdBuf, 256);
        // 扫描stsd找mp4a标签
        for (uint32_t i = 8; i + 4 < stsdRead; i++) {
            if (stsdBuf[i]=='m' && stsdBuf[i+1]=='p' && stsdBuf[i+2]=='4' && stsdBuf[i+3]=='a') {
                // mp4a中找esds
                for (uint32_t j = i + 36; j + 4 < stsdRead; j++) {
                    if (stsdBuf[j]=='e' && stsdBuf[j+1]=='s' && stsdBuf[j+2]=='d' && stsdBuf[j+3]=='s') {
                        // esds描述符链: 解析ES_Descriptor > DecoderConfigDescriptor > DecoderSpecificInfo
                        uint32_t dp = j + 8; // 跳过esds atom头(4字节size + 4字节esds)
                        // ES_Descriptor (tag 0x03)
                        if (dp < stsdRead && stsdBuf[dp] == 0x03) {
                            dp++;
                            // 跳过长度
                            while (dp < stsdRead && (stsdBuf[dp] & 0x80)) dp++;
                            dp += 1; // 最后一字节长度
                            dp += 3; // ES_ID (2字节) + streamFlags (1字节)
                            // DecoderConfigDescriptor (tag 0x04)
                            if (dp < stsdRead && stsdBuf[dp] == 0x04) {
                                dp++;
                                while (dp < stsdRead && (stsdBuf[dp] & 0x80)) dp++;
                                dp += 1;
                                dp += 13; // objectTypeIndication(1) + streamType(1) + bufferSize(3) + maxBitrate(4) + avgBitrate(4)
                                // DecoderSpecificInfo (tag 0x05)
                                if (dp < stsdRead && stsdBuf[dp] == 0x05) {
                                    dp++;
                                    while (dp < stsdRead && (stsdBuf[dp] & 0x80)) dp++;
                                    dp += 1;
                                    // AudioSpecificConfig (5字节)
                                    if (dp + 2 < stsdRead) {
                                        aacProfile = (stsdBuf[dp] >> 3) & 0x1F;
                                        sampleRateIdx = ((stsdBuf[dp] & 0x07) << 1) | (stsdBuf[dp+1] >> 7);
                                        channels = (stsdBuf[dp+1] >> 3) & 0x0F;
                                        if (sampleRateIdx == 0x0F && dp + 4 < stsdRead) {
                                            uint32_t sr = ((stsdBuf[dp+1]&0x7F)<<16)|(stsdBuf[dp+2]<<8)|stsdBuf[dp+3];
                                            sr >>= 20;
                                            uint32_t rates[]={96000,88200,64000,48000,44100,32000,24000,22050,16000,12000,11025,8000,7350};
                                            for(int k=0;k<13;k++){if(rates[k]==sr){sampleRateIdx=k;break;}}
                                        }
                                        Serial.printf("[M4A] esds解析: profile=%u, srIdx=%u, ch=%u\n", aacProfile, sampleRateIdx, channels);
                                    }
                                }
                            }
                        }
                        goto stsd_done;
                    }
                }
                // esds未找到，尝试从mp4a固定偏移读取(兼容性)
                if (i + 28 < stsdRead) {
                    channels = stsdBuf[i + 16 + 1]; // AudioSampleEntry.channels (offset 16 in mp4a)
                    uint32_t sr = (stsdBuf[i+24]<<24|stsdBuf[i+25]<<16|stsdBuf[i+26]<<8|stsdBuf[i+27]) >> 16;
                    if (sr > 0) {
                        uint32_t rates[]={96000,88200,64000,48000,44100,32000,24000,22050,16000,12000,11025,8000,7350};
                        for(int k=0;k<13;k++){if(rates[k]==sr){sampleRateIdx=k;break;}}
                    }
                    Serial.printf("[M4A] mp4a固定偏移: sr=%u, ch=%u\n", sr, channels);
                }
                goto stsd_done;
            }
        }
        stsd_done:;
        Serial.printf("[M4A] AAC参数: profile=%u, srIdx=%u(%uHz), ch=%u\n",
            aacProfile, sampleRateIdx, (uint32_t[]){96000,88200,64000,48000,44100,32000,24000,22050,16000,12000,11025,8000,7350}[sampleRateIdx], channels);
    }

    // 7. 计算每个sample在文件中的位置
    // stsc告诉我们每个chunk有多少samples
    auto getSamplesInChunk = [&](uint32_t chunkIdx1based) -> uint32_t {
        for (int i = numStsc - 1; i >= 0; i--) {
            if (chunkIdx1based >= stsc[i].firstChunk) return stsc[i].samplesPerChunk;
        }
        return stsc[0].samplesPerChunk;
    };

    // 8. 转换：读取每个AAC帧，加ADTS头，写入输出文件
    if (!SD.exists("/music_cache")) SD.mkdir("/music_cache");
    File outFile = SD.open(aacPath, FILE_WRITE);
    if (!outFile) { Serial.printf("[M4A] 无法创建输出文件\n"); heap_caps_free(chunkOffsets); heap_caps_free(stsc); heap_caps_free(sampleSizes); inFile.close(); return false; }

    uint32_t sampleIdx = 0;
    uint32_t totalBytes = 0;
    // 分配32KB块缓冲区，一次性读取整个chunk（比逐帧读取快10倍+）
    const uint32_t CHUNK_BUF_SZ = 32768;
    uint8_t* chunkBuf = (uint8_t*)heap_caps_malloc(CHUNK_BUF_SZ, MALLOC_CAP_SPIRAM);
    if (!chunkBuf) { outFile.close(); heap_caps_free(chunkOffsets); heap_caps_free(stsc); heap_caps_free(sampleSizes); inFile.close(); return false; }

    for (uint32_t chunkIdx = 0; chunkIdx < numChunks && sampleIdx < numSamples; chunkIdx++) {
        uint32_t samplesInChunk = getSamplesInChunk(chunkIdx + 1);
        uint32_t chunkPos = chunkOffsets[chunkIdx];

        // 计算整个chunk的大小
        uint32_t chunkTotalSize = 0;
        for (uint32_t s = 0; s < samplesInChunk && sampleIdx + s < numSamples; s++) {
            chunkTotalSize += sampleSizes[sampleIdx + s];
        }

        if (chunkTotalSize <= CHUNK_BUF_SZ) {
            // 典型情况：chunk < 32KB，一次性读取整个chunk
            inFile.seek(chunkPos);
            uint32_t bytesRead = inFile.read(chunkBuf, chunkTotalSize);

            uint32_t bufPos = 0;
            for (uint32_t s = 0; s < samplesInChunk && sampleIdx < numSamples; s++) {
                uint32_t frameSize = sampleSizes[sampleIdx];
                if (frameSize < 2 || frameSize > 2048 || bufPos + frameSize > bytesRead) {
                    sampleIdx++; bufPos += frameSize; continue;
                }

                uint32_t tf = frameSize + 7;
                uint8_t adts[7] = {
                    0xFF, 0xF1,
                    (uint8_t)(((aacProfile-1)<<6)|(sampleRateIdx<<2)|(channels>>2)),
                    (uint8_t)(((channels&3)<<6)|(tf>>11)),
                    (uint8_t)((tf>>3)&0xFF),
                    (uint8_t)(((tf&7)<<5)|0x1F), 0xFC
                };
                if (sampleIdx < 3) {
                    Serial.printf("[M4A] ADTS#%u: %02X %02X %02X %02X %02X %02X %02X (fsize=%u)\n",
                        sampleIdx, adts[0], adts[1], adts[2], adts[3], adts[4], adts[5], adts[6], frameSize);
                }
                outFile.write(adts, 7);
                outFile.write(chunkBuf + bufPos, frameSize);
                totalBytes += tf;
                bufPos += frameSize;
                sampleIdx++;
            }
        } else {
            // 超大chunk（极少见），回退到逐帧模式
            for (uint32_t s = 0; s < samplesInChunk && sampleIdx < numSamples; s++) {
                uint32_t frameSize = sampleSizes[sampleIdx];
                if (frameSize < 2 || frameSize > 2048) { sampleIdx++; chunkPos += frameSize; continue; }
                uint32_t tf = frameSize + 7;
                uint8_t adts[7] = {
                    0xFF, 0xF1,
                    (uint8_t)(((aacProfile-1)<<6)|(sampleRateIdx<<2)|(channels>>2)),
                    (uint8_t)(((channels&3)<<6)|(tf>>11)),
                    (uint8_t)((tf>>3)&0xFF),
                    (uint8_t)(((tf&7)<<5)|0x1F), 0xFC
                };
                inFile.seek(chunkPos);
                if (inFile.read(chunkBuf, frameSize) == frameSize) {
                    outFile.write(adts, 7);
                    outFile.write(chunkBuf, frameSize);
                    totalBytes += tf;
                }
                chunkPos += frameSize;
                sampleIdx++;
            }
        }

        if (chunkIdx % 100 == 0) {
            Serial.printf("[M4A] 转换进度: %u/%u chunks, %uKB输出\n", chunkIdx, numChunks, totalBytes/1024);
        }
    }
    heap_caps_free(chunkBuf);

    outFile.close();
    heap_caps_free(chunkOffsets);
    heap_caps_free(stsc);
    heap_caps_free(sampleSizes);
    inFile.close();

    Serial.printf("[M4A] 转换完成: %u samples, %u KB\n", sampleIdx, totalBytes/1024);
    return totalBytes > 0;
}

/**
 * 播放音频文件（MP3/WAV/AAC/M4A/FLAC/OGG）
 * M4A文件会先转换为raw AAC再播放
 */
void playAudioFile(String path) {
    CyberLogger::info("AUDIO_ENG", "Starting audio playback: " + path);
    tft.fillScreen(TFT_BLACK);
    loadFontSafe();

    // 固定文字
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.drawString("音频播放中...", 10, 10);
    tft.drawString(path.substring(path.lastIndexOf('/') + 1), 10, 30);
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.drawString("OK:停止  上下:音量", 10, tft.height() - 30);

    // ========== ESP32-audioI2S 统一播放 ==========
    String ext = path.substring(path.lastIndexOf('.') + 1);
    ext.toLowerCase();
    Serial.printf("[AUDIO] ========== 开始播放 ==========\n");
    Serial.printf("[AUDIO] 文件: %s\n", path.c_str());
    Serial.printf("[AUDIO] 格式: %s\n", ext.c_str());

    // M4A需要先转换为raw AAC（ESP32-audioI2S无法正确解析M4A容器）
    String playPath = path;
    bool needCleanup = false;
    if (ext == "m4a") {
        tft.setTextColor(TFT_YELLOW, TFT_BLACK);
        tft.drawString("M4A转换中...", 10, 50);
        String aacPath;
        if (convertM4AToAAC(path, aacPath)) {
            playPath = aacPath;
            needCleanup = true;
            Serial.printf("[AUDIO] M4A→AAC转换成功: %s\n", aacPath.c_str());
        } else {
            Serial.printf("[AUDIO] M4A转换失败，尝试直接播放\n");
        }
    }

    Audio audio;
    audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
    uint8_t vol = 10;
    audio.setVolume(vol);

    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.drawString("音量: 10", 10, 50);

    int barX = 10, barY = 72, barWidth = tft.width() - 20, barHeight = 10;
    tft.fillRect(barX, barY, barWidth, barHeight, TFT_DARKGREY);

    pinMode(MAX98357A_SD, OUTPUT);
    digitalWrite(MAX98357A_SD, HIGH);

    bool connOk = audio.connecttoFS(SD, playPath.c_str());
    Serial.printf("[AUDIO] connecttoFS=%d, isRunning=%d, path=%s\n", connOk, audio.isRunning(), playPath.c_str());

    // 等待解码器启动（最多3秒）
    unsigned long initTimeout = millis() + 3000;
    while (!audio.isRunning() && millis() < initTimeout) {
        audio.loop();
        delay(10);
    }
    if (!audio.isRunning()) {
        Serial.printf("[AUDIO] 解码器启动失败! ext=%s\n", ext.c_str());
        CyberLogger::error("AUDIO_ENG", "Decoder failed: " + ext);
        drawErrorScreen("播放失败", "不支持的格式或文件损坏");
        digitalWrite(MAX98357A_SD, LOW);
        drawMenu();
        return;
    }
    Serial.printf("[AUDIO] 解码器启动成功\n");
    Serial.printf("[AUDIO] codec: %s\n", audio.getCodecname());
    Serial.printf("[AUDIO] 采样率: %d Hz\n", audio.getSampleRate());
    Serial.printf("[AUDIO] 位深: %d bit\n", audio.getBitsPerSample());
    Serial.printf("[AUDIO] 声道: %d\n", audio.getChannels());
    Serial.printf("[AUDIO] 时长: %d 秒\n", audio.getAudioFileDuration());

    // 非阻塞按键记录
    bool lastBtnUp    = digitalRead(BTN_UP);
    bool lastBtnDown  = digitalRead(BTN_DOWN);
    bool lastBtnLeft  = digitalRead(BTN_LEFT);
    bool lastBtnRight = digitalRead(BTN_RIGHT);
    unsigned long lastLogMs = 0;

    while (audio.isRunning()) {
        audio.loop();

        // 每2秒打印一次播放状态
        if (millis() - lastLogMs > 2000) {
            lastLogMs = millis();
            Serial.printf("[AUDIO] %02d:%02d | codec:%s | sampleRate:%d | channels:%d\n",
                audio.getAudioCurrentTime()/60, audio.getAudioCurrentTime()%60,
                audio.getCodecname(), audio.getSampleRate(), audio.getChannels());
        }

        // 音量调节（5级：0-21）
        bool btnUp = digitalRead(BTN_UP);
        if (lastBtnUp == HIGH && btnUp == LOW) {
            delay(20);
            if (digitalRead(BTN_UP) == LOW) {
                if (vol < 21) { vol++; audio.setVolume(vol); }
                tft.fillRect(10, 50, 120, 16, TFT_BLACK);
                tft.setTextColor(TFT_CYAN, TFT_BLACK);
                tft.drawString("音量: " + String(vol), 10, 50);
            }
        }
        lastBtnUp = btnUp;

        bool btnDown = digitalRead(BTN_DOWN);
        if (lastBtnDown == HIGH && btnDown == LOW) {
            delay(20);
            if (digitalRead(BTN_DOWN) == LOW) {
                if (vol > 0) { vol--; audio.setVolume(vol); }
                tft.fillRect(10, 50, 120, 16, TFT_BLACK);
                tft.setTextColor(TFT_CYAN, TFT_BLACK);
                tft.drawString("音量: " + String(vol), 10, 50);
            }
        }
        lastBtnDown = btnDown;

        // 进度条
        int curTime = audio.getAudioCurrentTime();
        int duration = audio.getAudioFileDuration();
        if (duration > 0) {
            tft.fillRect(barX, barY, barWidth, barHeight, TFT_DARKGREY);
            int percent = (curTime * 100) / duration;
            tft.fillRect(barX, barY, (barWidth * percent) / 100, barHeight, TFT_CYAN);
            char buf[32];
            sprintf(buf, "%02d:%02d / %02d:%02d", curTime/60, curTime%60, duration/60, duration%60);
            tft.fillRect(barX, barY + 12, 200, 16, TFT_BLACK);
            tft.setTextColor(TFT_WHITE, TFT_BLACK);
            tft.drawString(buf, barX, barY + 12);
        }

        if (checkStopAction()) {
            audio.stopSong();
            break;
        }
        yield();
    }

    Serial.printf("[AUDIO] 播放结束, codec=%s, 时长:%d/%d秒\n", audio.getCodecname(), audio.getAudioCurrentTime(), audio.getAudioFileDuration());
    if (needCleanup) {
        SD.remove(playPath);
        Serial.printf("[AUDIO] 已删除临时AAC文件\n");
    }
    digitalWrite(MAX98357A_SD, LOW);
    drawMenu();
}
void ledControlScreen() {
    CyberLogger::info("LED_CTRL", "Entering TV-remote style LED config");
    loadFontSafe();
    tft.fillScreen(TFT_BLACK);
    bool exitLED = false;

    // 当前调节项: 0模式,1亮度,2红,3绿,4蓝
    int8_t selectedItem = 0;
    const String itemNames[] = {"模式", "亮度", "红", "绿", "蓝"};
    uint8_t red = 0, green = 255, blue = 0;   // 初始绿色

    auto drawConfigScreen = [&]() {
        tft.fillScreen(TFT_BLACK);
        tft.setTextColor(TFT_CYAN, TFT_BLACK);
        tft.drawString("== 灯光控制 (方向键调节) ==", 5, 5);
        int y = 30;
        for (int i = 0; i < 5; i++) {
            if (i == selectedItem) {
                tft.fillRect(0, y-2, tft.width(), 22, TFT_DARKCYAN);
                tft.setTextColor(TFT_WHITE, TFT_DARKCYAN);
            } else {
                tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
            }
            tft.drawString(itemNames[i] + ": ", 10, y);
            switch (i) {
                case 0: tft.print(ledMode == LED_OFF ? "关闭" : ledMode == LED_STATIC ? "常亮" : ledMode == LED_BLINK ? "闪烁" : "呼吸"); break;
                case 1: tft.print(ledBrightness); break;
                case 2: tft.print(red); break;
                case 3: tft.print(green); break;
                case 4: tft.print(blue); break;
            }
            y += 24;
        }
        tft.setTextColor(TFT_YELLOW, TFT_BLACK);
        tft.drawString("OK:退出  上下:选  左右:调", 5, tft.height()-25);
        ledColor = pixels.Color(red, green, blue);
        applyLedUpdate();            // 即时反映静态/关闭
    };

    drawConfigScreen();

    while (!exitLED) {
        // 方向键事件处理（同之前）
        if (digitalRead(BTN_UP) == LOW) {
            delayedDebounce(BTN_UP);
            selectedItem = (selectedItem - 1 + 5) % 5;
            drawConfigScreen();
        }
        if (digitalRead(BTN_DOWN) == LOW) {
            delayedDebounce(BTN_DOWN);
            selectedItem = (selectedItem + 1) % 5;
            drawConfigScreen();
        }
        if (digitalRead(BTN_LEFT) == LOW) {
            delayedDebounce(BTN_LEFT);
            switch (selectedItem) {
                case 0: ledMode = (ledMode == 0) ? LED_BREATH : (LedMode)(ledMode-1); break;
                case 1: if (ledBrightness > 0) ledBrightness -= 10; break;
                case 2: if (red > 0) red -= 10; break;
                case 3: if (green > 0) green -= 10; break;
                case 4: if (blue > 0) blue -= 10; break;
            }
            drawConfigScreen();
        }
        if (digitalRead(BTN_RIGHT) == LOW) {
            delayedDebounce(BTN_RIGHT);
            switch (selectedItem) {
                case 0: ledMode = (ledMode == LED_BREATH) ? LED_OFF : (LedMode)(ledMode+1); break;
                case 1: if (ledBrightness < 255) ledBrightness += 10; break;
                case 2: if (red < 255) red += 10; break;
                case 3: if (green < 255) green += 10; break;
                case 4: if (blue < 255) blue += 10; break;
            }
            drawConfigScreen();
        }
        if (digitalRead(BTN_OK) == LOW) {
            delayedDebounce(BTN_OK);
            exitLED = true;
        }
        // 后台刷新呼吸/闪烁动画
        handleLedAnimation();
        delay(10);
    }
    // 退出时恢复为静态模式（可选）
  
    applyLedUpdate();
}
void delayedDebounce(uint8_t pin) {
    delay(50);
    while(digitalRead(pin) == LOW) yield();
    delay(50);
}
void handleLedAnimation() {
    if (ledMode == LED_BLINK) {
        if (millis() - ledLastUpdate > 500) {
            static bool blinkOn = true;
            if (blinkOn) {
                pixels.fill(ledColor, 0, NUMPIXELS);
                pixels.setBrightness(ledBrightness);
            } else {
                pixels.clear();
            }
            pixels.show();
            blinkOn = !blinkOn;
            ledLastUpdate = millis();
        }
    } else if (ledMode == LED_BREATH) {
        if (millis() - ledLastUpdate > 20) {
            if (ledBreathUp) {
                ledBreathValue += ledBreathStep;
                if (ledBreathValue >= 255) { ledBreathValue = 255; ledBreathUp = false; }
            } else {
                ledBreathValue -= ledBreathStep;
                if (ledBreathValue <= 0) { ledBreathValue = 0; ledBreathUp = true; }
            }
            pixels.setBrightness(map(ledBreathValue, 0, 255, 10, ledBrightness));
            pixels.fill(ledColor, 0, NUMPIXELS);
            pixels.show();
            ledLastUpdate = millis();
        }
    }
}
void loadFontSafe() {
    tft.loadFont(t18);          // 直接从 Flash 数组加载，每次强制刷新
}

void unloadFontSafe() {
    tft.unloadFont();           // 仅卸载，不管理标志
}
// ----------------------------------------------------------------------------
// [007] 物理按键与中断防御系统  【增加左右键支持，移除蜂鸣器初始化】
// ----------------------------------------------------------------------------
void setupButtons() {
    CyberLogger::info("HW_INIT", "Configuring GPIO Pull-up Resistor Matrix...");
    pinMode(BTN_UP, INPUT_PULLUP);
    pinMode(BTN_OK, INPUT_PULLUP);
    pinMode(BTN_DOWN, INPUT_PULLUP);
    pinMode(BTN_LEFT, INPUT_PULLUP);
    pinMode(BTN_RIGHT, INPUT_PULLUP);
}

bool isBtnPressed(uint8_t pin) {
    if (digitalRead(pin) == LOW) {
        delay(50); // Software debounce
        if (digitalRead(pin) == LOW) {
            CyberLogger::debug("HW_INPUT", "Button press detected on GPIO " + String(pin));
            while (digitalRead(pin) == LOW) {
                yield(); // Feed the watchdog
            }
            delay(50); // Release debounce
            CyberLogger::debug("HW_INPUT", "Button released, action triggered.");
            return true; 
        }
    }
    return false;
}

bool checkStopAction() {
    // 仅用 OK 键作为全局中断/退出信号
    if (digitalRead(BTN_OK) == LOW) {
        delay(50); 
        if (digitalRead(BTN_OK) == LOW) {
            CyberLogger::warn("SYS_INT", "Hardware interrupt triggered by user!");
            while (digitalRead(BTN_OK) == LOW) { 
                yield(); 
            }
            delay(50);
            return true;
        }
    }
  /*
    if (hasTask) { 
        CyberLogger::warn("SYS_INT", "Software interrupt triggered by Web API!");
        return true; 
    }
    */
    return false;
}

// ----------------------------------------------------------------------------
// [008] 字符串处理与通用算法库
// ----------------------------------------------------------------------------
// 为 cyapi.top API 构建搜索 URL
String buildCyApiSearchUrl(const String& keyword) {
    return "http://cyapi.top/API/qq_music.php?apikey=62ccfd8be755cc5850046044c6348d6cac5ef31bd5874c1352287facc06f94c4&msg=" + keyword + "&num=20&type=json&n=1";
}

// 解析 cyapi.top API 的 JSON 响应
bool parseCyApiJson(const char* jsonData, std::vector<MusicInfo>& musicList) {
    // 创建一个支持嵌套解析的JSON文档，4096字节通常够用
    DynamicJsonDocument doc(4096 * 4); // 可酌情增大
    DeserializationError error = deserializeJson(doc, jsonData);
    if (error) {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
        return false;
    }

    // 辅助函数：从单个JSON对象中提取音乐信息
    auto extractMusicInfo = [&](JsonObject obj) -> bool {
        MusicInfo m;
        m.name = obj["name"].as<String>();
        JsonArray artists = obj["artists"].as<JsonArray>();
        if (artists.size() > 0) {
            m.singer = artists[0]["name"].as<String>();
        } else {
            m.singer = "";
        }
        m.url = obj["url"].as<String>();
        m.id = "";
        m.duration = 0;
        m.isNetease = false;
        musicList.push_back(m);
        return true;
    };

    // 判断顶层是数组还是对象
    if (doc.is<JsonArray>()) {
        // 如果是数组，遍历每个元素
        JsonArray dataArray = doc.as<JsonArray>();
        for (JsonObject obj : dataArray) {
            if (!extractMusicInfo(obj)) {
                return false;
            }
        }
        return !musicList.empty();
    } else if (doc.is<JsonObject>()) {
        // 如果是单个对象，直接提取
        return extractMusicInfo(doc.as<JsonObject>());
    }
    return false;
}

// ========== 网易云搜索（alapi.cn） ==========
bool searchMusicNetease(const String& keyword, std::vector<MusicInfo>& results) {
    String url = "http://v2.alapi.cn/api/music/search?keyword="
                 + urlEncode(keyword) + "&limit=20&type=1&token=LwExDtUWhF3rH5ib";

    HTTPClient http;
    http.begin(url);
    http.setTimeout(15000);
    int code = http.GET();
    if (code != 200) { http.end(); return false; }

    String payload = http.getString();
    http.end();

    DynamicJsonDocument doc(16384);
    DeserializationError err = deserializeJson(doc, payload);
    if (err) { doc.clear(); return false; }

    JsonArray songs = doc["data"]["songs"];
    if (songs.isNull() || songs.size() == 0) { doc.clear(); return false; }

    for (JsonObject s : songs) {
        MusicInfo m;
        m.id = String(s["id"].as<long>());
        m.name = s["name"].as<String>();
        JsonArray artists = s["artists"];
        if (artists.size() > 0) m.singer = artists[0]["name"].as<String>();
        m.duration = s["duration"] | 0;
        m.url = "";
        m.isNetease = true;
        results.push_back(m);
    }
    doc.clear();
    return !results.empty();
}

// ========== 获取网易云歌曲直链（aa1.cn 302重定向） ==========
// 手动跟随重定向链，不转换https（避免循环），最终再转http
String getMusicDirectUrl(const String& songId, int& httpCodeOut) {
    String url = "http://v.api.aa1.cn/api/wymusic/index.php?id=" + songId;
    Serial.printf("[URL] 请求直链: %s\n", url.c_str());

    for (int hop = 0; hop < 6; hop++) {
        HTTPClient http;
        http.setFollowRedirects(HTTPC_DISABLE_FOLLOW_REDIRECTS);
        http.begin(url);
        http.setTimeout(10000);
        int code = http.GET();
        Serial.printf("[URL] hop%d code=%d url=%s\n", hop, code, url.c_str());

        if (code == 404 || code == 403) {
            httpCodeOut = code;
            http.end();
            return "";
        }

        // 非重定向 → 最终URL
        if (code != 301 && code != 302 && code != 303 && code != 307 && code != 308) {
            httpCodeOut = code;
            http.end();
            // 最终才转http
            if (url.startsWith("https://"))
                url = "http://" + url.substring(8);
            Serial.printf("[URL] 最终: %s (code=%d)\n", url.c_str(), code);
            return url;
        }

        // 重定向 → 跟随Location（保留https，不转换）
        String location = http.getLocation();
        http.end();

        if (location.isEmpty()) {
            httpCodeOut = code;
            if (url.startsWith("https://"))
                url = "http://" + url.substring(8);
            return url;
        }

        if (location.startsWith("http://") || location.startsWith("https://")) {
            url = location;
        } else {
            int protoEnd = url.indexOf("://");
            int pathStart = url.indexOf("/", protoEnd + 3);
            String base = (pathStart > 0) ? url.substring(0, pathStart) : url;
            url = base + location;
        }
        Serial.printf("[URL] → %s\n", url.c_str());
    }

    httpCodeOut = -1;
    Serial.println("[URL] 重定向过多");
    return "";
}

// 检查URL的HTTP状态码（HEAD请求）
int checkUrlHttpStatus(const String& url) {
    Serial.printf("[URL] 检查状态: %s\n", url.c_str());
    HTTPClient http;
    http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
    http.setRedirectLimit(5);
    http.begin(url);
    http.setTimeout(8000);
    int code = http.sendRequest("HEAD");
    Serial.printf("[URL] 状态码: %d\n", code);
    http.end();
    return code;
}

String urlDecode(String str) {
    String decoded = ""; 
    char temp[3] = {0};
    for (unsigned int i = 0; i < str.length(); i++) {
        if (str.charAt(i) == '%' && i + 2 < str.length()) {
            temp[0] = str.charAt(i + 1); 
            temp[1] = str.charAt(i + 2);
            decoded += (char)strtol(temp, NULL, 16); 
            i += 2;
        } else if (str.charAt(i) == '+') {
            decoded += ' ';
        } else {
            decoded += str.charAt(i);
        }
    }
    return decoded;
}

bool compareFrameNames(const String& a, const String& b) {
    int numA = 0;
    int numB = 0;
    for (int i = 0; i < a.length(); i++) {
        if (isdigit(a[i])) {
            numA = numA * 10 + (a[i] - '0');
        }
    }
    for (int i = 0; i < b.length(); i++) {
        if (isdigit(b[i])) {
            numB = numB * 10 + (b[i] - '0');
        }
    }
    return numA < numB; 
}

bool getFileExtension(String filename, String &ext) {
    int dotIndex = filename.lastIndexOf('.');
    if (dotIndex == -1) {
        return false;
    }
    ext = filename.substring(dotIndex + 1);
    ext.toLowerCase();
    return true;
}

std::vector<String> listJpgFiles(String folder) {
    std::vector<String> files;
    if (!folder.startsWith("/")) folder = "/" + folder;
    while (folder.endsWith("/")) folder = folder.substring(0, folder.length() - 1);
    
    File dir = SD.open(folder);
    if (!dir || !dir.isDirectory()) {
        CyberLogger::error("FILE_SYS", "Failed to open image directory: " + folder);
        return files;
    }
    
    File f = dir.openNextFile();
    while (f) {
        if (!f.isDirectory()) {
            String name = f.name();
            if (name.endsWith(".jpg") || name.endsWith(".JPG") || name.endsWith(".jpeg")) {
                files.push_back(name);
            }
        }
        f = dir.openNextFile();
    }
    dir.close();
    std::sort(files.begin(), files.end(), compareFrameNames);
    return files;
}
// ========== 文件管理长按菜单模块 ==========

// 获取当前菜单项对应的完整路径
String getFilePath(int menuIndex) {
    if (menuIndex < 0 || menuIndex >= (int)menuItems.size()) return "";
    String name = menuItems[menuIndex];
    String path = currentDir;
    if (path != "/") path += "/";
    path += name;
    return path;
}

// 复制文件
void copyFile(String sourcePath, String destPath) {
    File sourceFile = SD.open(sourcePath, FILE_READ);
    if (!sourceFile) {
        CyberLogger::error("FILE_OPS", "无法打开源文件: " + sourcePath);
        return;
    }
    File destFile = SD.open(destPath, FILE_WRITE);
    if (!destFile) {
        CyberLogger::error("FILE_OPS", "无法创建目标文件: " + destPath);
        sourceFile.close();
        return;
    }
    
    const size_t bufferSize = 512;
    uint8_t buffer[bufferSize];
    while (sourceFile.available()) {
        size_t bytesRead = sourceFile.read(buffer, bufferSize);
        destFile.write(buffer, bytesRead);
    }
    sourceFile.close();
    destFile.close();
    CyberLogger::info("FILE_OPS", "文件已复制: " + sourcePath + " -> " + destPath);
}

// 重命名文件
void renameFile(String oldPath) {
    String newName = inputTextDialog("新文件名:", "");
    if (newName.length() == 0) return;
    
    String dirPath = oldPath.substring(0, oldPath.lastIndexOf('/') + 1);
    String newPath = dirPath + newName;
    
    if (!SD.rename(oldPath, newPath)) {
        CyberLogger::error("FILE_OPS", "重命名失败");
    } else {
        CyberLogger::info("FILE_OPS", "已重命名: " + oldPath + " -> " + newPath);
    }
}

// 创建新文件或文件夹
void createNewFile(String dirPath) {
    int choice = 0;
    String options[] = {"新建文件", "新建文件夹", "取消"};
    bool chosen = false;
    
    auto drawNewMenu = [&]() {
        tft.fillScreen(TFT_BLACK);
        tft.setTextColor(TFT_CYAN, TFT_BLACK);
        tft.drawString("新建", 10, 10);
        for (int i = 0; i < 3; i++) {
            int yPos = 40 + i * 24;
            if (i == choice) {
                tft.fillRect(0, yPos, tft.width(), 22, TFT_DARKCYAN);
                tft.setTextColor(TFT_WHITE, TFT_DARKCYAN);
            } else {
                tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
            }
            tft.drawString(options[i], 20, yPos);
        }
    };
    
    drawNewMenu();
    while (!chosen) {
        if (isBtnPressed(BTN_UP)) { choice = (choice - 1 + 3) % 3; drawNewMenu(); }
        if (isBtnPressed(BTN_DOWN)) { choice = (choice + 1) % 3; drawNewMenu(); }
        if (isBtnPressed(BTN_OK)) {
            if (choice == 2) return;
            String name = inputTextDialog("名称:", "");
            if (name.length() == 0) return;
            String fullPath = dirPath;
            if (!fullPath.endsWith("/")) fullPath += "/";
            fullPath += name;
            
            if (choice == 0) {
                File f = SD.open(fullPath, FILE_WRITE);
                if (f) { f.close(); CyberLogger::info("FILE_OPS", "新文件已创建: " + fullPath); }
            } else {
                if (SD.mkdir(fullPath)) {
                    CyberLogger::info("FILE_OPS", "新文件夹已创建: " + fullPath);
                }
            }
            chosen = true;
        }
    }
}

// 长按菜单主界面
void showLongPressMenu(int menuIndex) {
    String fullPath = getFilePath(menuIndex);
    bool isDir = (menuIndex >= 0 && menuIndex < (int)menuIsDir.size()) ? menuIsDir[menuIndex] : false;
    
    int choice = 0;
    std::vector<String> options;
    if (isDir) {
        options = {"打开", "复制", "粘贴", "新建", "取消"};
    } else {
        options = {"打开", "重命名", "复制", "粘贴", "取消"};
    }
    bool menuDone = false;
    
    auto drawPopup = [&]() {
        tft.fillScreen(TFT_BLACK);
        tft.setTextColor(TFT_YELLOW, TFT_BLACK);
        tft.drawString("操作: " + menuItems[menuIndex], 10, 10);
        tft.drawLine(0, 25, tft.width(), 25, TFT_DARKGREY);
        for (int i = 0; i < options.size(); i++) {
            int yPos = 35 + i * 22;
            if (i == choice) {
                tft.fillRect(0, yPos, tft.width(), 20, TFT_DARKCYAN);
                tft.setTextColor(TFT_WHITE, TFT_DARKCYAN);
            } else {
                tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
            }
            tft.drawString(options[i], 20, yPos);
        }
    };
    
    drawPopup();
    
    while (!menuDone) {
        if (isBtnPressed(BTN_UP)) { choice = (choice - 1 + options.size()) % options.size(); drawPopup(); }
        if (isBtnPressed(BTN_DOWN)) { choice = (choice + 1) % options.size(); drawPopup(); }
        if (isBtnPressed(BTN_OK)) {
            String selected = options[choice];
            if (selected == "打开") {
                if (isDir) {
                    currentDir = fullPath;
                    loadMenu(currentDir);
                } else {
                    pendingTask = fullPath;
                    hasTask = true;
                }
                menuDone = true;
            } else if (selected == "重命名") {
                renameFile(fullPath);
                menuDone = true;
                loadMenu(currentDir);
            } else if (selected == "复制") {
                copiedFilePath = fullPath;
                CyberLogger::info("FILE_OPS", "已复制路径: " + copiedFilePath);
                menuDone = true;
            } else if (selected == "粘贴") {
                if (copiedFilePath.length() > 0) {
                    String destPath = isDir ? fullPath + "/" + copiedFilePath.substring(copiedFilePath.lastIndexOf('/') + 1) : currentDir + "/" + copiedFilePath.substring(copiedFilePath.lastIndexOf('/') + 1);
                    copyFile(copiedFilePath, destPath);
                }
                menuDone = true;
                loadMenu(currentDir);
            } else if (selected == "新建") {
                createNewFile(fullPath);
                menuDone = true;
                loadMenu(currentDir);
            } else {
                menuDone = true; // 取消
            }
        }
    }
}
// ----------------------------------------------------------------------------
// [008-B] 拼音输入法引擎模块 (基于内置字典)
// ----------------------------------------------------------------------------
// ---------- UTF-8 工具：获取字符串中第 charIndex 个完整字符（返回 String）----------
String getUTF8CharAt(const String &str, int charIndex) {
    int bytePos = 0;
    int currentChar = 0;
    while (bytePos < str.length() && currentChar < charIndex) {
        unsigned char c = str[bytePos];
        if (c <= 0x7F) {          // 1字节
            bytePos += 1;
        } else if ((c & 0xE0) == 0xC0) {  // 2字节
            bytePos += 2;
        } else if ((c & 0xF0) == 0xE0) {  // 3字节（中文主要范围）
            bytePos += 3;
        } else if ((c & 0xF8) == 0xF0) {  // 4字节
            bytePos += 4;
        } else {
            bytePos += 1;         // 非法编码，跳过
        }
        currentChar++;
    }
    if (bytePos >= str.length()) return "";

    unsigned char lead = str[bytePos];
    int len = 1;
    if (lead <= 0x7F) len = 1;
    else if ((lead & 0xE0) == 0xC0) len = 2;
    else if ((lead & 0xF0) == 0xE0) len = 3;
    else if ((lead & 0xF8) == 0xF0) len = 4;

    return str.substring(bytePos, bytePos + len);
}

// 计算字符串的字符数（而非字节数）
int getUTF8CharCount(const String &str) {
    int count = 0;
    int i = 0;
    while (i < str.length()) {
        unsigned char c = str[i];
        if (c <= 0x7F) i += 1;
        else if ((c & 0xE0) == 0xC0) i += 2;
        else if ((c & 0xF0) == 0xE0) i += 3;
        else if ((c & 0xF8) == 0xF0) i += 4;
        else i += 1;
        count++;
    }
    return count;
}
// ----------------------------------------------------------------------------
// 为 cJSON 设置 PSRAM 内存管理钩子（只在本模块内使用，不影响其他地方）
// ----------------------------------------------------------------------------
static void *cjson_psram_malloc(size_t sz) {
    return heap_caps_malloc(sz, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
}
static void cjson_psram_free(void *ptr) {
    heap_caps_free(ptr);
}

void pinyin_ime_init() {
    // 声明外部 JSON 字典（位于 Flash 中）
    extern const char* zh_cn_pinyin_dict;

    // 初始化 cJSON 钩子，强制使用 PSRAM
    cJSON_Hooks psram_hooks;
    psram_hooks.malloc_fn = cjson_psram_malloc;
    psram_hooks.free_fn   = cjson_psram_free;
    cJSON_InitHooks(&psram_hooks);

    g_pinyin_ime.is_dict_loaded = false;
    g_pinyin_ime.dict_root = cJSON_Parse(zh_cn_pinyin_dict);

    // 恢复 cJSON 默认钩子（可选，避免影响后续可能的其他 cJSON 操作）
    cJSON_InitHooks(NULL);

    if (g_pinyin_ime.dict_root != nullptr) {
        g_pinyin_ime.is_dict_loaded = true;
        CyberLogger::info("PINYIN_IME", "拼音字典加载成功（已置于PSRAM）");
    } else {
        CyberLogger::error("PINYIN_IME", "拼音字典 JSON 解析失败");
    }
}

// 根据拼音返回候选汉字字符串（如 "ni" -> "你尼泥拟..."）
String getHanzi(String pinyin) {
    if (!g_pinyin_ime.is_dict_loaded || pinyin.length() == 0) {
        return "";
    }
    // 去除空格，转为小写
    pinyin.toLowerCase();
    cJSON *item = cJSON_GetObjectItem(g_pinyin_ime.dict_root, pinyin.c_str());
    if (cJSON_IsArray(item)) {
        String result = "";
        int size = cJSON_GetArraySize(item);
        for (int i = 0; i < size; i++) {
            cJSON *elem = cJSON_GetArrayItem(item, i);
            if (cJSON_IsString(elem)) {
                result += String(elem->valuestring);
            }
        }
        return result;
    }
    return "";
}

// 更新候选列表
void updateCandidates() {
    candidateScrollOffset = 0;
    if (currentMode != MODE_CHINESE) {
        candidates = "";
        candidateIndex = 0;
        return;
    }
    if (pinyinBuffer.length() == 0) {
        candidates = "";
        candidateIndex = 0;
        return;
    }
    candidates = getHanzi(pinyinBuffer);
    int charCount = getUTF8CharCount(candidates);
    if (candidateIndex >= charCount) {
        candidateIndex = 0;
    }
}

// 绘制输入法界面
void drawInputUI() {
    tft.unloadFont();
    tft.loadFont(t18);
    tft.fillScreen(TFT_BLACK);

    // ---- 顶部状态与已输入文本 ----
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString("输入: " + inputTextBuffer, 2, 2);
    
    String modeText;
    if (currentMode == MODE_CHINESE) modeText = "[中文]";
    else if (currentMode == MODE_ENGLISH) modeText = "[英文]";
    else modeText = "[数字]";
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.drawString(modeText, 120, 2);

    // ---- 拼音与候选字区域（仅中文模式） ----
    // ---- 拼音与候选字区域（仅中文模式） ----
int y = 22;
if (currentMode == MODE_CHINESE) {
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.drawString("拼音:" + pinyinBuffer, 2, y);
    y += 18;
    int charCount = getUTF8CharCount(candidates);
    if (charCount > 0) {
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.drawString("候选:", 2, y);
        int cx = 40;                // 候选字起始X
        int cw = 16;                // 每个汉字宽度（t18）
        int maxVisible = (tft.width() - cx) / cw;  // 屏幕能显示的候选字数
        int drawn = 0;
        for (int charIdx = candidateScrollOffset; charIdx < charCount && drawn < maxVisible; charIdx++) {
            String ch = getUTF8CharAt(candidates, charIdx);
            if (focusOnCandidates && charIdx == candidateIndex) {
                tft.fillRect(cx, y, cw, 18, TFT_DARKCYAN);
                tft.setTextColor(TFT_WHITE, TFT_DARKCYAN);
            } else {
                tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
            }
            tft.setCursor(cx, y);
            tft.print(ch);
            cx += cw;
            drawn++;
        }
    }
}

    // ---- QWER 键盘 ----
    // 根据当前模式选择键盘布局
    const char* const* layout = (currentMode == MODE_NUMBER) ? keys_num : keys_abc;
    int keyW = 24;
    int keyH = 22;
    int startY = 100;
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 10; c++) {
            char ch = layout[r][c];
            if (ch == ' ' || ch == 0) continue;
            int x = c * keyW;
            int yPos = startY + r * (keyH + 2);

            // 焦点高亮（仅当焦点在键盘且不在候选字上）
            if (!focusOnCandidates && r == keyRow && c == keyCol) {
                tft.fillRect(x, yPos, keyW, keyH, TFT_BLUE);
                tft.setTextColor(TFT_WHITE, TFT_BLUE);
            } else {
                tft.fillRect(x, yPos, keyW, keyH, TFT_DARKCYAN);
                tft.setTextColor(TFT_WHITE, TFT_DARKCYAN);
            }

            // 键帽文字
            String label;
            if (ch == 0x01) label = "模";        // 模式切换
            else if (ch == 0x7f) label = "退";
            else if (ch == 0x03) label = "确";
            else if (ch == ' ') label = "空格";
            else label = String(ch);
            tft.setCursor(x + 2, yPos + 3);
            tft.print(label);
        }
    }
}
// 处理按键输入逻辑
void handleInputKey(char ch) {
    candidateScrollOffset = 0;
    if (ch == 0x01) {        // 模式切换：中文 → 英文 → 数字 → 中文
        currentMode = (InputMode)((currentMode + 1) % 3);
        pinyinBuffer = "";
        candidates = "";
        candidateIndex = 0;
        focusOnCandidates = false;
    }
    else if (ch == 0x7f) {   // 退格
        if (currentMode == MODE_CHINESE && !pinyinBuffer.isEmpty()) {
            pinyinBuffer.remove(pinyinBuffer.length() - 1);
            updateCandidates();
        } else if (!inputTextBuffer.isEmpty()) {
            inputTextBuffer.remove(inputTextBuffer.length() - 1);
        }
    }
    else if (ch == 0x03) {   // 确认提交（退出输入法）
        if (currentMode == MODE_CHINESE && !pinyinBuffer.isEmpty()) {
            if (!candidates.isEmpty()) {
                inputTextBuffer += getUTF8CharAt(candidates, 0); // 第一候选
                pinyinBuffer = "";
                candidates = "";
                candidateIndex = 0;
            }
        }
        inputCompleted = true;
    }
    else if (ch == ' ') {    // 空格键
        if (currentMode == MODE_CHINESE) {
            if (!candidates.isEmpty()) {
                inputTextBuffer += getUTF8CharAt(candidates, candidateIndex);
                pinyinBuffer = "";
                candidates = "";
                candidateIndex = 0;
            } else {
                inputTextBuffer += ' ';
            }
        } else {
            inputTextBuffer += ' ';   // 英文/数字模式直接输入空格
        }
    }
    else {                   // 字母/数字/符号等可打印字符
        if (currentMode == MODE_CHINESE) {
            // 中文模式只接受小写字母
            pinyinBuffer += ch;
            updateCandidates();
        } else {
            // 英文模式或数字模式直接追加
            inputTextBuffer += ch;
        }
    }
    drawInputUI();
}
// 输入法主函数，返回用户最终输入的字符串
String inputTextDialog(const String& prompt, const String& initial) {
    // 重置所有输入状态
    candidateScrollOffset = 0;
    inputTextBuffer = initial;
    pinyinBuffer = "";
    candidates = "";
    candidateIndex = 0;
    currentMode = MODE_CHINESE;      // 默认启动中文模式
    focusOnCandidates = false;
    keyRow = 0;
    keyCol = 0;
    inputCompleted = false;

    drawInputUI();

    while (!inputCompleted) {
        int numCands = getUTF8CharCount(candidates);

        // ============================
        // 焦点在键盘上（普通输入）
        // ============================
        if (!focusOnCandidates) {
            // --- 上键：如果在键盘第一行且中文模式候选非空，则进入候选焦点 ---
            if (isBtnPressed(BTN_UP)) {
                if (keyRow == 0 && currentMode == MODE_CHINESE && numCands > 0) {
                    focusOnCandidates = true;
                    candidateIndex = 0;
                    drawInputUI();
                } else if (keyRow > 0) {
                    keyRow--;
                    drawInputUI();
                }
            }
            // --- 下键 ---
            if (isBtnPressed(BTN_DOWN)) {
                if (keyRow < 3) {
                    keyRow++;
                    drawInputUI();
                }
            }
            // --- 左键 ---
            if (isBtnPressed(BTN_LEFT)) {
                if (keyCol > 0) {
                    keyCol--;
                    drawInputUI();
                }
            }
            // --- 右键 ---
            if (isBtnPressed(BTN_RIGHT)) {
                if (keyCol < 9) {
                    keyCol++;
                    drawInputUI();
                }
            }
            // --- OK 键：输入当前高亮的键盘字符 ---
            if (isBtnPressed(BTN_OK)) {
                const char* const* layout = (currentMode == MODE_NUMBER) ? keys_num : keys_abc;
                char ch = layout[keyRow][keyCol];
                if (ch != ' ' && ch != 0) {
                    handleInputKey(ch);    // 内部会调用 drawInputUI()
                }
            }
        }
        // ============================
        // 焦点在候选字上（仅中文模式）
        // ============================
        // ---- 焦点在候选字时 ----
else {
    int maxVisible = (tft.width() - 40) / 16;   // 与绘制中一致
    if (isBtnPressed(BTN_LEFT)) {
        if (candidateIndex > 0) {
            candidateIndex--;
            // 自动滚动：确保选中项可见
            if (candidateIndex < candidateScrollOffset) {
                candidateScrollOffset = candidateIndex;
            }
            drawInputUI();
        }
    }
    if (isBtnPressed(BTN_RIGHT)) {
        int numCands = getUTF8CharCount(candidates);
        if (candidateIndex < numCands - 1) {
            candidateIndex++;
            // 自动滚动：确保选中项可见
            if (candidateIndex >= candidateScrollOffset + maxVisible) {
                candidateScrollOffset = candidateIndex - maxVisible + 1;
            }
            drawInputUI();
        }
    }
    // 下键和OK键逻辑保持不变

            // --- 下键：返回键盘 ---
            if (isBtnPressed(BTN_DOWN)) {
                focusOnCandidates = false;
                drawInputUI();
            }
            // --- OK 键：选择当前高亮候选字 ---
            if (isBtnPressed(BTN_OK)) {
                if (numCands > 0) {
                    // 取出完整的 UTF-8 汉字
                    inputTextBuffer += getUTF8CharAt(candidates, candidateIndex);
                    pinyinBuffer = "";
                    candidates = "";
                    candidateIndex = 0;
                    focusOnCandidates = false;
                    drawInputUI();
                }
            }
        }

        // 避免快速循环占用 CPU
        delay(20);
    }

    return inputTextBuffer;
}
// ========== 在线音乐搜索与播放模块 ==========
// 将字符串中的非法字符替换为下划线，用于文件名
String sanitizeFileName(String input) {
    const String illegal = "/\\:* ?\"<>|";
    String result = input;
    for (int i = 0; i < result.length(); i++) {
        if (illegal.indexOf(result[i]) >= 0) {
            result.setCharAt(i, '_');
        }
    }
    return result;
}

// 从URL下载文件到SD卡，显示进度条，成功返回true
// ========== HTTPS下载函数（支持重定向 + 错误日志）==========
/**
 * 加速版下载：使用 PSRAM 1MB 缓冲区，减少 SD 写入次数
 * @param url      下载地址
 * @param savePath 保存路径（将被自动修正为真实扩展名）
 * @return 成功返回 true，失败返回 false
 */
bool downloadFileWithProgress(const String& url, String& savePath) {
    Serial.printf("[DOWNLOAD] 空闲堆: %d, PSRAM 空闲: %d\n",
                  ESP.getFreeHeap(), ESP.getFreePsram());

    // ===== 代理已移除，改为直连 =====

    Serial.printf("[DOWNLOAD] ====== 开始下载任务 ======\n");
    Serial.printf("[DOWNLOAD] URL: %s\n", url.c_str());
    Serial.printf("[DOWNLOAD] 保存路径: %s\n", savePath.c_str());

    // ----- 尝试多组 UA / Referer 组合 -----
    struct HeaderSet {
        const char* userAgent;
        const char* referer;
    };
    std::vector<HeaderSet> headerSets = {
        {"Mozilla/5.0 (Linux; Android 16; V2425A) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/131.0.6778.200 Mobile Safari/537.36 VivoBrowser/28.7.45.0",
         "https://y.qq.com/"},
        {"Mozilla/5.0 (Linux; Android 16; V2425A) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/131.0.6778.200 Mobile Safari/537.36 VivoBrowser/28.7.45.0",
         "https://cyapi.top/"},
        {"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/131.0.0.0 Safari/537.36",
         "https://y.qq.com/"},
        {"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/131.0.0.0 Safari/537.36",
         ""},
        {"ESP32-CyberOS/5.0", ""}
    };

    String urlVariants[2] = { url, "" };
    if (url.startsWith("https://")) {
        urlVariants[1] = "http://" + url.substring(8);
    }

    // 尝试不同 URL 和 UA
    for (const String& testUrl : urlVariants) {
        if (testUrl.isEmpty()) continue;
        for (const HeaderSet& hs : headerSets) {
            std::unique_ptr<WiFiClient> client;
            if (testUrl.startsWith("https")) {
                auto secure = std::make_unique<WiFiClientSecure>();
                secure->setInsecure();
                client = std::move(secure);
            } else {
                client = std::make_unique<WiFiClient>();
            }

            HTTPClient http;
            http.setTimeout(20000);
            http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
            http.setRedirectLimit(5);

            if (!http.begin(*client, testUrl)) {
                Serial.printf("[DOWNLOAD] begin() 失败: %s\n", testUrl.c_str());
                http.end();
                client->stop();
                continue;
            }

            http.addHeader("User-Agent", hs.userAgent);
            if (strlen(hs.referer) > 0) http.addHeader("Referer", hs.referer);
            http.addHeader("Accept-Encoding", "identity");
            http.addHeader("Connection", "keep-alive");

            Serial.printf("[DOWNLOAD] 尝试 URL=%s, UA=%s\n", testUrl.c_str(), hs.userAgent);

            int httpCode = http.GET();
            Serial.printf("[DOWNLOAD] HTTP 响应码: %d\n", httpCode);

            // 处理重定向
            if (httpCode == 301 || httpCode == 302 || httpCode == 307) {
                String redirectUrl = http.getLocation();
                http.end();
                client->stop();
                if (!redirectUrl.isEmpty()) {
                    Serial.printf("[DOWNLOAD] 跟随重定向: %s\n", redirectUrl.c_str());
                    return downloadFileWithProgress(redirectUrl, savePath);
                }
                continue;
            }

            if (httpCode == 200) {
                // ----- 获取文件大小 -----
                unsigned long fileSize = 0;
                bool sizeKnown = false;
                if (http.hasHeader("Content-Length")) {
                    fileSize = http.header("Content-Length").toInt();
                    sizeKnown = true;
                    Serial.printf("[DOWNLOAD] 文件大小: %lu 字节 (%.1f KB)\n",
                                  fileSize, fileSize / 1024.0);
                } else {
                    Serial.printf("[DOWNLOAD] 未告知文件大小\n");
                }

                // 打开 SD 卡写入
                File file = SD.open(savePath, FILE_WRITE);
                if (!file) {
                    http.end();
                    drawErrorScreen("SD卡错误", "无法创建文件:\n" + savePath);
                    return false;
                }

                // ----- 绘制下载进度界面 -----
                tft.fillScreen(TFT_BLACK);
                loadFontSafe();
                tft.setTextColor(TFT_WHITE, TFT_BLACK);
                tft.drawString("下载中: " + savePath.substring(savePath.lastIndexOf('/') + 1), 5, 5);
                int barX = 5, barY = 45, barW = tft.width() - 10, barH = 14;
                tft.drawRect(barX, barY, barW, barH, TFT_DARKGREY);

                // ----- 大缓冲区加速 (PSRAM) -----
                const size_t BUF_SIZE = 1024 * 1024; // 1 MB
                uint8_t* buffer = (uint8_t*)heap_caps_malloc(BUF_SIZE, MALLOC_CAP_SPIRAM);
                if (!buffer) {
                    // 如果 PSRAM 分配失败，回退到小缓冲（128 KB）
                    buffer = (uint8_t*)heap_caps_malloc(128 * 1024, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
                    if (!buffer) {
                        Serial.println("[DOWNLOAD] PSRAM 分配失败，下载终止");
                        file.close();
                        SD.remove(savePath);
                        http.end();
                        return false;
                    }
                }
                size_t actualBufSize = heap_caps_get_free_size(MALLOC_CAP_SPIRAM) > BUF_SIZE ? BUF_SIZE : 128 * 1024;

                WiFiClient *stream = http.getStreamPtr();
                unsigned long downloaded = 0;
                unsigned long lastUpdate = 0;

                while (http.connected() && (!sizeKnown || downloaded < fileSize)) {
                    if (checkStopAction()) {
                        Serial.println("[DOWNLOAD] 用户取消下载");
                        file.close();
                        SD.remove(savePath);
                        http.end();
                        heap_caps_free(buffer);
                        return false;
                    }

                    size_t available = stream->available();
                    if (available > 0) {
                        size_t toRead = min(available, actualBufSize);
                        int bytesRead = stream->readBytes(buffer, toRead);
                        if (bytesRead > 0) {
                            file.write(buffer, bytesRead);
                            downloaded += bytesRead;

                            // 每 200ms 或最后一块时刷新进度
                            if (millis() - lastUpdate > 200 || (sizeKnown && downloaded >= fileSize)) {
                                lastUpdate = millis();
                                if (sizeKnown && fileSize > 0) {
                                    int percent = (downloaded * 100) / fileSize;
                                    int fillW = (barW * downloaded) / fileSize;
                                    tft.fillRect(barX + 1, barY + 1, fillW, barH - 2, TFT_CYAN);
                                    tft.fillRect(barX, barY + 18, tft.width(), 18, TFT_BLACK);
                                    tft.setTextColor(TFT_WHITE, TFT_BLACK);
                                    char buf[40];
                                    sprintf(buf, "%d%%  %lu / %lu KB", percent,
                                            downloaded / 1024, fileSize / 1024);
                                    tft.drawString(buf, barX, barY + 18);
                                } else {
                                    int fillW = (downloaded / 1024 % 20) * (barW / 20);
                                    tft.fillRect(barX + 1, barY + 1, fillW, barH - 2, TFT_CYAN);
                                    tft.fillRect(barX, barY + 18, tft.width(), 18, TFT_BLACK);
                                    tft.setTextColor(TFT_WHITE, TFT_BLACK);
                                    char buf[30];
                                    sprintf(buf, "已下载: %lu KB", downloaded / 1024);
                                    tft.drawString(buf, barX, barY + 18);
                                }
                            }
                        }
                    }
                    delay(1);
                }

                file.close();
                heap_caps_free(buffer);

                // ----- 根据文件头修正真实扩展名 -----
                String realExt = "mp3";
                File checkFile = SD.open(savePath, FILE_READ);
                if (checkFile) {
                    uint8_t header[12];
                    int len = checkFile.read(header, sizeof(header));
                    checkFile.close();
                    if (len >= 3 && header[0] == 'I' && header[1] == 'D' && header[2] == '3')
                        realExt = "mp3";
                    else if (len >= 2 && header[0] == 0xFF && (header[1] & 0xE0) == 0xE0)
                        realExt = "mp3";
                    else if (len >= 12 && memcmp(&header[4], "ftyp", 4) == 0) {
                        if (memcmp(&header[8], "M4A ", 4) == 0 || memcmp(&header[8], "mp42", 4) == 0)
                            realExt = "m4a";
                        else if (memcmp(&header[8], "aac", 3) == 0)
                            realExt = "aac";
                        else realExt = "m4a";
                    } else if (len >= 4 && header[0]=='f' && header[1]=='L' && header[2]=='a' && header[3]=='C')
                        realExt = "flac";
                    else if (len >= 4 && header[0]=='R' && header[1]=='I' && header[2]=='F' && header[3]=='F')
                        realExt = "wav";
                }

                String currentExt = savePath.substring(savePath.lastIndexOf('.') + 1);
                currentExt.toLowerCase();
                if (currentExt != realExt) {
                    String newPath = savePath.substring(0, savePath.lastIndexOf('.')) + "." + realExt;
                    SD.rename(savePath, newPath);
                    savePath = newPath;
                    Serial.printf("[DOWNLOAD] 重命名为 %s\n", savePath.c_str());
                }

                http.end();
                Serial.printf("[DOWNLOAD] 完成，总计 %lu 字节\n", downloaded);

                if (sizeKnown && downloaded < fileSize) {
                    SD.remove(savePath);
                    drawErrorScreen("下载不完整", "实际下载 " + String(downloaded / 1024) + " KB");
                    return false;
                }
                return true;
            }

            Serial.printf("[DOWNLOAD] 失败: %d\n", httpCode);
            http.end();
            delay(200);
        }
    }

    drawErrorScreen("下载失败", "多次重试后无法下载\n" + url);
    return false;
}
// ========== 网络连通性诊断 ==========
bool pingTest(const char* host) {
    tft.fillScreen(TFT_BLACK);
    loadFontSafe();
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.drawString("网络诊断中...", 10, 10);
    tft.drawString("Ping " + String(host) + " ...", 10, 30);

    // 使用 WiFiClient 连接 HTTP 端口测试连通性
    WiFiClient testClient;
    unsigned long start = millis();
    bool success = testClient.connect(host, 80, 3000);
    unsigned long elapsed = millis() - start;

    if (success) {
        testClient.stop();
        tft.setTextColor(TFT_GREEN, TFT_BLACK);
        tft.drawString("✓ 网络连通正常 (" + String(elapsed) + "ms)", 10, 55);
        Serial.printf("[NET_DIAG] Ping %s 成功, 延迟=%lums\n", host, elapsed);
        delay(500);
        return true;
    } else {
        tft.setTextColor(TFT_RED, TFT_BLACK);
        tft.drawString("✗ 网络不通，请检查WiFi", 10, 55);
        tft.drawString("目标: " + String(host), 10, 75);
        Serial.printf("[NET_DIAG] ERROR: Ping %s 失败\n", host);
        delay(2000);
        return false;
    }
}
// ========== 错误屏幕显示 ==========
void drawErrorScreen(const String& title, const String& msg) {
    tft.fillScreen(TFT_BLACK);
    loadFontSafe();

    // 红色标题
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.drawString("[" + title + "]", 5, 5);

    // 白色错误详情
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    int y = 35;
    // 简单换行处理(每24px一行)
    String remaining = msg;
    while (remaining.length() > 0 && y < tft.height() - 30) {
        int maxLen = 28;  // 240屏宽下t18大约显示28个半角字符
        String line = remaining.substring(0, min(maxLen, (int)remaining.length()));
        tft.drawString(line, 5, y);
        y += 24;
        if (remaining.length() > maxLen) {
            remaining = remaining.substring(maxLen);
        } else {
            break;
        }
    }

    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.drawString("按OK返回", 5, tft.height() - 25);

    // 同时打印到串口
    Serial.printf("[ERROR] %s: %s\n", title.c_str(), msg.c_str());

    // 等待用户确认
    while (!checkStopAction()) { delay(10); }
    delay(200); // 消抖
}

// ========== 双源在线音乐搜索 ==========
void searchMusicOnline() {
    if (WiFi.status() != WL_CONNECTED) {
        drawErrorScreen("WiFi", "未连接网络");
        return;
    }

    String keyword = inputTextDialog("输入歌名:", "");
    if (keyword.isEmpty()) return;

    tft.fillScreen(TFT_BLACK); loadFontSafe();
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.drawString("双源搜索中...", 10, 30);

    std::vector<MusicInfo> cyapiResults;
    std::vector<MusicInfo> neteaseResults;

    // ---- 源1: CYAPI (QQ音乐) ----
    {
        String cyapiUrl = buildCyApiSearchUrl(keyword);
        Serial.printf("[搜索] CYAPI: %s\n", cyapiUrl.c_str());

        HTTPClient http;
        http.setTimeout(15000);
        std::unique_ptr<WiFiClient> client;
        if (cyapiUrl.startsWith("https")) {
            auto secure = std::make_unique<WiFiClientSecure>();
            secure->setInsecure();
            client = std::move(secure);
        } else {
            client = std::make_unique<WiFiClient>();
        }
        http.begin(*client, cyapiUrl);
        int code = http.GET();
        if (code == 200) {
            String payload = http.getString();
            parseCyApiJson(payload.c_str(), cyapiResults);
            Serial.printf("[搜索] CYAPI 成功: %d首\n", (int)cyapiResults.size());
        } else {
            Serial.printf("[搜索] CYAPI 失败: %d\n", code);
        }
        http.end();
    }

    // ---- 源2: 网易云 (alapi.cn) ----
    searchMusicNetease(keyword, neteaseResults);
    Serial.printf("[搜索] 网易云: %d首\n", (int)neteaseResults.size());

    // ---- 合并: CYAPI(QQ)置顶 + 网易云在后 ----
    std::vector<MusicInfo> merged;
    for (auto& m : cyapiResults) {
        m.name = m.name + "-qq";
        merged.push_back(m);
    }
    for (auto& m : neteaseResults) {
        merged.push_back(m);
    }

    if (merged.empty()) {
        drawErrorScreen("搜索失败", "两个源均无结果");
        return;
    }

    Serial.printf("[搜索] 合并结果: %d首 (QQ:%d + 网易云:%d)\n",
                  (int)merged.size(), (int)cyapiResults.size(), (int)neteaseResults.size());
    displayMusicSearchResults(merged);
}
// 解析JSON并显示歌曲列表
void displayMusicSearchResults(std::vector<MusicInfo>& musicList) {
    if (musicList.empty()) {
        drawErrorScreen("无结果", "未找到相关歌曲");
        return;
    }

    int listIdx = 0, listTop = 0;
    int maxItems = (tft.height() - 40) / 22;
    bool exitList = false;

    auto drawMusicList = [&]() {
        tft.fillScreen(TFT_BLACK);
        loadFontSafe();
        tft.setTextColor(TFT_GREEN, TFT_BLACK);
        tft.drawString("搜索结果:", 5, 5);
        String countStr = String(musicList.size()) + "首";
        tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
        tft.drawString(countStr, tft.width() - tft.textWidth(countStr) - 5, 5);
        for (int i = 0; i < maxItems; i++) {
            int idx = listTop + i;
            if (idx >= musicList.size()) break;
            int yPos = 30 + i * 22;
            if (idx == listIdx) {
                tft.fillRect(0, yPos, tft.width(), 22, TFT_DARKCYAN);
                tft.setTextColor(TFT_WHITE, TFT_DARKCYAN);
            } else {
                tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
            }
            tft.drawString(musicList[idx].name + " - " + musicList[idx].singer, 10, yPos);
        }
    };

    drawMusicList();

    while (!exitList) {
        // --- OK 键：播放 ---
        if (digitalRead(BTN_OK) == LOW) {
            delay(50);  // 消抖
            while (digitalRead(BTN_OK) == LOW) { yield(); }
            delay(50);

            if (WiFi.status() != WL_CONNECTED) {
                drawErrorScreen("断开", "WiFi已断开");
                drawMusicList();
                continue;
            }

            MusicInfo& song = musicList[listIdx];
            String playUrl;
            int urlHttpCode = 0;

            // 获取播放直链
            tft.fillScreen(TFT_BLACK);
            loadFontSafe();
            tft.setTextColor(TFT_CYAN, TFT_BLACK);
            tft.drawString("获取播放地址...", 10, 10);

            if (song.isNetease) {
                // 网易云：通过aa1.cn获取直链，自带状态码
                playUrl = getMusicDirectUrl(song.id, urlHttpCode);
            } else {
                // QQ：已有直链，HEAD检查状态
                playUrl = song.url;
                // 强制https转http
                if (playUrl.startsWith("https://"))
                    playUrl = "http://" + playUrl.substring(8);
                urlHttpCode = checkUrlHttpStatus(playUrl);
            }

            // 统一检查HTTP错误码
            if (urlHttpCode == 404) {
                Serial.println("[PLAY] 404: 歌曲无版权或需要VIP");
                drawErrorScreen("版权限制", song.isNetease ?
                    "垃圾网易云没版权/VIP" : "QQ音乐没版权/VIP");
                drawMusicList();
                continue;
            }
            if (urlHttpCode == 403) {
                Serial.println("[PLAY] 403: 防盗链拦截");
                drawErrorScreen("防盗链", "运气不错,请求被大厂防盗链干死了");
                drawMusicList();
                continue;
            }

            // 网易云的URL再强制转一次http
            if (playUrl.startsWith("https://")) {
                playUrl = "http://" + playUrl.substring(8);
                Serial.printf("[PLAY] https转http: %s\n", playUrl.c_str());
            }
            Serial.printf("[PLAY] 最终播放URL: %s\n", playUrl.c_str());

            if (playUrl.isEmpty()) {
                drawErrorScreen("失败", "无法获取播放链接");
                drawMusicList();
                continue;
            }

            // 检测M4A格式 — 下载到本地后转换播放
            bool isM4A = playUrl.indexOf(".m4a") >= 0 || playUrl.indexOf("m4a?") >= 0;
            Serial.printf("[PLAY] isM4A=%d, URL: %s\n", isM4A, playUrl.c_str());
            String playAacPath;
            bool m4aReady = false;

            if (isM4A) {
                // 下载M4A到本地，然后用convertM4AToAAC转换播放
                Serial.printf("[PLAY] M4A格式，下载到本地...\n");
                tft.fillScreen(TFT_BLACK);
                loadFontSafe();
                tft.setTextColor(TFT_CYAN, TFT_BLACK);
                tft.drawString("M4A下载中...", 10, 30);
                tft.setTextColor(TFT_WHITE, TFT_BLACK);
                tft.drawString(song.name, 10, 50);

                if (!SD.exists("/music_cache")) SD.mkdir("/music_cache");
                String dlPath = "/music_cache/_download.m4a";
                SD.remove(dlPath);

                HTTPClient m4aDl;
                m4aDl.setTimeout(30000);
                m4aDl.begin(playUrl);
                int m4aCode = m4aDl.GET();
                if (m4aCode != 200) {
                    drawErrorScreen("下载失败", "HTTP " + String(m4aCode));
                    m4aDl.end();
                    drawMusicList();
                    continue;
                }

                WiFiClient* stream = m4aDl.getStreamPtr();
                uint32_t totalSz = m4aDl.getSize();
                File dlFile = SD.open(dlPath, FILE_WRITE);
                if (!dlFile) {
                    drawErrorScreen("SD卡写入失败", "");
                    m4aDl.end();
                    drawMusicList();
                    continue;
                }

                uint8_t* dlBuf = (uint8_t*)heap_caps_malloc(4096, MALLOC_CAP_SPIRAM);
                uint32_t downloaded = 0;
                unsigned long dlStart = millis();
                while (m4aDl.connected() && downloaded < totalSz) {
                    size_t avail = stream->available();
                    if (avail) {
                        int toRead = min((size_t)4096, avail);
                        int got = stream->read(dlBuf, toRead);
                        if (got > 0) {
                            dlFile.write(dlBuf, got);
                            downloaded += got;
                        }
                    } else { delay(1); }
                    if (millis() - dlStart > 500) {
                        tft.fillRect(10, 70, 220, 16, TFT_BLACK);
                        tft.setTextColor(TFT_YELLOW, TFT_BLACK);
                        tft.drawString(String(downloaded/1024) + "KB / " + String(totalSz/1024) + "KB", 10, 70);
                        dlStart = millis();
                    }
                    yield();
                }
                dlFile.flush();
                dlFile.close();
                heap_caps_free(dlBuf);
                m4aDl.end();
                Serial.printf("[PLAY] M4A下载完成: %uKB\n", downloaded/1024);

                // 转换M4A→AAC
                tft.setTextColor(TFT_CYAN, TFT_BLACK);
                tft.drawString("M4A转换中...", 10, 90);
                String aacPath;
                if (convertM4AToAAC(dlPath, aacPath)) {
                    playAacPath = aacPath;
                    m4aReady = true;
                    SD.remove(dlPath); // 删除原始M4A
                    Serial.printf("[PLAY] M4A转换成功: %s\n", aacPath.c_str());
                } else {
                    drawErrorScreen("M4A转换失败", "");
                    SD.remove(dlPath);
                    drawMusicList();
                    continue;
                }
            }

            // 播放UI
            tft.fillScreen(TFT_BLACK);
            loadFontSafe();
            tft.setTextColor(TFT_GREEN, TFT_BLACK);
            tft.drawString("正在播放:", 10, 5);
            tft.setTextColor(TFT_WHITE, TFT_BLACK);
            tft.drawString(song.name, 10, 23);
            tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
            tft.drawString((song.isNetease ? "NetEase" : "QQ") + String(isM4A ? " [M4A/SD]" : " [Stream]"), 10, 41);
            uint8_t vol = 10;
            tft.setTextColor(TFT_CYAN, TFT_BLACK);
            tft.drawString("Vol:" + String(vol), 10, 59);
            tft.setTextColor(TFT_YELLOW, TFT_BLACK);
            tft.drawString("OK:暂停 右:下载 左:退出", 5, tft.height() - 18);

            // PSRAM分配Audio对象，避免栈溢出
            Audio* audio = (Audio*)heap_caps_malloc(sizeof(Audio), MALLOC_CAP_SPIRAM);
            if (!audio) {
                drawErrorScreen("内存不足", "无法分配PSRAM");
                drawMusicList();
                continue;
            }
            new (audio) Audio();  // placement new
            audio->setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
            audio->setVolume(vol);

            // PSRAM分配下载缓冲区
            uint8_t* dlBuf = (uint8_t*)heap_caps_malloc(2048, MALLOC_CAP_SPIRAM);

            pinMode(MAX98357A_SD, OUTPUT);
            digitalWrite(MAX98357A_SD, HIGH);

            // M4A播放转换后的AAC，其他格式流式播放
            bool connOk;
            if (isM4A && m4aReady) {
                Serial.printf("[PLAY] 播放AAC: %s\n", playAacPath.c_str());
                connOk = audio->connecttoFS(SD, playAacPath.c_str());
            } else {
                Serial.printf("[PLAY] 连接音频流: %s\n", playUrl.c_str());
                connOk = audio->connecttohost(playUrl.c_str());
            }
            if (!connOk) {
                Serial.printf("[PLAY] 连接失败, isM4A=%d\n", isM4A);
                drawErrorScreen("播放失败", isM4A ? "AAC解码失败" : "无法连接音频流");
                digitalWrite(MAX98357A_SD, LOW);
                audio->~Audio();
                heap_caps_free(audio);
                if (dlBuf) heap_caps_free(dlBuf);
                drawMusicList();
                continue;
            }

            // 等待开始播放
            unsigned long initTimeout = millis() + 10000;
            while (!audio->isRunning() && millis() < initTimeout) {
                audio->loop();
                yield();
            }
            if (!audio->isRunning()) {
                Serial.println("[PLAY] 音频流初始化超时");
                drawErrorScreen("播放失败", "音频流初始化超时");
                digitalWrite(MAX98357A_SD, LOW);
                audio->~Audio();
                heap_caps_free(audio);
                if (dlBuf) heap_caps_free(dlBuf);
                drawMusicList();
                continue;
            }

            // 打印音频信息
            Serial.printf("[PLAY] 编码: %s\n", audio->getCodecname());
            Serial.printf("[PLAY] 比特率: %d kbps\n", audio->getBitRate() / 1000);
            Serial.printf("[PLAY] 采样率: %d Hz\n", audio->getSampleRate());
            Serial.printf("[PLAY] 位深: %d bit\n", audio->getBitsPerSample());
            Serial.printf("[PLAY] 声道: %d\n", audio->getChannels());

            // 播放状态
            unsigned long playStartMs = millis();
            unsigned long lastDisplayUpdate = 0;
            uint32_t lastCurTime = 0;
            uint32_t lastDurTime = 0;
            bool paused = false;

            // 下载状态
            bool downloading = false;
            HTTPClient* dlHttp = nullptr;
            WiFiClient* dlClient = nullptr;
            File dlFile;
            uint32_t dlBytes = 0;
            uint32_t dlTotal = 0;

            // 进度条参数
            int barX = 10, barY = 95, barW = tft.width() - 20, barH = 8;

            // 播放循环
            bool songFinished = false;
            while (audio->isRunning() || paused || downloading) {
                if (!paused && audio->isRunning()) {
                    audio->loop();
                }
                // 歌曲播完，如果没有下载任务就退出
                if (!audio->isRunning() && !paused && !downloading) {
                    if (!songFinished) {
                        songFinished = true;
                        tft.fillRect(10, 10, tft.width() - 20, 16, TFT_BLACK);
                        tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
                        tft.drawString("播放结束", 10, 10);
                        Serial.printf("[PLAY] 播放结束，时长: %lus\n", (millis() - playStartMs) / 1000);
                    }
                    delay(100);
                    // 等用户按任意键退出
                    if (digitalRead(BTN_OK) == LOW || digitalRead(BTN_LEFT) == LOW ||
                        digitalRead(BTN_UP) == LOW || digitalRead(BTN_DOWN) == LOW ||
                        digitalRead(BTN_RIGHT) == LOW) {
                        break;
                    }
                    continue;
                }

                // ---- OK键：暂停/恢复 ----
                if (digitalRead(BTN_OK) == LOW) {
                    delayedDebounce(BTN_OK);
                    audio->pauseResume();
                    paused = !paused;
                    Serial.printf("[PLAY] %s\n", paused ? "暂停" : "恢复");
                    tft.fillRect(10, 10, tft.width() - 20, 16, TFT_BLACK);
                    tft.setTextColor(paused ? TFT_YELLOW : TFT_GREEN, TFT_BLACK);
                    tft.drawString(paused ? "已暂停" : "正在播放:", 10, 10);
                }
                // ---- 左键：退出 ----
                if (digitalRead(BTN_LEFT) == LOW) {
                    delayedDebounce(BTN_LEFT);
                    if (downloading && dlClient) {
                        dlClient->stop();
                        delete dlClient;
                        dlClient = nullptr;
                        dlHttp->end();
                        delete dlHttp;
                        dlHttp = nullptr;
                        dlFile.close();
                        downloading = false;
                        Serial.printf("[DL] 取消下载，已保存: %u bytes\n", dlBytes);
                    }
                    audio->stopSong();
                    Serial.printf("[PLAY] 退出播放，时长: %lus\n", (millis() - playStartMs) / 1000);
                    break;
                }
                // ---- 右键：下载到SD ----
                if (digitalRead(BTN_RIGHT) == LOW && !downloading) {
                    delayedDebounce(BTN_RIGHT);
                    if (!SD.exists("/music_cache")) SD.mkdir("/music_cache");
                    String safeName = sanitizeFileName(song.name + "_" + song.singer);
                    String dlPath = "/music_cache/" + safeName + ".mp3";

                    Serial.printf("[DL] 开始下载: %s -> %s\n", playUrl.c_str(), dlPath.c_str());
                    dlFile = SD.open(dlPath.c_str(), FILE_WRITE);
                    if (dlFile) {
                        dlHttp = new HTTPClient();
                        dlHttp->setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
                        dlHttp->setRedirectLimit(5);
                        dlHttp->begin(playUrl);
                        dlHttp->setTimeout(5000);
                        int dlCode = dlHttp->GET();
                        if (dlCode == 200) {
                            dlTotal = dlHttp->getSize();
                            dlClient = new WiFiClient(dlHttp->getStream());
                            dlBytes = 0;
                            downloading = true;
                            Serial.printf("[DL] HTTP 200, 大小: %u bytes\n", dlTotal);
                            tft.fillRect(barX, barY + barH + 18, barW, barH, TFT_DARKGREY);
                            tft.setTextColor(TFT_ORANGE, TFT_BLACK);
                            tft.drawString("下载中...", 10, barY + barH + 30);
                        } else {
                            Serial.printf("[DL] HTTP 失败: %d\n", dlCode);
                            dlFile.close();
                            dlHttp->end();
                            delete dlHttp;
                            dlHttp = nullptr;
                            tft.setTextColor(TFT_RED, TFT_BLACK);
                            tft.drawString("下载失败:" + String(dlCode), 10, barY + barH + 30);
                        }
                    } else {
                        Serial.println("[DL] 无法创建文件");
                    }
                }
                // ---- 上键：音量+ ----
                if (digitalRead(BTN_UP) == LOW) {
                    delayedDebounce(BTN_UP);
                    if (vol < 21) { vol++; audio->setVolume(vol); }
                    Serial.printf("[PLAY] 音量: %d\n", vol);
                    tft.fillRect(10, 59, 80, 16, TFT_BLACK);
                    tft.setTextColor(TFT_CYAN, TFT_BLACK);
                    tft.drawString("Vol:" + String(vol), 10, 59);
                }
                // ---- 下键：音量- ----
                if (digitalRead(BTN_DOWN) == LOW) {
                    delayedDebounce(BTN_DOWN);
                    if (vol > 0) { vol--; audio->setVolume(vol); }
                    Serial.printf("[PLAY] 音量: %d\n", vol);
                    tft.fillRect(10, 59, 80, 16, TFT_BLACK);
                    tft.setTextColor(TFT_CYAN, TFT_BLACK);
                    tft.drawString("Vol:" + String(vol), 10, 59);
                }

                // ---- 下载数据写入 ----
                if (downloading && dlClient && dlClient->connected() && dlClient->available()) {
                    int bytesRead = dlClient->read(dlBuf, 2048);
                    if (bytesRead > 0) {
                        dlFile.write(dlBuf, bytesRead);
                        dlBytes += bytesRead;
                    }
                    if (dlTotal > 0 && dlBytes >= dlTotal) {
                        dlClient->stop();
                        delete dlClient;
                        dlClient = nullptr;
                        dlHttp->end();
                        delete dlHttp;
                        dlHttp = nullptr;
                        dlFile.close();
                        downloading = false;
                        Serial.printf("[DL] 下载完成: %u bytes\n", dlBytes);
                        tft.fillRect(10, barY + barH + 30, barW, 16, TFT_BLACK);
                        tft.setTextColor(TFT_GREEN, TFT_BLACK);
                        tft.drawString("下载完成!", 10, barY + barH + 30);
                    }
                }
                if (downloading && dlClient && !dlClient->connected() && dlBytes > 0) {
                    dlFile.close();
                    dlHttp->end();
                    delete dlHttp;
                    dlHttp = nullptr;
                    delete dlClient;
                    dlClient = nullptr;
                    downloading = false;
                    Serial.printf("[DL] 连接断开，已保存: %u bytes\n", dlBytes);
                    tft.fillRect(10, barY + barH + 30, barW, 16, TFT_BLACK);
                    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
                    tft.drawString("下载中断:" + String(dlBytes/1024) + "KB", 10, barY + barH + 30);
                }

                // ---- 每500ms更新显示 ----
                unsigned long now = millis();
                if (now - lastDisplayUpdate >= 500) {
                    lastDisplayUpdate = now;

                    // 更新音频时间（播放中才更新，结束后保持最后值）
                    if (audio->isRunning()) {
                        lastCurTime = audio->getAudioCurrentTime();
                        uint32_t d = audio->getAudioFileDuration();
                        if (d > 0) lastDurTime = d;
                    }
                    int curM = lastCurTime / 60, curS = lastCurTime % 60;
                    int durM = lastDurTime / 60, durS = lastDurTime % 60;
                    char timeBuf[24];
                    if (lastDurTime > 0) {
                        sprintf(timeBuf, "%02d:%02d / %02d:%02d", curM, curS, durM, durS);
                    } else {
                        sprintf(timeBuf, "%02d:%02d", curM, curS);
                    }
                    tft.fillRect(10, barY + barH + 2, barW, 14, TFT_BLACK);
                    tft.setTextColor(TFT_WHITE, TFT_BLACK);
                    tft.drawString(String(timeBuf), 10, barY + barH + 2);

                    // 播放进度条
                    if (!songFinished) {
                        tft.fillRect(barX, barY, barW, barH, TFT_DARKGREY);
                        if (lastDurTime > 0 && lastCurTime <= lastDurTime) {
                            int fillW = (int)((float)lastCurTime / lastDurTime * barW);
                            if (fillW > barW) fillW = barW;
                            tft.fillRect(barX, barY, fillW, barH, TFT_GREEN);
                        }
                    }

                    // 下载进度条
                    if (downloading && dlTotal > 0) {
                        int dlFillW = (int)((float)dlBytes / dlTotal * barW);
                        if (dlFillW > barW) dlFillW = barW;
                        tft.fillRect(barX, barY + 18, dlFillW, barH, TFT_ORANGE);
                        int dlPct = (int)((float)dlBytes / dlTotal * 100);
                        tft.fillRect(10, barY + barH + 30, 80, 14, TFT_BLACK);
                        tft.setTextColor(TFT_ORANGE, TFT_BLACK);
                        char pctBuf[20];
                        sprintf(pctBuf, "%d%% %uKB", dlPct, dlBytes / 1024);
                        tft.drawString(String(pctBuf), 10, barY + barH + 30);
                    }

                    Serial.printf("[PLAY] %s | 缓冲:%d | RSSI:%d\n",
                                  timeBuf, audio->inBufferFilled(), WiFi.RSSI());
                }

                yield();
            }

            // 清理
            if (downloading) {
                if (dlClient) { dlClient->stop(); delete dlClient; dlClient = nullptr; }
                if (dlHttp) { dlHttp->end(); delete dlHttp; dlHttp = nullptr; }
                dlFile.close();
            }
            audio->~Audio();
            heap_caps_free(audio);
            if (dlBuf) heap_caps_free(dlBuf);
            // 清理M4A临时文件
            if (isM4A) {
                if (!playAacPath.isEmpty() && SD.exists(playAacPath)) SD.remove(playAacPath);
                Serial.printf("[PLAY] 已删除M4A临时文件\n");
            }
            digitalWrite(MAX98357A_SD, LOW);
            drawMusicList();
        }

        // 方向键
        if (isBtnPressed(BTN_UP)) {
            if (listIdx > 0) listIdx--;
            if (listIdx < listTop) listTop = listIdx;
            drawMusicList();
        }
        if (isBtnPressed(BTN_DOWN)) {
            if (listIdx < musicList.size() - 1) listIdx++;
            if (listIdx >= listTop + maxItems) listTop = listIdx - maxItems + 1;
            drawMusicList();
        }
        if (isBtnPressed(BTN_LEFT)) {
            exitList = true;
        }
        delay(10);
    }
}

// ========== 随机音乐 ==========
void playRandomMusic() {
    if (WiFi.status() != WL_CONNECTED) {
        drawErrorScreen("需要WiFi", "请先连接WiFi");
        return;
    }

    uint8_t vol = 10;
    pinMode(MAX98357A_SD, OUTPUT);
    digitalWrite(MAX98357A_SD, HIGH);
    bool wantExit = false;

    while (!wantExit) {
        // 获取随机URL
        tft.fillScreen(TFT_BLACK);
        loadFontSafe();
        tft.setTextColor(TFT_CYAN, TFT_BLACK);
        tft.drawString("随机音乐...", 10, 30);

        // 获取随机歌曲JSON
        HTTPClient http;
        http.begin("http://free.wqwlkj.cn/wqwlapi/wyy_random.php?type=json");
        http.setTimeout(10000);
        int code = http.GET();
        String playUrl, songName, artistName;
        if (code == 200) {
            String payload = http.getString();
            Serial.printf("[RANDOM] JSON: %.200s...\n", payload.c_str());
            // 用ArduinoJson解析
            DynamicJsonDocument doc(2048);
            if (!deserializeJson(doc, payload)) {
                playUrl = doc["data"]["url"].as<String>();
                songName = doc["data"]["name"].as<String>();
                artistName = doc["data"]["artistsname"].as<String>();
                Serial.printf("[RANDOM] 歌名:%s 歌手:%s\n", songName.c_str(), artistName.c_str());
            } else {
                Serial.println("[RANDOM] JSON解析失败");
            }
        }
        http.end();

        if (playUrl.startsWith("https://"))
            playUrl = "http://" + playUrl.substring(8);

        Serial.printf("[RANDOM] URL: %s\n", playUrl.c_str());

        if (playUrl.isEmpty()) {
            drawErrorScreen("获取失败", "空URL");
            break;
        }

        // 播放UI
        tft.fillScreen(TFT_BLACK);
        tft.setTextColor(TFT_GREEN, TFT_BLACK);
        tft.drawString(songName.isEmpty() ? "随机音乐" : songName, 10, 5);
        tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
        tft.drawString(artistName.isEmpty() ? "Netease" : artistName, 10, 23);
        tft.setTextColor(TFT_CYAN, TFT_BLACK);
        tft.drawString("Vol:" + String(vol), 10, 41);
        tft.setTextColor(TFT_YELLOW, TFT_BLACK);
        tft.drawString("OK:暂停 右:换曲 左:退出", 5, tft.height() - 18);

        Audio* audio = (Audio*)heap_caps_malloc(sizeof(Audio), MALLOC_CAP_SPIRAM);
        if (!audio) { drawErrorScreen("内存不足", ""); break; }
        new (audio) Audio();
        audio->setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
        audio->setVolume(vol);

        bool connOk = audio->connecttohost(playUrl.c_str());
        if (!connOk) {
            drawErrorScreen("播放失败", "");
            audio->~Audio();
            heap_caps_free(audio);
            break;
        }

        unsigned long initTimeout = millis() + 10000;
        while (!audio->isRunning() && millis() < initTimeout) {
            audio->loop();
            yield();
        }
        if (!audio->isRunning()) {
            drawErrorScreen("初始化超时", "");
            audio->~Audio();
            heap_caps_free(audio);
            break;
        }

        Serial.printf("[RANDOM] codec:%s, sr:%d, ch:%d\n",
            audio->getCodecname(), audio->getSampleRate(), audio->getChannels());

        // 播放循环
        bool lastBtnUp = digitalRead(BTN_UP);
        bool lastBtnDown = digitalRead(BTN_DOWN);
        bool lastBtnOk = digitalRead(BTN_OK);
        bool lastBtnRight = digitalRead(BTN_RIGHT);
        unsigned long lastLog = 0;
        bool paused = false;
        bool wantNext = false;

        while (audio->isRunning() || paused) {
            if (!paused) audio->loop();

            if (millis() - lastLog > 2000) {
                lastLog = millis();
                Serial.printf("[RANDOM] %02d:%02d\n",
                    audio->getAudioCurrentTime()/60, audio->getAudioCurrentTime()%60);
            }

            // OK: 暂停/恢复
            bool btnOk = digitalRead(BTN_OK);
            if (lastBtnOk == HIGH && btnOk == LOW) {
                delay(20);
                if (digitalRead(BTN_OK) == LOW) {
                    paused = !paused;
                    if (paused) audio->stopSong();
                    else { audio->connecttohost(playUrl.c_str()); }
                    tft.fillRect(10, 5, 220, 16, TFT_BLACK);
                    tft.setTextColor(paused ? TFT_YELLOW : TFT_GREEN, TFT_BLACK);
                    tft.drawString(paused ? "已暂停" : "随机音乐", 10, 5);
                }
            }
            lastBtnOk = btnOk;

            // UP/DOWN: 音量
            bool btnUp = digitalRead(BTN_UP);
            if (lastBtnUp == HIGH && btnUp == LOW) {
                delay(20);
                if (digitalRead(BTN_UP) == LOW) {
                    if (vol < 21) { vol++; audio->setVolume(vol); }
                    tft.fillRect(10, 41, 100, 16, TFT_BLACK);
                    tft.setTextColor(TFT_CYAN, TFT_BLACK);
                    tft.drawString("Vol:" + String(vol), 10, 41);
                }
            }
            lastBtnUp = btnUp;

            bool btnDown = digitalRead(BTN_DOWN);
            if (lastBtnDown == HIGH && btnDown == LOW) {
                delay(20);
                if (digitalRead(BTN_DOWN) == LOW) {
                    if (vol > 0) { vol--; audio->setVolume(vol); }
                    tft.fillRect(10, 41, 100, 16, TFT_BLACK);
                    tft.setTextColor(TFT_CYAN, TFT_BLACK);
                    tft.drawString("Vol:" + String(vol), 10, 41);
                }
            }
            lastBtnDown = btnDown;

            // RIGHT: 切歌
            bool btnRight = digitalRead(BTN_RIGHT);
            if (lastBtnRight == HIGH && btnRight == LOW) {
                delay(20);
                if (digitalRead(BTN_RIGHT) == LOW) {
                    wantNext = true;
                    break;
                }
            }
            lastBtnRight = btnRight;

            // LEFT: 退出
            if (digitalRead(BTN_LEFT) == LOW) { delay(20); if (digitalRead(BTN_LEFT) == LOW) { wantExit = true; break; } }

            delay(5);
        }

        audio->stopSong();
        audio->~Audio();
        heap_caps_free(audio);

        if (wantExit) break;
        // wantNext: 继续循环获取下一首
    }

    digitalWrite(MAX98357A_SD, LOW);
    tft.fillScreen(TFT_BLACK);
}

// ========== WiFi 设置与连接模块 ==========

// 从NVS加载WiFi凭据并启动后台连接（非阻塞）
void loadWiFiCredentials() {
    // === RAM优化：preferences改为函数内局部变量 ===
    Preferences preferences;
    preferences.begin("wifi-config", true);  // 只读模式打开
    String ssid = preferences.getString("ssid", "");
    String password = preferences.getString("password", "");
    preferences.end();

    if (!ssid.isEmpty()) {
        CyberLogger::info("WIFI", "找到已保存的WiFi凭据: " + ssid);
        WiFi.mode(WIFI_STA);
        WiFi.begin(ssid.c_str(), password.c_str());  // 非阻塞，立即返回
        wifiConfigured = true;  // 标记WiFi凭据已加载，后台连接
        // ⚠️ 不再阻塞等待连接，改为后台连接
        CyberLogger::info("WIFI", "后台连接已启动，请稍候");
    } else {
        CyberLogger::warn("WIFI", "NVS中无WiFi凭据");
    }
}

// 保存WiFi凭据到NVS
void saveWiFiCredentials(String ssid, String password) {
    // === RAM优化：preferences改为函数内局部变量 ===
    Preferences preferences;
    preferences.begin("wifi-config", false);
    preferences.putString("ssid", ssid);
    preferences.putString("password", password);
    preferences.end();
    CyberLogger::info("WIFI", "凭据已保存");
}

// 清除WiFi凭据
void clearWiFiCredentials() {
    // === RAM优化：preferences改为函数内局部变量 ===
    Preferences preferences;
    preferences.begin("wifi-config", false);
    preferences.clear();
    preferences.end();
    CyberLogger::info("WIFI", "凭据已清除");
}

// 连接WiFi网络
void connectToWiFi(String ssid, String password) {
    tft.fillScreen(TFT_BLACK);
    loadFontSafe();
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString("正在连接 " + ssid + " ...", 10, 30);
    
    WiFi.begin(ssid.c_str(), password.c_str());
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(500);
        tft.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        tft.fillScreen(TFT_BLACK);
        tft.setTextColor(TFT_GREEN, TFT_BLACK);
        tft.drawString("连接成功!", 10, 30);
        tft.drawString("IP: " + WiFi.localIP().toString(), 10, 50);
        saveWiFiCredentials(ssid, password);
        wifiConfigured = true;
        delay(2000);
    } else {
        tft.fillScreen(TFT_BLACK);
        tft.setTextColor(TFT_RED, TFT_BLACK);
        tft.drawString("连接失败!", 10, 30);
        delay(2000);
    }
}

// ========== 桌面矢量图标绘制 ==========
void drawIconFileManager(int x, int y, uint16_t c) {
    // 文件夹: 矩形 + 折角
    tft.fillRoundRect(x, y + 4, 32, 24, 3, c);
    tft.fillRect(x, y, 14, 8, c);
    tft.fillRect(x, y + 4, 32, 24, 0x0000); // 内部镂空
    tft.fillRect(x + 3, y + 7, 26, 18, c);
    // 文档纸
    tft.fillRect(x + 10, y + 2, 16, 20, 0xFFFF);
    tft.drawLine(x + 13, y + 7, x + 23, y + 7, 0x4208);
    tft.drawLine(x + 13, y + 11, x + 23, y + 11, 0x4208);
    tft.drawLine(x + 13, y + 15, x + 20, y + 15, 0x4208);
}

void drawIconMusic(int x, int y, uint16_t c) {
    // 音符: 圆 + 竖线 + 横线
    tft.fillCircle(x + 8, y + 22, 6, c);
    tft.fillCircle(x + 24, y + 18, 6, c);
    tft.fillRect(x + 13, y + 4, 3, 20, c);
    tft.fillRect(x + 29, y + 2, 3, 18, c);
    tft.fillRect(x + 13, y + 2, 19, 4, c);
}

void drawIconAlarm(int x, int y, uint16_t c) {
    // 闹钟: 圆 + 两个小耳朵 + 指针
    tft.drawCircle(x + 16, y + 18, 14, c);
    tft.fillCircle(x + 16, y + 18, 12, 0x0000);
    // 耳朵
    tft.fillRect(x + 3, y + 6, 4, 6, c);
    tft.fillRect(x + 25, y + 6, 4, 6, c);
    // 指针
    tft.drawLine(x + 16, y + 18, x + 16, y + 10, 0xFFFF);
    tft.drawLine(x + 16, y + 18, x + 22, y + 18, 0xFFE0);
    tft.fillCircle(x + 16, y + 18, 2, c);
}

void drawIconPomodoro(int x, int y, uint16_t c) {
    // 番茄: 圆 + 叶子
    tft.fillCircle(x + 16, y + 18, 12, c);
    tft.fillCircle(x + 16, y + 18, 10, 0xF800); // 红色内部
    // 叶子
    tft.fillTriangle(x + 12, y + 7, x + 16, y + 2, x + 20, y + 7, 0x07E0);
    tft.drawLine(x + 16, y + 2, x + 16, y + 8, 0x0400);
}

void drawIconCalc(int x, int y, uint16_t c) {
    // 计算器: 矩形 + 按钮网格
    tft.fillRoundRect(x + 2, y, 28, 32, 3, c);
    tft.fillRect(x + 5, y + 3, 22, 8, 0xFFFF); // 屏幕
    // 按钮 (3x3)
    for (int r = 0; r < 3; r++) {
        for (int cc = 0; cc < 3; cc++) {
            tft.fillRect(x + 5 + cc * 8, y + 14 + r * 6, 6, 4, 0x0000);
        }
    }
}

void drawIconAI(int x, int y, uint16_t c) {
    // 机器人: 方形头 + 天线 + 眼睛
    tft.fillRoundRect(x + 4, y + 8, 24, 20, 4, c);
    // 天线
    tft.drawLine(x + 16, y + 8, x + 16, y + 2, c);
    tft.fillCircle(x + 16, y + 2, 2, c);
    // 眼睛
    tft.fillCircle(x + 11, y + 16, 3, 0x0000);
    tft.fillCircle(x + 21, y + 16, 3, 0x0000);
    tft.fillCircle(x + 11, y + 16, 1, 0xFFFF);
    tft.fillCircle(x + 21, y + 16, 1, 0xFFFF);
    // 嘴
    tft.drawLine(x + 10, y + 23, x + 22, y + 23, 0x0000);
}

void drawIconFC(int x, int y, uint16_t c) {
    // 游戏手柄: 椭圆 + 十字键 + 按钮
    tft.fillRoundRect(x + 2, y + 8, 28, 18, 8, c);
    // 十字键
    tft.fillRect(x + 8, y + 13, 10, 4, 0x0000);
    tft.fillRect(x + 11, y + 10, 4, 10, 0x0000);
    // 按钮
    tft.fillCircle(x + 22, y + 15, 3, 0xF800);
    tft.fillCircle(x + 26, y + 13, 3, 0xFFE0);
}

void drawIconClock(int x, int y, uint16_t c) {
    // 时钟: 圆 + 数字位 + 指针
    tft.drawCircle(x + 16, y + 16, 14, c);
    tft.fillCircle(x + 16, y + 16, 12, 0x0000);
    // 刻度
    for (int i = 0; i < 12; i++) {
        float angle = i * 30 * 3.14159 / 180;
        int x1 = x + 16 + (int)(11 * cos(angle));
        int y1 = y + 16 + (int)(11 * sin(angle));
        int x2 = x + 16 + (int)(13 * cos(angle));
        int y2 = y + 16 + (int)(13 * sin(angle));
        tft.drawLine(x1, y1, x2, y2, c);
    }
    // 指针
    tft.drawLine(x + 16, y + 16, x + 16, y + 8, 0xFFFF);
    tft.drawLine(x + 16, y + 16, x + 22, y + 14, 0xFFE0);
    tft.fillCircle(x + 16, y + 16, 2, c);
}

void drawIconStopwatch(int x, int y, uint16_t c) {
    // 秒表: 圆 + 顶部按钮
    tft.fillCircle(x + 16, y + 18, 13, c);
    tft.fillCircle(x + 16, y + 18, 11, 0x0000);
    tft.fillRect(x + 13, y + 2, 6, 5, c);
    tft.drawLine(x + 16, y + 18, x + 16, y + 10, 0xFFFF);
    tft.drawLine(x + 16, y + 18, x + 22, y + 16, 0xFFE0);
    tft.fillCircle(x + 16, y + 18, 2, c);
}

void drawIconCountdown(int x, int y, uint16_t c) {
    // 倒计时: 沙漏
    tft.fillTriangle(x + 4, y + 2, x + 28, y + 2, x + 16, y + 14, c);
    tft.fillTriangle(x + 4, y + 30, x + 28, y + 30, x + 16, y + 18, c);
    tft.fillRect(x + 4, y + 2, 24, 3, c);
    tft.fillRect(x + 4, y + 27, 24, 3, c);
    // 中间连接
    tft.drawLine(x + 14, y + 15, x + 18, y + 17, c);
    tft.drawLine(x + 14, y + 17, x + 18, y + 15, c);
}

void drawIconSearch(int x, int y, uint16_t c) {
    // 放大镜: 圆 + 手柄
    tft.drawCircle(x + 13, y + 13, 10, c);
    tft.drawCircle(x + 13, y + 13, 8, c);
    tft.drawLine(x + 20, y + 20, x + 28, y + 28, c);
    tft.drawLine(x + 21, y + 19, x + 29, y + 27, c);
}

void drawIconSettings(int x, int y, uint16_t c) {
    // 齿轮: 中心圆 + 齿
    tft.fillCircle(x + 16, y + 16, 6, c);
    tft.fillCircle(x + 16, y + 16, 3, 0x0000);
    // 8个齿
    for (int i = 0; i < 8; i++) {
        float angle = i * 45 * 3.14159 / 180;
        int x1 = x + 16 + (int)(8 * cos(angle));
        int y1 = y + 16 + (int)(8 * sin(angle));
        int x2 = x + 16 + (int)(13 * cos(angle));
        int y2 = y + 16 + (int)(13 * sin(angle));
        tft.drawLine(x1, y1, x2, y2, c);
        tft.drawLine(x1 + 1, y1, x2 + 1, y2, c);
    }
}

void drawIconAppleSpoof(int x, int y, uint16_t c) {
    // 苹果 + 信号波: 苹果轮廓 + 3条弧形信号线
    // 苹果主体
    tft.fillCircle(x + 14, y + 14, 8, c);
    tft.fillCircle(x + 22, y + 14, 8, c);
    tft.fillRect(x + 10, y + 12, 16, 10, c);
    // 叶子
    tft.fillTriangle(x + 16, y + 4, x + 19, y + 2, x + 21, y + 7, 0x07E0);
    // 咬一口 (黑色缺口)
    tft.fillCircle(x + 25, y + 14, 4, 0x10A2);
    // WiFi信号弧线 (用短线段模拟)
    for (int i = 0; i < 3; i++) {
        int r = 10 + i * 4;
        int cx = x + 16, cy = y + 26;
        for (int a = 210; a <= 330; a += 5) {
            float rad = a * 3.14159 / 180.0;
            int px = cx + (int)(r * cos(rad));
            int py = cy + (int)(r * sin(rad));
            tft.drawPixel(px, py, c);
            tft.drawPixel(px + 1, py, c);
        }
    }
}

void drawIconDashboard(int x, int y, uint16_t c) {
    // 仪表盘: 圆形表盘 + 刻度 + 指针
    tft.drawCircle(x + 16, y + 16, 14, c);
    tft.drawCircle(x + 16, y + 16, 12, 0x4208);
    // 刻度
    for (int i = 0; i < 8; i++) {
        float a = (i * 45 + 225) * 3.14159 / 180.0;
        int x1 = x + 16 + (int)(10 * cos(a));
        int y1 = y + 16 + (int)(10 * sin(a));
        int x2 = x + 16 + (int)(13 * cos(a));
        int y2 = y + 16 + (int)(13 * sin(a));
        tft.drawLine(x1, y1, x2, y2, c);
    }
    // 指针
    tft.drawLine(x + 16, y + 16, x + 16 + 9, y + 16 - 5, 0xF800);
    tft.fillCircle(x + 16, y + 16, 2, c);
}

void drawIconCalendar(int x, int y, uint16_t c) {
    // 日历: 矩形 + 红色顶部横条 + 白色主体 + 日期数字
    tft.fillRoundRect(x + 2, y + 2, 28, 28, 3, 0xFFFF);  // 白色主体
    tft.fillRect(x + 2, y + 2, 28, 8, 0xF800);            // 红色顶部
    // 两个挂钩
    tft.fillRect(x + 8, y, 2, 6, c);
    tft.fillRect(x + 22, y, 2, 6, c);
    // 日期数字
    tft.setTextColor(0x0000, 0xFFFF);
    tft.drawString("21", x + 8, y + 14);
}

void drawIconScriptEditor(int x, int y, uint16_t c) {
    // 脚本编辑器: 代码编辑器图标
    tft.fillRoundRect(x + 2, y + 2, 28, 28, 3, 0x4208);  // 深灰背景
    tft.drawRoundRect(x + 2, y + 2, 28, 28, 3, c);
    // 代码行
    tft.fillRect(x + 6, y + 7, 16, 2, c);
    tft.fillRect(x + 6, y + 12, 12, 2, 0xC618);
    tft.fillRect(x + 6, y + 17, 20, 2, c);
    tft.fillRect(x + 6, y + 22, 8, 2, 0xC618);
    // 光标
    tft.fillRect(x + 15, y + 22, 2, 4, 0xFFE0);
}

// 根据索引绘制对应图标
void drawDesktopIcon(int idx, int x, int y) {
    uint16_t c = desktopAppColor(idx);
    String action = String(desktopAppAction(idx));
    if (action == "FILE_MANAGER") drawIconFileManager(x, y, c);
    else if (action == "ONLINE_MUSIC") drawIconMusic(x, y, c);
    else if (action == "ALARM") drawIconAlarm(x, y, c);
    else if (action == "POMODORO") drawIconPomodoro(x, y, c);
    else if (action == "CALC") drawIconCalc(x, y, c);
    else if (action == "AI") drawIconAI(x, y, c);
    else if (action == "FC") drawIconFC(x, y, c);
    else if (action == "CLOCK") drawIconClock(x, y, c);
    else if (action == "STOPWATCH") drawIconStopwatch(x, y, c);
    else if (action == "COUNTDOWN") drawIconCountdown(x, y, c);
    else if (action == "FILESEARCH") drawIconSearch(x, y, c);
    else if (action == "APPLE_SPOOF") drawIconAppleSpoof(x, y, c);
    else if (action == "DASHBOARD") drawIconDashboard(x, y, c);
    else if (action == "CALENDAR") drawIconCalendar(x, y, c);
    else if (action == "SCRIPT_EDITOR") drawIconScriptEditor(x, y, c);
    else if (action == "SETTINGS") drawIconSettings(x, y, c);
    else if (action == "RANDOM_MUSIC") drawIconMusic(x, y, c);
    else {
        // 脚本应用: 彩色方块 + "B" 标记
        tft.fillRoundRect(x, y, 32, 32, 6, c);
        tft.setTextColor(0x0000, c);
        tft.drawString("B", x + 10, y + 10);
    }
}

// ========== 翻页滑动动画 ==========
// dir: -1 = 向右滑(上一页), +1 = 向左滑(下一页)
void animateDesktopSlide(int fromPage, int toPage, int dir, int newCursorX, int newCursorY) {
    loadFontSafe();
    int startX = 0, startY = 22;
    int cellW = 80, cellH = 66;
    int iconAreaH = DESKTOP_ROWS * cellH;  // 198
    int frames = 5;

    for (int f = 1; f <= frames; f++) {
        int offset = dir * (240 * f / frames);

        // 清除图标区域
        tft.fillRect(0, startY, 240, iconAreaH, TFT_BLACK);

        // 绘制旧页 (滑出)
        int oldOffset = -offset;  // 向左滑时 offset>0, 旧页往左移
        int oldPageOffset = fromPage * DESKTOP_PER_PAGE;
        for (int row = 0; row < DESKTOP_ROWS; row++) {
            for (int col = 0; col < DESKTOP_COLS; col++) {
                int idx = oldPageOffset + row * DESKTOP_COLS + col;
                if (idx >= desktopTotal()) break;
                int cx = startX + col * cellW + oldOffset;
                if (cx + cellW < 0 || cx > 240) continue;  // 屏幕外跳过
                int cy = startY + row * cellH;
                int ix = cx + (cellW - 32) / 2;
                int iy = cy + 4;
                drawDesktopIcon(idx, ix, iy);
                String appName = String(desktopAppName(idx));
                tft.setTextColor(0xFFFF, TFT_BLACK);
                int nw = tft.textWidth(appName);
                tft.drawString(appName, cx + (cellW - nw) / 2, cy + 42);
            }
        }

        // 绘制新页 (滑入)
        int newOffset = 240 - offset;  // 从右侧滑入: 240→0
        if (dir < 0) newOffset = -240 - offset;  // 从左侧滑入: -240→0
        int newPageOffset = toPage * DESKTOP_PER_PAGE;
        for (int row = 0; row < DESKTOP_ROWS; row++) {
            for (int col = 0; col < DESKTOP_COLS; col++) {
                int idx = newPageOffset + row * DESKTOP_COLS + col;
                if (idx >= desktopTotal()) break;
                int cx = startX + col * cellW + newOffset;
                if (cx + cellW < 0 || cx > 240) continue;
                int cy = startY + row * cellH;
                int ix = cx + (cellW - 32) / 2;
                int iy = cy + 4;
                drawDesktopIcon(idx, ix, iy);
                String appName = String(desktopAppName(idx));
                tft.setTextColor(0xFFFF, TFT_BLACK);
                int nw = tft.textWidth(appName);
                tft.drawString(appName, cx + (cellW - nw) / 2, cy + 42);
            }
        }

        delay(8);
        yield();
    }

    // 最终绘制 (含选中高亮和页码)
    desktopPage = toPage;
    desktopCursorX = newCursorX;
    desktopCursorY = newCursorY;
    drawDesktop();
}

// ========== 桌面绘制 ==========
void drawDesktop() {
    lastDesktopActivity = millis();
    Serial.printf("[DESKTOP] drawDesktop called, total=%d\n", desktopTotal());
    tft.fillScreen(TFT_BLACK);
    loadFontSafe();

    // 状态栏 (顶部 20px)
    tft.fillRect(0, 0, 240, 18, TFT_BLACK);
    tft.setTextColor(0x07E0, TFT_BLACK);
    if (ntpSynced) {
        char timeBuf[6];
        snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d", hour(), minute());
        tft.drawString(timeBuf, 5, 3);
    } else {
        tft.setTextColor(0x8410, TFT_BLACK);
        tft.drawString("--:--", 5, 3);
    }
    if (WiFi.isConnected()) {
        int rssi = WiFi.RSSI();
        int bars = 0;
        if (rssi >= -50) bars = 4;
        else if (rssi >= -60) bars = 3;
        else if (rssi >= -70) bars = 2;
        else bars = 1;
        int bx = 170, by = 4;
        for (int i = 0; i < 4; i++) {
            int bh = 4 + i * 2;
            uint16_t bc = (i < bars) ? 0x07E0 : 0x4208;
            tft.fillRect(bx + i * 5, by + (12 - bh), 3, bh, bc);
        }
        tft.setTextColor(0x07FF, TFT_BLACK);
        char rssiBuf[8];
        snprintf(rssiBuf, sizeof(rssiBuf), "%d", rssi);
        tft.drawString(rssiBuf, 195, 3);
    } else {
        tft.setTextColor(0x8410, TFT_BLACK);
        tft.drawString("NoWiFi", 185, 3);
    }
    tft.drawLine(0, 18, 240, 18, 0x4208);

    // 图标网格 (3x3), 每格 80x66px
    int startX = 0, startY = 22;
    int cellW = 80, cellH = 66;
    int pageOffset = desktopPage * DESKTOP_PER_PAGE;

    for (int row = 0; row < DESKTOP_ROWS; row++) {
        for (int col = 0; col < DESKTOP_COLS; col++) {
            int idx = pageOffset + row * DESKTOP_COLS + col;
            if (idx >= desktopTotal()) break;

            int cx = startX + col * cellW;
            int cy = startY + row * cellH;
            bool sel = (col == desktopCursorX && row == desktopCursorY);

            // 选中高亮背景
            if (sel) {
                tft.fillRoundRect(cx + 2, cy + 2, cellW - 4, cellH - 4, 6, 0x030D);
                tft.drawRoundRect(cx + 2, cy + 2, cellW - 4, cellH - 4, 6, 0x07FF);
            }

            // 矢量图标
            int ix = cx + (cellW - 32) / 2;
            int iy = cy + 4;
            drawDesktopIcon(idx, ix, iy);

            // 名称 (图标下方)
            String appName = String(desktopAppName(idx));
            tft.setTextColor(sel ? 0xFFE0 : 0xFFFF, TFT_BLACK);
            int nw = tft.textWidth(appName);
            tft.drawString(appName, cx + (cellW - nw) / 2, cy + 42);
        }
    }

    // 页码指示器
    int totalPages = (desktopTotal() + DESKTOP_PER_PAGE - 1) / DESKTOP_PER_PAGE;
    int dotY = 228;
    for (int p = 0; p < totalPages; p++) {
        int dotX = 110 + p * 12;
        tft.fillCircle(dotX, dotY, 3, p == desktopPage ? 0xFFFF : 0x4208);
    }
}

// ========== 屏幕自动休眠系统 (仅桌面模式) ==========
void wakeScreen() {
    if (!screenSleeping) return;
    Preferences prefs;
    prefs.begin("sys-config", true);
    uint8_t b = prefs.getUChar("brightness", 255);
    prefs.end();
    for (int i = 0; i <= SCREEN_FADE_STEPS; i++) {
        analogWrite(TFT_BL, b * i / SCREEN_FADE_STEPS);
        delay(SCREEN_FADE_DELAY);
        yield();
    }
    g_brightness = b;
    screenSleeping = false;
    drawDesktop();
}

void checkScreenSleep() {
    if (!onDesktop || screenSleeping) return;
    if (g_screenSleepMs > 0 && millis() - lastDesktopActivity >= g_screenSleepMs) {
        screenSleeping = true;
        for (int i = g_brightness; i > 0; i -= g_brightness / SCREEN_FADE_STEPS) {
            analogWrite(TFT_BL, max(i - g_brightness / SCREEN_FADE_STEPS, 0));
            delay(SCREEN_FADE_DELAY);
            yield();
        }
        analogWrite(TFT_BL, 0);
    }
}

// ========== 通用画面过渡动画 ==========
// 渐暗 → (此时调用方绘制新画面) → 渐亮
void animateTransition() {
    for (int i = SCREEN_FADE_STEPS; i >= 0; i--) {
        analogWrite(TFT_BL, g_brightness * i / SCREEN_FADE_STEPS);
        delay(SCREEN_FADE_DELAY);
        yield();
    }
}

void animateRestore() {
    for (int i = 1; i <= SCREEN_FADE_STEPS; i++) {
        analogWrite(TFT_BL, g_brightness * i / SCREEN_FADE_STEPS);
        delay(SCREEN_FADE_DELAY);
        yield();
    }
    analogWrite(TFT_BL, g_brightness);
}

// ========== 熄屏超时设置 ==========
void sleepSettings() {
    inMenu = false;
    loadFontSafe();

    // 读取保存的熄屏超时
    Preferences prefs;
    prefs.begin("sys-config", true);
    uint16_t timeout = prefs.getUShort("sleepTimeout", 15);
    prefs.end();
    const char* options[] = {"从不", "10秒", "15秒", "30秒", "1分钟", "3分钟"};
    const uint16_t values[] = {0, 10, 15, 30, 60, 180};
    int sel = 0;
    for (int i = 0; i < 6; i++) {
        if (values[i] == timeout) { sel = i; break; }
    }

    auto drawUI = [&]() {
        tft.fillScreen(TFT_BLACK);
        tft.setTextColor(0x07FF, TFT_BLACK);
        tft.drawString("熄屏休眠", 10, 10);
        tft.drawLine(0, 28, 240, 28, 0x4208);
        for (int i = 0; i < 6; i++) {
            int y = 34 + i * 28;
            if (i == sel) {
                tft.fillRect(0, y, 240, 26, 0x030D);
                tft.setTextColor(0xFFE0);
            } else {
                tft.setTextColor(0xFFFF);
            }
            tft.drawString(String(i == sel ? "> " : "  ") + options[i], 10, y + 5);
        }
        tft.setTextColor(0xC618, TFT_BLACK);
        tft.drawString("UP/DOWN:选择  OK:保存", 10, 210);
    };

    g_screenSleepMs = timeout > 0 ? timeout * 1000UL : 0;
    drawUI();
    while (true) {
        if (isBtnPressed(BTN_UP)) { if (sel > 0) sel--; drawUI(); }
        if (isBtnPressed(BTN_DOWN)) { if (sel < 5) sel++; drawUI(); }
        if (isBtnPressed(BTN_OK) || isBtnPressed(BTN_LEFT)) {
            // 保存到NVS
            if (values[sel] != timeout) {
                prefs.begin("sys-config", false);
                prefs.putUShort("sleepTimeout", values[sel]);
                prefs.end();
                g_screenSleepMs = values[sel] * 1000UL;
                CyberLogger::info("SLEEP", "熄屏超时设置为 " + String(options[sel]));
            }
            break;
        }
        delay(20);
    }
}

void launchDesktopApp(const char* action) {
    // === 过渡动画：渐暗 ===
    for (int i = SCREEN_FADE_STEPS; i >= 0; i--) {
        analogWrite(TFT_BL, g_brightness * i / SCREEN_FADE_STEPS);
        delay(SCREEN_FADE_DELAY);
        yield();
    }
    onDesktop = false;
    String act = String(action);

    if (act == "FILE_MANAGER") {
        inMenu = true;
        currentDir = "/";
        menuLoaded = false;
        loadMenu(currentDir);
        drawMenu();
        animateRestore();
        return;
    }
    else if (act == "ONLINE_MUSIC") { animateRestore(); searchMusicOnline(); }
    else if (act == "ALARM") { animateRestore(); alarmSettings(); }
    else if (act == "POMODORO") { animateRestore(); pomodoroTimer(); }
    else if (act == "CALC") { animateRestore(); runCalculator(); }
    else if (act == "AI") { animateRestore(); runAIChat(); }
    else if (act == "FC") { animateRestore(); fcGameBrowser(); }
    else if (act == "CLOCK") { animateRestore(); spacemanClock(); }
    else if (act == "STOPWATCH") { animateRestore(); stopwatchTimer(); }
    else if (act == "COUNTDOWN") { animateRestore(); countdownTimer(); }
    else if (act == "FILESEARCH") { animateRestore(); fileSearch(); }
    else if (act == "APPLE_SPOOF") { animateRestore(); appleJuiceControl(); }
    else if (act == "DASHBOARD") { animateRestore(); systemDashboard(); }
    else if (act == "CALENDAR") { animateRestore(); calendarApp(); }
    else if (act == "SCRIPT_EDITOR") { animateRestore(); scriptEditor(); }
    else if (act == "SETTINGS") { animateRestore(); settingsMenu(); }
    else if (act == "RANDOM_MUSIC") { animateRestore(); playRandomMusic(); }
    else if (act.startsWith("/apps/")) {
        // 模板应用
        String infoPath = act + "/app.info";
        AppConfig cfg;
        if (cfg.load(infoPath)) {
            tft.fillScreen(TFT_BLACK);
            loadFontSafe();
            animateRestore();
            runAppTemplate(cfg);
            tft.init();
            tft.setRotation(0);
            tft.invertDisplay(true);
            tft.fillScreen(TFT_BLACK);
        }
    }

    // 回退: 应用函数返回时走完整过渡
    animateTransition();
    onDesktop = true;
    drawDesktop();
    animateRestore();
}

// ========== 秒表 ==========
void stopwatchTimer() {
    inMenu = false;
    loadFontSafe();

    unsigned long startTime = 0;
    unsigned long elapsed = 0;
    bool running = false;
    std::vector<unsigned long> laps;

    auto drawUI = [&]() {
        tft.fillScreen(TFT_BLACK);
        tft.setTextColor(0x07FF, TFT_BLACK); // CYAN
        tft.drawString("Stopwatch", 10, 10);
        tft.drawLine(0, 28, 240, 28, 0x4208);

        // 时间显示 (大字)
        unsigned long ms = running ? (millis() - startTime + elapsed) : elapsed;
        int min = (ms / 60000) % 60;
        int sec = (ms / 1000) % 60;
        int cs = (ms / 10) % 100;
        char buf[12];
        snprintf(buf, sizeof(buf), "%02d:%02d.%02d", min, sec, cs);
        tft.setTextSize(2);
        tft.setTextColor(0xFFFF, TFT_BLACK);
        tft.drawString(buf, 40, 45);
        tft.setTextSize(1);

        // 圈数列表 (最近5圈)
        int showCount = (int)laps.size();
        if (showCount > 5) showCount = 5;
        for (int i = 0; i < showCount; i++) {
            int idx = laps.size() - 1 - i;
            unsigned long lapMs = laps[idx];
            int lm = (lapMs / 60000) % 60;
            int ls = (lapMs / 1000) % 60;
            int lc = (lapMs / 10) % 100;
            char lbuf[20];
            snprintf(lbuf, sizeof(lbuf), "#%d  %02d:%02d.%02d", idx + 1, lm, ls, lc);
            tft.setTextColor(0xC618, TFT_BLACK); // LIGHTGREY
            tft.drawString(lbuf, 10, 100 + i * 18);
        }

        tft.setTextColor(0xFFE0, TFT_BLACK); // YELLOW
        tft.drawString("OK: " + String(running ? "Lap/Pause" : "Start"), 10, 200);
        tft.drawString("UP: Reset  LEFT: Back", 10, 216);
    };

    drawUI();

    while (true) {
        if (running) {
            // 实时刷新
            static unsigned long lastRefresh = 0;
            if (millis() - lastRefresh >= 10) {
                lastRefresh = millis();
                drawUI();
            }
        }

        if (isBtnPressed(BTN_OK)) {
            if (!running) {
                startTime = millis();
                running = true;
            } else {
                // 记录圈
                unsigned long now = millis() - startTime + elapsed;
                laps.push_back(now);
            }
            drawUI();
        }
        // 长按OK暂停
        if (digitalRead(BTN_OK) == LOW && running) {
            unsigned long pressStart = millis();
            while (digitalRead(BTN_OK) == LOW) { delay(10); }
            if (millis() - pressStart > 600) {
                elapsed += millis() - startTime;
                running = false;
                drawUI();
            }
        }
        if (isBtnPressed(BTN_UP)) {
            running = false;
            elapsed = 0;
            laps.clear();
            drawUI();
        }
        if (isBtnPressed(BTN_LEFT)) {
            break;
        }
        delay(10);
    }
}

// ========== 倒计时器 ==========
void countdownTimer() {
    inMenu = false;
    loadFontSafe();

    int totalSeconds = 60;  // 默认1分钟
    unsigned long endTime = 0;
    bool running = false;

    auto drawUI = [&]() {
        tft.fillScreen(TFT_BLACK);
        tft.setTextColor(0x07FF, TFT_BLACK);
        tft.drawString("Countdown", 10, 10);
        tft.drawLine(0, 28, 240, 28, 0x4208);

        unsigned long sec;
        if (running) {
            unsigned long now = millis();
            sec = (now >= endTime) ? 0 : (endTime - now) / 1000;
        } else {
            sec = totalSeconds;
        }
        int min = sec / 60;
        int s = sec % 60;
        char buf[10];
        snprintf(buf, sizeof(buf), "%02d:%02d", min, s);
        tft.setTextSize(3);
        tft.setTextColor(running ? 0xFFE0 : 0xFFFF, TFT_BLACK);
        tft.drawString(buf, 50, 60);
        tft.setTextSize(1);

        // 进度条
        unsigned long totalMs = totalSeconds * 1000UL;
        unsigned long remainingMs = running ? ((millis() >= endTime) ? 0 : endTime - millis()) : totalMs;
        int barX = 10, barY = 130, barW = 220, barH = 14;
        tft.drawRect(barX, barY, barW, barH, 0x4208);
        int fillW = (int)((uint64_t)(barW - 2) * (totalMs - remainingMs) / totalMs);
        tft.fillRect(barX + 1, barY + 1, fillW, barH - 2, running ? 0xFFE0 : 0x07E0);

        tft.setTextColor(0xC618, TFT_BLACK);
        if (!running) {
            tft.drawString("UP/DOWN: Adjust time", 10, 160);
            tft.drawString("LEFT/RIGHT: -/+10s", 10, 176);
        }
        tft.drawString("OK: " + String(running ? "Stop" : "Start"), 10, 200);
        tft.drawString("LEFT: Back", 10, 216);
    };

    drawUI();

    while (true) {
        if (running) {
            if (millis() >= endTime) {
                running = false;
                // 闪烁提示
                for (int i = 0; i < 8; i++) {
                    pixels.setPixelColor(0, i % 2 ? 0 : pixels.Color(0, 255, 0));
                    pixels.show();
                    delay(150);
                }
                pixels.setPixelColor(0, 0);
                pixels.show();
                drawUI();
            } else {
                static unsigned long lastRefresh = 0;
                if (millis() - lastRefresh >= 200) {
                    lastRefresh = millis();
                    drawUI();
                }
            }
        }

        if (isBtnPressed(BTN_OK)) {
            if (!running) {
                endTime = millis() + (unsigned long)totalSeconds * 1000;
                running = true;
            } else {
                running = false;
            }
            drawUI();
        }
        if (!running) {
            if (isBtnPressed(BTN_UP)) {
                if (totalSeconds < 3600) totalSeconds += 60;
                drawUI();
            }
            if (isBtnPressed(BTN_DOWN)) {
                if (totalSeconds > 10) totalSeconds -= 60;
                drawUI();
            }
            if (isBtnPressed(BTN_RIGHT)) {
                if (totalSeconds < 3590) totalSeconds += 10;
                drawUI();
            }
        }
        if (isBtnPressed(BTN_LEFT)) {
            if (running) { running = false; }
            else break;
            drawUI();
        }
        delay(20);
    }
}

// ========== 日历 ==========
void calendarApp() {
    inMenu = false;
    onDesktop = false;
    loadFontSafe();

    int viewYear = year(), viewMonth = month();
    int cursorDay = day();
    int todayYear = year(), todayMonth = month(), todayDay = day();

    // 计算某月1号是星期几 (0=日, 1=一, ..., 6=六)
    auto firstDayOfWeek = [](int y, int m) -> int {
        // 使用蔡勒公式变体
        if (m < 3) { m += 12; y--; }
        int w = (1 + 2 * m + 3 * (m + 1) / 5 + y + y / 4 - y / 100 + y / 400) % 7;
        return (w + 1) % 7;  // 转换为 0=日, 1=一, ..., 6=六
    };

    // 计算某月天数
    auto daysInMonth = [](int y, int m) -> int {
        int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
        if (m == 2 && ((y % 4 == 0 && y % 100 != 0) || y % 400 == 0)) return 29;
        return days[m - 1];
    };

    auto drawCal = [&]() {
        tft.fillScreen(TFT_BLACK);
        unloadFontSafe(); loadFontSafe();

        // 标题栏: < 2026年5月 >
        tft.fillRect(0, 0, 240, 26, 0x2124);
        tft.setTextColor(0x07FF, 0x2124);
        char titleBuf[20];
        snprintf(titleBuf, sizeof(titleBuf), "%d/%d", viewYear, viewMonth);
        int tw = tft.textWidth(titleBuf);
        tft.drawString(titleBuf, (240 - tw) / 2, 5);
        tft.setTextColor(0x8410, 0x2124);
        tft.drawString("<", 8, 5);
        tft.drawString(">", 224, 5);
        tft.drawLine(0, 26, 240, 26, 0x4208);

        // 星期行
        const char* weekDays[] = {"Su", "Mo", "Tu", "We", "Th", "Fr", "Sa"};
        int cellW = 34, cellH = 26;
        int gridX = 0, gridY = 30;
        for (int i = 0; i < 7; i++) {
            uint16_t col = (i == 0 || i == 6) ? 0xF800 : 0xC618;  // 周末红色
            tft.setTextColor(col, TFT_BLACK);
            tft.drawString(weekDays[i], gridX + i * cellW + (cellW - tft.textWidth(weekDays[i])) / 2, gridY);
        }
        gridY += 22;

        // 日期网格
        int firstDay = firstDayOfWeek(viewYear, viewMonth);
        int totalDays = daysInMonth(viewYear, viewMonth);
        int rows = (firstDay + totalDays + 6) / 7;

        for (int d = 1; d <= totalDays; d++) {
            int pos = firstDay + d - 1;
            int col = pos % 7;
            int row = pos / 7;
            int cx = gridX + col * cellW;
            int cy = gridY + row * cellH;

            bool isToday = (viewYear == todayYear && viewMonth == todayMonth && d == todayDay);
            bool isCursor = (d == cursorDay);
            bool isWeekend = (col == 0 || col == 6);

            // 选中/今天高亮
            if (isCursor) {
                tft.fillRoundRect(cx + 1, cy, cellW - 2, cellH - 2, 4, 0x030D);
                tft.drawRoundRect(cx + 1, cy, cellW - 2, cellH - 2, 4, 0x07FF);
            } else if (isToday) {
                tft.fillRoundRect(cx + 1, cy, cellW - 2, cellH - 2, 4, 0x2124);
            }

            // 数字
            char dayBuf[4];
            snprintf(dayBuf, sizeof(dayBuf), "%d", d);
            uint16_t dayCol;
            if (isCursor) dayCol = 0xFFE0;
            else if (isToday) dayCol = 0x07FF;
            else if (isWeekend) dayCol = 0xF800;
            else dayCol = 0xFFFF;
            tft.setTextColor(dayCol, isCursor ? 0x030D : (isToday ? 0x2124 : TFT_BLACK));
            int dw = tft.textWidth(dayBuf);
            tft.drawString(dayBuf, cx + (cellW - dw) / 2, cy + 5);
        }

        // 底部信息
        int bottomY = gridY + rows * cellH + 4;
        tft.drawLine(0, bottomY, 240, bottomY, 0x4208);
        bottomY += 4;

        tft.setTextColor(0x07FF, TFT_BLACK);
        const char* weekDayNames[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
        // 今天日期
        char todayBuf[30];
        int wd = (todayYear > 0) ? 0 : 0;
        snprintf(todayBuf, sizeof(todayBuf), "%d-%02d-%02d %s",
            todayYear, todayMonth, todayDay, weekDayNames[((todayDay + firstDayOfWeek(todayYear, todayMonth)) % 7)]);
        tft.drawString(todayBuf, 10, bottomY);

        // 提示
        tft.setTextColor(0x8410, TFT_BLACK);
        tft.drawString("LR:Month UD:Day OK:Today", 10, bottomY + 16);
        tft.drawString("L(twice):Exit", 10, bottomY + 32);
    };

    drawCal();

    unsigned long lastLeftPress = 0;
    while (true) {
        bool redraw = false;

        if (isBtnPressed(BTN_LEFT)) {
            unsigned long now = millis();
            if (now - lastLeftPress < 400) break;  // 双击LEFT退出
            lastLeftPress = now;
            viewMonth--;
            if (viewMonth < 1) { viewMonth = 12; viewYear--; }
            int dim = daysInMonth(viewYear, viewMonth);
            if (cursorDay > dim) cursorDay = dim;
            redraw = true;
        }
        if (isBtnPressed(BTN_RIGHT)) {
            viewMonth++;
            if (viewMonth > 12) { viewMonth = 1; viewYear++; }
            int dim = daysInMonth(viewYear, viewMonth);
            if (cursorDay > dim) cursorDay = dim;
            redraw = true;
        }
        if (isBtnPressed(BTN_UP)) {
            cursorDay -= 7;
            if (cursorDay < 1) cursorDay = 1;
            redraw = true;
        }
        if (isBtnPressed(BTN_DOWN)) {
            cursorDay += 7;
            int dim = daysInMonth(viewYear, viewMonth);
            if (cursorDay > dim) cursorDay = dim;
            redraw = true;
        }
        if (isBtnPressed(BTN_OK)) {
            viewYear = todayYear;
            viewMonth = todayMonth;
            cursorDay = todayDay;
            redraw = true;
        }

        if (redraw) drawCal();
        delay(50);
    }

    unloadFontSafe();
    animateTransition();
    onDesktop = true;
    drawDesktop();
    animateRestore();
}

// ========== 脚本编辑器 ==========
void scriptEditor() {
    inMenu = false;
    onDesktop = false;
    loadFontSafe();

    if (!SD.exists("/apps")) SD.mkdir("/apps");

    // 浏览 /apps/ 下的应用文件夹
    std::vector<String> appDirs;
    std::vector<String> appNames;

    auto refreshList = [&]() {
        appDirs.clear();
        appNames.clear();
        File dir = SD.open("/apps");
        if (!dir) return;
        File f = dir.openNextFile();
        while (f) {
            if (f.isDirectory()) {
                String name = f.name();
                if (name != "." && name != "..") {
                    appDirs.push_back("/apps/" + name);
                    appNames.push_back(name);
                }
            }
            f = dir.openNextFile();
        }
        dir.close();
    };

    int selected = 0, topIdx = 0;
    refreshList();

    auto drawEditorUI = [&]() {
        tft.fillScreen(TFT_BLACK);
        tft.fillRect(0, 0, 240, 26, 0x2124);
        tft.setTextColor(0x07FF, 0x2124);
        tft.drawString("App Editor", 8, 5);
        tft.setTextColor(0x8410, 0x2124);
        char cnt[8]; snprintf(cnt, sizeof(cnt), "%d", (int)appDirs.size());
        tft.drawString(cnt, 200, 5);
        tft.drawLine(0, 26, 240, 26, 0x4208);

        if (appDirs.empty()) {
            tft.setTextColor(0xFFFF, TFT_BLACK);
            tft.drawString("No apps found", 50, 100);
            tft.setTextColor(0x8410, TFT_BLACK);
            tft.drawString("OK: Create new app", 50, 120);
        } else {
            int perPage = 8;
            for (int i = 0; i < perPage && topIdx + i < (int)appDirs.size(); i++) {
                int y = 32 + i * 22;
                int idx = topIdx + i;
                bool sel = (idx == selected);
                if (sel) tft.fillRect(0, y, 240, 22, 0x030D);
                tft.setTextColor(sel ? 0xFFE0 : 0xFFFF, TFT_BLACK);
                tft.drawString((sel ? "> " : "  ") + appNames[idx], 5, y + 3);
            }
        }

        tft.setTextColor(0x8410, TFT_BLACK);
        tft.drawString("UD:Nav OK:Edit R:Run", 20, 210);
        tft.drawString("LEFT:Back", 70, 226);
    };

    drawEditorUI();

    while (true) {
        if (isBtnPressed(BTN_UP)) {
            if (selected > 0) { selected--; if (selected < topIdx) topIdx = selected; drawEditorUI(); }
        }
        if (isBtnPressed(BTN_DOWN)) {
            if (selected < (int)appDirs.size() - 1) { selected++; if (selected >= topIdx + 8) topIdx = selected - 7; drawEditorUI(); }
        }
        if (isBtnPressed(BTN_LEFT)) break;

        if (isBtnPressed(BTN_OK)) {
            if (appDirs.empty()) {
                // 创建新模板应用
                unloadFontSafe();
                String name = inputTextDialog("App name:", "");
                loadFontSafe();
                if (name.length() > 0) {
                    String dirPath = "/apps/" + name;
                    SD.mkdir(dirPath);
                    String infoPath = dirPath + "/app.info";
                    File f = SD.open(infoPath, FILE_WRITE);
                    if (f) {
                        f.println("name=" + name);
                        f.println("color=65535");
                        f.println("template=info");
                        f.println("title=" + name);
                        f.println("line1=Hello World!");
                        f.close();
                    }
                    refreshList();
                    scanApps();
                }
                drawEditorUI();
            } else {
                // 编辑选中应用的 app.info
                String infoPath = appDirs[selected] + "/app.info";
                int editCursor = 0, editTop = 0;

                std::vector<String> editLines;
                File f = SD.open(infoPath);
                if (f) {
                    while (f.available()) {
                        String line = f.readStringUntil('\n');
                        line.trim();
                        if (line.endsWith("\r")) line = line.substring(0, line.length() - 1);
                        if (line.length() > 0) editLines.push_back(line);
                    }
                    f.close();
                }
                if (editLines.empty()) {
                    editLines.push_back("name=" + appNames[selected]);
                    editLines.push_back("color=65535");
                    editLines.push_back("template=info");
                    editLines.push_back("title=" + appNames[selected]);
                }

                auto drawEditUI = [&]() {
                    tft.fillScreen(TFT_BLACK);
                    tft.fillRect(0, 0, 240, 22, 0x2124);
                    tft.setTextColor(0x07FF, 0x2124);
                    String title = appNames[selected] + "/app.info";
                    if (title.length() > 22) title = title.substring(0, 22);
                    tft.drawString(title, 4, 4);
                    char pos[12]; snprintf(pos, sizeof(pos), "%d/%d", editCursor + 1, (int)editLines.size());
                    tft.setTextColor(0x8410, 0x2124);
                    tft.drawString(pos, 190, 4);
                    tft.drawLine(0, 22, 240, 22, 0x4208);

                    int perPage = 8;
                    for (int i = 0; i < perPage && editTop + i < (int)editLines.size(); i++) {
                        int y = 26 + i * 24;
                        int idx = editTop + i;
                        bool sel = (idx == editCursor);
                        if (sel) tft.fillRect(0, y, 240, 22, 0x030D);
                        tft.setTextColor(sel ? 0xFFE0 : 0xC618, sel ? 0x030D : TFT_BLACK);
                        String ln = editLines[idx];
                        if (ln.length() > 28) ln = ln.substring(0, 28);
                        tft.drawString(ln, 4, y + 3);
                    }

                    tft.setTextColor(0x8410, TFT_BLACK);
                    tft.drawString("UD:Line OK:Edit R:Run", 16, 210);
                    tft.drawString("LEFT:Save&Back", 52, 226);
                };

                drawEditUI();

                while (true) {
                    if (isBtnPressed(BTN_UP)) {
                        if (editCursor > 0) { editCursor--; if (editCursor < editTop) editTop = editCursor; drawEditUI(); }
                    }
                    if (isBtnPressed(BTN_DOWN)) {
                        if (editCursor < (int)editLines.size() - 1) { editCursor++; if (editCursor >= editTop + 8) editTop = editCursor - 7; drawEditUI(); }
                    }
                    if (isBtnPressed(BTN_OK)) {
                        unloadFontSafe();
                        String newLine = inputTextDialog("Line:", editLines[editCursor]);
                        loadFontSafe();
                        if (newLine.length() > 0) {
                            editLines[editCursor] = newLine;
                        }
                        drawEditUI();
                    }
                    if (isBtnPressed(BTN_RIGHT)) {
                        // 保存并运行
                        File sf = SD.open(infoPath, FILE_WRITE);
                        if (sf) {
                            for (auto& l : editLines) { sf.println(l); }
                            sf.close();
                        }
                        // 重新加载并运行模板
                        AppConfig cfg;
                        if (cfg.load(infoPath)) {
                            tft.fillScreen(TFT_BLACK);
                            runAppTemplate(cfg);
                            tft.init();
                            tft.setRotation(0);
                            tft.invertDisplay(true);
                            tft.fillScreen(TFT_BLACK);
                        }
                        loadFontSafe();
                        drawEditUI();
                    }
                    if (isBtnPressed(BTN_LEFT)) {
                        // 保存并退出
                        File sf = SD.open(infoPath, FILE_WRITE);
                        if (sf) {
                            for (auto& l : editLines) { sf.println(l); }
                            sf.close();
                        }
                        break;
                    }
                    delay(30);
                }

                drawEditorUI();
            }
        }

        if (isBtnPressed(BTN_RIGHT) && !appDirs.empty()) {
            // 直接运行选中的应用
            String infoPath = appDirs[selected] + "/app.info";
            AppConfig cfg;
            if (cfg.load(infoPath)) {
                tft.fillScreen(TFT_BLACK);
                loadFontSafe();
                runAppTemplate(cfg);
                tft.init();
                tft.setRotation(0);
                tft.invertDisplay(true);
                tft.fillScreen(TFT_BLACK);
            }
            loadFontSafe();
            drawEditorUI();
        }

        delay(30);
    }

    unloadFontSafe();
    animateTransition();
    onDesktop = true;
    drawDesktop();
    animateRestore();
}

// ========== 文件搜索 ==========
void fileSearch() {
    inMenu = false;
    loadFontSafe();

    // 简单输入: 用上下键选字母，OK确认
    String keyword = "";
    bool searching = true;
    std::vector<String> results;
    int resultIdx = 0;
    int resultTop = 0;

    auto drawSearchUI = [&]() {
        tft.fillScreen(TFT_BLACK);
        tft.setTextColor(0x07FF, TFT_BLACK);
        tft.drawString("File Search", 10, 10);
        tft.drawLine(0, 28, 240, 28, 0x4208);

        tft.setTextColor(0xFFFF, TFT_BLACK);
        tft.drawString("Search: " + keyword + "_", 10, 38);

        if (results.empty() && keyword.length() > 0) {
            tft.setTextColor(0xF800, TFT_BLACK);
            tft.drawString("No results", 10, 65);
        }

        int perPage = 7;
        for (int i = 0; i < perPage && resultTop + i < (int)results.size(); i++) {
            int y = 65 + i * 20;
            int idx = resultTop + i;
            uint16_t color = (idx == resultIdx) ? 0xFFE0 : 0xC618;
            tft.setTextColor(color, TFT_BLACK);
            String display = results[idx];
            if (display.length() > 30) display = display.substring(0, 30);
            tft.drawString((idx == resultIdx ? "> " : "  ") + display, 5, y);
        }

        tft.setTextColor(0xC618, TFT_BLACK);
        tft.drawString("Type: U/D letter OK:Search", 5, 200);
        tft.drawString("L/R:cursor  LEFT:Back", 5, 216);
    };

    // 递归搜索函数
    std::function<void(File&, String)> searchDir;
    searchDir = [&](File &dir, String path) {
        File file = dir.openNextFile();
        while (file) {
            String name = file.name();
            String fullPath = path + "/" + name;
            if (file.isDirectory()) {
                if (name != "." && name != "..") {
                    searchDir(file, fullPath);
                }
            } else {
                name.toLowerCase();
                String kw = keyword;
                kw.toLowerCase();
                if (name.indexOf(kw) >= 0) {
                    results.push_back(fullPath);
                }
            }
            file = dir.openNextFile();
        }
    };

    drawSearchUI();

    // 字母选择器
    int cursorPos = 0;  // 光标位置
    char currentChar = 'a';

    while (true) {
        if (isBtnPressed(BTN_UP)) {
            if (results.empty()) {
                currentChar++;
                if (currentChar > 'z') currentChar = 'a';
            } else {
                if (resultIdx > 0) resultIdx--;
                if (resultIdx < resultTop) resultTop = resultIdx;
            }
            drawSearchUI();
        }
        if (isBtnPressed(BTN_DOWN)) {
            if (results.empty()) {
                currentChar--;
                if (currentChar < 'a') currentChar = 'z';
            } else {
                if (resultIdx < (int)results.size() - 1) resultIdx++;
                if (resultIdx >= resultTop + 7) resultTop = resultIdx - 6;
            }
            drawSearchUI();
        }
        if (isBtnPressed(BTN_LEFT)) {
            if (keyword.length() > 0) {
                keyword.remove(keyword.length() - 1);
                results.clear();
                resultIdx = 0;
            } else {
                break;
            }
            drawSearchUI();
        }
        if (isBtnPressed(BTN_RIGHT)) {
            if (results.empty()) {
                keyword += currentChar;
                // 执行搜索
                results.clear();
                resultIdx = 0;
                File root = SD.open("/");
                if (root) {
                    searchDir(root, "");
                    root.close();
                }
            }
            drawSearchUI();
        }
        if (isBtnPressed(BTN_OK)) {
            if (results.empty()) {
                // 输入当前字符并搜索
                keyword += currentChar;
                results.clear();
                resultIdx = 0;
                File root = SD.open("/");
                if (root) {
                    searchDir(root, "");
                    root.close();
                }
            } else {
                // 打开选中的文件
                String path = results[resultIdx];
                pendingTask = path;
                hasTask = true;
                break;
            }
            drawSearchUI();
        }
        delay(50);
    }
}

// ========== 亮度调节 ==========
void brightnessControl() {
    inMenu = false;
    loadFontSafe();

    // 读取保存的亮度
    Preferences prefs;
    prefs.begin("sys-config", true);
    uint8_t brightness = prefs.getUChar("brightness", 255);
    prefs.end();
    g_brightness = brightness;

    auto drawUI = [&](uint8_t b) {
        tft.fillScreen(TFT_BLACK);
        tft.setTextColor(0x07FF, TFT_BLACK);
        tft.drawString("Brightness", 10, 10);
        tft.drawLine(0, 28, 240, 28, 0x4208);

        tft.setTextColor(0xFFFF, TFT_BLACK);
        tft.drawString("Level: " + String(b * 100 / 255) + "%", 10, 40);

        int barX = 10, barY = 65, barW = 220, barH = 18;
        tft.drawRect(barX, barY, barW, barH, 0x4208);
        tft.fillRect(barX + 1, barY + 1, (barW - 2) * b / 255, barH - 2, 0xFFE0);

        tft.setTextColor(0xC618, TFT_BLACK);
        tft.drawString("UP/DOWN: Adjust", 10, 110);
        tft.drawString("LEFT: Save & Back", 10, 128);
    };

    analogWrite(TFT_BL, brightness);
    drawUI(brightness);

    while (true) {
        if (isBtnPressed(BTN_UP)) {
            if (brightness < 255) { brightness += 25; if (brightness > 255) brightness = 255; }
            analogWrite(TFT_BL, brightness);
            drawUI(brightness);
        }
        if (isBtnPressed(BTN_DOWN)) {
            if (brightness > 25) { brightness -= 25; } else { brightness = 0; }
            analogWrite(TFT_BL, brightness);
            drawUI(brightness);
        }
        if (isBtnPressed(BTN_LEFT) || isBtnPressed(BTN_OK)) {
            break;
        }
        delay(30);
    }

    // 保存
    prefs.begin("sys-config", false);
    prefs.putUChar("brightness", brightness);
    prefs.end();
    g_brightness = brightness;
}

// ========== 设置子菜单 ==========
void settingsMenu() {
    inMenu = false;
    loadFontSafe();

    const char* settingsItems[] = {
        "WiFi设置", "亮度调节", "灯光控制", "熄屏休眠", "FC设置",
        "系统监控", "输入法"
    };
    int settingsCount = 7;
    int selected = 0;
    int topIdx = 0;

    auto drawSettingsUI = [&]() {
        tft.fillScreen(TFT_BLACK);
        tft.setTextColor(0x07FF, TFT_BLACK);
        tft.drawString("Settings", 10, 10);
        tft.drawLine(0, 28, 240, 28, 0x4208);

        int perPage = 8;
        for (int i = 0; i < perPage && topIdx + i < settingsCount; i++) {
            int y = 34 + i * 22;
            int idx = topIdx + i;
            uint16_t color = (idx == selected) ? 0xFFE0 : 0xFFFF;
            tft.setTextColor(color, TFT_BLACK);
            if (idx == selected) {
                tft.fillRect(0, y, 240, 22, 0x030D);
            }
            tft.drawString((idx == selected ? "> " : "  ") + String(settingsItems[idx]), 5, y + 3);
        }

        tft.setTextColor(0xC618, TFT_BLACK);
        tft.drawString("UP/DOWN:Select OK:Enter", 5, 216);
    };

    drawSettingsUI();

    while (true) {
        if (isBtnPressed(BTN_UP)) {
            if (selected > 0) selected--;
            if (selected < topIdx) topIdx = selected;
            drawSettingsUI();
        }
        if (isBtnPressed(BTN_DOWN)) {
            if (selected < settingsCount - 1) selected++;
            if (selected >= topIdx + 8) topIdx = selected - 7;
            drawSettingsUI();
        }
        if (isBtnPressed(BTN_OK)) {
            String item = String(settingsItems[selected]);
            if (item == "WiFi设置") { wifiSettings(); }
            else if (item == "亮度调节") { brightnessControl(); }
            else if (item == "灯光控制") { ledControlScreen(); }
            else if (item == "熄屏休眠") { sleepSettings(); }
            else if (item == "FC设置") { fcSettings(); }
            else if (item == "系统监控") { displaySystemMonitor(); }
            else if (item == "输入法") { drawInputUI(); }
            drawSettingsUI();
        }
        if (isBtnPressed(BTN_LEFT)) {
            break;
        }
        delay(30);
    }
}

// ========== 闹钟持久化 ==========
void loadAlarms() {
    Preferences prefs;
    prefs.begin("alarm-config", true);
    for (int i = 0; i < MAX_ALARMS; i++) {
        String key = "a" + String(i);
        alarms[i].hour = prefs.getUChar((key + "h").c_str(), 7);
        alarms[i].minute = prefs.getUChar((key + "m").c_str(), 0);
        alarms[i].repeatDays = prefs.getUChar((key + "d").c_str(), 0);
        alarms[i].enabled = prefs.getBool((key + "e").c_str(), false);
        String rt = prefs.getString((key + "r").c_str(), "");
        strncpy(alarms[i].ringtone, rt.c_str(), 63);
        alarms[i].ringtone[63] = 0;
    }
    alarmVolume = prefs.getUChar("vol", 50);
    prefs.end();
}

void saveAlarms() {
    Preferences prefs;
    prefs.begin("alarm-config", false);
    for (int i = 0; i < MAX_ALARMS; i++) {
        String key = "a" + String(i);
        prefs.putUChar((key + "h").c_str(), alarms[i].hour);
        prefs.putUChar((key + "m").c_str(), alarms[i].minute);
        prefs.putUChar((key + "d").c_str(), alarms[i].repeatDays);
        prefs.putBool((key + "e").c_str(), alarms[i].enabled);
        prefs.putString((key + "r").c_str(), String(alarms[i].ringtone));
    }
    prefs.putUChar("vol", alarmVolume);
    prefs.end();
}

// ========== 闹钟检测与响铃 ==========
void checkAlarms() {
    uint8_t h = hour(), m = minute();
    uint8_t wd = weekday(); // 1=Sun, 2=Mon..7=Sat
    uint8_t dayBit = (wd == 1) ? (1 << 6) : (1 << (wd - 2));

    for (int i = 0; i < MAX_ALARMS; i++) {
        if (!alarms[i].enabled || alarmFiredToday[i]) continue;
        if (alarms[i].hour != h || alarms[i].minute != m) continue;
        if (alarms[i].repeatDays != 0 && !(alarms[i].repeatDays & dayBit)) continue;
        alarmTriggered = true;
        alarmTriggerTime = millis();
        alarmTriggeredIndex = i;
        alarmFiredToday[i] = true;
        if (alarms[i].repeatDays == 0) {
            alarms[i].enabled = false;
            saveAlarms();
        }
        CyberLogger::info("ALARM", "闹钟触发 #" + String(i));
        break;
    }
    // 跨天重置
    static uint8_t lastDay = 0;
    if (hour() == 0 && minute() == 0 && lastDay != day()) {
        memset(alarmFiredToday, 0, sizeof(alarmFiredToday));
        lastDay = day();
    }
}

void handleAlarmRinging() {
    // 获取触发闹钟的铃声路径
    String ringPath = "";
    if (alarmTriggeredIndex >= 0 && alarmTriggeredIndex < MAX_ALARMS) {
        ringPath = String(alarms[alarmTriggeredIndex].ringtone);
    }
    Serial.printf("[ALARM] ringPath='%s' len=%d\n", ringPath.c_str(), ringPath.length());

    // 如果有自定义铃声，启动MP3播放
    Audio *alarmAudio = nullptr;
    if (ringPath.length() > 0) {
        pinMode(MAX98357A_SD, OUTPUT);
        digitalWrite(MAX98357A_SD, HIGH);
        alarmAudio = new Audio();
        alarmAudio->setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
        alarmAudio->setVolume(alarmVolume * 21 / 100);  // 0-100 → 0-21
        bool ok = alarmAudio->connecttoFS(SD, ringPath.c_str());
        Serial.printf("[ALARM] connecttoFS=%d isRunning=%d\n", ok, alarmAudio->isRunning());
        // 等待解码器启动
        unsigned long t = millis() + 3000;
        while (!alarmAudio->isRunning() && millis() < t) {
            alarmAudio->loop();
            delay(10);
        }
        Serial.printf("[ALARM] after wait isRunning=%d\n", alarmAudio->isRunning());
    }

    // 清除MP3播放器可能绘制的UI，绘制闹钟UI
    tft.fillScreen(TFT_BLACK);
    loadFontSafe();
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.setTextSize(2);
    tft.drawString("ALARM!", 60, 40);
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    if (alarmTriggeredIndex >= 0 && alarmTriggeredIndex < MAX_ALARMS) {
        char timeStr[6];
        snprintf(timeStr, sizeof(timeStr), "%02d:%02d",
                 alarms[alarmTriggeredIndex].hour, alarms[alarmTriggeredIndex].minute);
        tft.drawString(String(timeStr), 95, 80);
        if (ringPath.length() > 0) {
            String name = ringPath.substring(ringPath.lastIndexOf('/') + 1);
            tft.setTextColor(TFT_CYAN, TFT_BLACK);
            tft.drawString(name, 10, 110);
        }
    }
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.drawString("Press any key to stop", 30, 160);

    // 主循环：音频播放 + LED闪烁 + 按键检测
    bool ledOn = false;
    unsigned long lastBlink = 0;
    while (true) {
        // 音频循环
        if (alarmAudio && alarmAudio->isRunning()) {
            alarmAudio->loop();
        }

        // LED 闪烁
        if (millis() - lastBlink >= 300) {
            lastBlink = millis();
            ledOn = !ledOn;
            pixels.setPixelColor(0, ledOn ? pixels.Color(255, 0, 0) : 0);
            pixels.show();
        }

        // 任意键关闭
        if (isBtnPressed(BTN_UP) || isBtnPressed(BTN_DOWN) ||
            isBtnPressed(BTN_LEFT) || isBtnPressed(BTN_RIGHT) || isBtnPressed(BTN_OK)) {
            break;
        }

        // 60秒超时自动关闭
        if (millis() - alarmTriggerTime > 60000) break;

        delay(10);
    }

    // 清理：停止音频
    if (alarmAudio) {
        alarmAudio->stopSong();
        delete alarmAudio;
        digitalWrite(MAX98357A_SD, LOW);
    }

    alarmTriggered = false;
    alarmTriggeredIndex = -1;
    pixels.setPixelColor(0, 0);
    pixels.show();
    ledMode = LED_STATIC;
    handleLedAnimation();
}

// ========== MP3文件选择器 ==========
String pickMp3File() {
    loadFontSafe();
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.drawString("Select MP3...", 10, 10);

    // 扫描SD卡根目录的mp3文件
    std::vector<String> mp3Files;
    File root = SD.open("/");
    if (root) {
        File file = root.openNextFile();
        while (file) {
            String name = file.name();
            if (!file.isDirectory() && name.endsWith(".mp3")) {
                mp3Files.push_back(name);
            }
            file = root.openNextFile();
        }
        root.close();
    }

    // 也扫描 /Music 目录
    File musicDir = SD.open("/Music");
    if (!musicDir) musicDir = SD.open("/music");
    if (musicDir) {
        File file = musicDir.openNextFile();
        while (file) {
            String name = file.name();
            if (!file.isDirectory() && name.endsWith(".mp3")) {
                mp3Files.push_back("/Music/" + name);
            }
            file = musicDir.openNextFile();
        }
        musicDir.close();
    }

    if (mp3Files.empty()) {
        tft.setTextColor(TFT_RED, TFT_BLACK);
        tft.drawString("No MP3 files found!", 10, 50);
        delay(1500);
        return "";
    }

    int selected = 0;
    int topIdx = 0;
    int perPage = 8;

    auto drawList = [&]() {
        tft.fillScreen(TFT_BLACK);
        tft.setTextColor(TFT_CYAN, TFT_BLACK);
        tft.drawString("Pick MP3 (" + String(mp3Files.size()) + ")", 10, 5);
        tft.drawLine(0, 22, tft.width(), 22, TFT_DARKGREY);

        for (int i = 0; i < perPage && topIdx + i < (int)mp3Files.size(); i++) {
            int y = 28 + i * 22;
            int idx = topIdx + i;
            uint16_t color = (idx == selected) ? TFT_YELLOW : TFT_WHITE;
            tft.setTextColor(color, TFT_BLACK);
            String display = mp3Files[idx];
            int slash = display.lastIndexOf('/');
            if (slash >= 0) display = display.substring(slash + 1);
            if (display.length() > 28) display = display.substring(0, 28);
            tft.drawString((idx == selected ? "> " : "  ") + display, 5, y);
        }

        tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
        tft.drawString("UP/DOWN:Select OK:Pick LEFT:Cancel", 5, 218);
    };

    drawList();

    while (true) {
        if (isBtnPressed(BTN_UP)) {
            if (selected > 0) {
                selected--;
                if (selected < topIdx) topIdx = selected;
            }
            drawList();
        }
        if (isBtnPressed(BTN_DOWN)) {
            if (selected < (int)mp3Files.size() - 1) {
                selected++;
                if (selected >= topIdx + perPage) topIdx = selected - perPage + 1;
            }
            drawList();
        }
        if (isBtnPressed(BTN_OK)) {
            return mp3Files[selected];
        }
        if (isBtnPressed(BTN_LEFT)) {
            return "";
        }
        delay(50);
    }
}

// ========== 闹钟设置界面 ==========
const char* dayNames[] = {"一", "二", "三", "四", "五", "六", "日"};

void alarmSettings() {
    inMenu = false;
    loadFontSafe();

    int selected = 0;  // 当前选中的闹钟槽位
    bool editing = false;
    int editField = 0; // 0=时, 1=分, 2=重复日, 3=开关

    auto drawAlarmList = [&]() {
        tft.fillScreen(TFT_BLACK);
        tft.setTextColor(TFT_CYAN, TFT_BLACK);
        tft.drawString("闹钟设置", 10, 10);
        tft.drawLine(0, 28, tft.width(), 28, TFT_DARKGREY);

        for (int i = 0; i < MAX_ALARMS; i++) {
            int y = 40 + i * 36;
            uint16_t color = (i == selected) ? TFT_YELLOW : TFT_WHITE;
            tft.setTextColor(color, TFT_BLACK);

            String line = String(i + 1) + ". ";
            if (alarms[i].enabled) {
                char timeStr[6];
                snprintf(timeStr, sizeof(timeStr), "%02d:%02d", alarms[i].hour, alarms[i].minute);
                line += timeStr;
                line += " [";
                if (alarms[i].repeatDays == 0) {
                    line += "一次";
                } else if (alarms[i].repeatDays == 0x7F) {
                    line += "每天";
                } else {
                    bool first = true;
                    for (int d = 0; d < 7; d++) {
                        if (alarms[i].repeatDays & (1 << d)) {
                            if (!first) line += ",";
                            line += dayNames[d];
                            first = false;
                        }
                    }
                }
                line += "]";
            } else {
                line += "--:-- [--]";
            }
            tft.drawString(line, 10, y);
            tft.setTextColor(alarms[i].enabled ? TFT_GREEN : TFT_DARKGREY, TFT_BLACK);
            tft.drawString(alarms[i].enabled ? "ON" : "OFF", 200, y);
        }

        tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
        tft.drawString("UP/DOWN: Select", 10, 180);
        tft.drawString("OK: Edit  LEFT: Back", 10, 196);
    };

    auto drawEditView = [&](int idx) {
        tft.fillScreen(TFT_BLACK);
        tft.setTextColor(TFT_CYAN, TFT_BLACK);
        tft.drawString("Edit Alarm #" + String(idx + 1), 10, 10);
        tft.drawLine(0, 28, tft.width(), 28, TFT_DARKGREY);

        // 时间
        char timeStr[6];
        snprintf(timeStr, sizeof(timeStr), "%02d:%02d", alarms[idx].hour, alarms[idx].minute);
        tft.setTextColor((editField == 0 || editField == 1) ? TFT_YELLOW : TFT_WHITE, TFT_BLACK);
        tft.drawString("Time: " + String(timeStr), 10, 40);
        // 高亮当前编辑的时/分
        if (editField == 0) {
            tft.drawRect(56, 38, 24, 18, TFT_YELLOW);
        } else if (editField == 1) {
            tft.drawRect(82, 38, 24, 18, TFT_YELLOW);
        }

        // 重复日
        tft.setTextColor(editField == 2 ? TFT_YELLOW : TFT_WHITE, TFT_BLACK);
        String repeatStr = "Repeat: ";
        if (alarms[idx].repeatDays == 0) repeatStr += "One-time";
        else if (alarms[idx].repeatDays == 0x7F) repeatStr += "Every day";
        else {
            bool first = true;
            for (int d = 0; d < 7; d++) {
                if (alarms[idx].repeatDays & (1 << d)) {
                    if (!first) repeatStr += ",";
                    repeatStr += dayNames[d];
                    first = false;
                }
            }
        }
        tft.drawString(repeatStr, 10, 70);
        // 显示7个日格子
        for (int d = 0; d < 7; d++) {
            int gx = 10 + d * 32;
            int gy = 90;
            bool selected_day = (alarms[idx].repeatDays & (1 << d));
            uint16_t gc = selected_day ? TFT_GREEN : TFT_DARKGREY;
            if (editField == 2) {
                tft.drawRect(gx - 1, gy - 1, 26, 18, TFT_YELLOW);
            }
            tft.setTextColor(gc, TFT_BLACK);
            tft.drawString(dayNames[d], gx + 4, gy);
        }

        // 开关
        tft.setTextColor(editField == 3 ? TFT_YELLOW : TFT_WHITE, TFT_BLACK);
        tft.drawString("Enabled: " + String(alarms[idx].enabled ? "YES" : "NO"), 10, 120);

        // 铃声
        tft.setTextColor(editField == 4 ? TFT_YELLOW : TFT_WHITE, TFT_BLACK);
        String ringName = strlen(alarms[idx].ringtone) > 0 ? String(alarms[idx].ringtone) : "Default";
        int sl = ringName.lastIndexOf('/');
        if (sl >= 0) ringName = ringName.substring(sl + 1);
        if (ringName.length() > 24) ringName = ringName.substring(0, 24);
        tft.drawString("Ring: " + ringName, 10, 140);

        // 音量
        tft.setTextColor(editField == 5 ? TFT_YELLOW : TFT_WHITE, TFT_BLACK);
        char volStr[20];
        snprintf(volStr, sizeof(volStr), "Volume: %d%%", alarmVolume);
        tft.drawString(volStr, 10, 160);
        // 进度条
        tft.drawRect(120, 158, 100, 12, TFT_DARKGREY);
        tft.fillRect(121, 159, 98 * alarmVolume / 100, 10, TFT_GREEN);

        tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
        tft.drawString("L/R: Field  U/D: Change", 10, 182);
        tft.drawString("OK: Pick mp3 / Toggle", 10, 198);
    };

    drawAlarmList();

    while (true) {
        if (!editing) {
            // 列表模式
            if (isBtnPressed(BTN_UP)) {
                if (selected > 0) selected--;
                drawAlarmList();
            }
            if (isBtnPressed(BTN_DOWN)) {
                if (selected < MAX_ALARMS - 1) selected++;
                drawAlarmList();
            }
            if (isBtnPressed(BTN_OK)) {
                editing = true;
                editField = 0;
                drawEditView(selected);
            }
            if (isBtnPressed(BTN_LEFT)) {
                break;  // 返回菜单
            }
        } else {
            // 编辑模式
            if (isBtnPressed(BTN_LEFT)) {
                if (editField > 0) {
                    editField--;
                    drawEditView(selected);
                } else {
                    editing = false;
                    saveAlarms();
                    drawAlarmList();
                }
            }
            if (isBtnPressed(BTN_RIGHT)) {
                if (editField < 5) {
                    editField++;
                    drawEditView(selected);
                }
            }
            if (isBtnPressed(BTN_UP)) {
                if (editField == 0) {
                    alarms[selected].hour = (alarms[selected].hour + 1) % 24;
                } else if (editField == 1) {
                    alarms[selected].minute = (alarms[selected].minute + 1) % 60;
                } else if (editField == 3) {
                    alarms[selected].enabled = true;
                } else if (editField == 5) {
                    if (alarmVolume <= 95) alarmVolume += 5;
                    else alarmVolume = 100;
                }
                drawEditView(selected);
            }
            if (isBtnPressed(BTN_DOWN)) {
                if (editField == 0) {
                    alarms[selected].hour = (alarms[selected].hour + 23) % 24;
                } else if (editField == 1) {
                    alarms[selected].minute = (alarms[selected].minute + 59) % 60;
                } else if (editField == 3) {
                    alarms[selected].enabled = false;
                } else if (editField == 5) {
                    if (alarmVolume >= 5) alarmVolume -= 5;
                    else alarmVolume = 0;
                }
                drawEditView(selected);
            }
            if (isBtnPressed(BTN_OK)) {
                if (editField == 2) {
                    // 循环切换重复日（简单模式：按一次加一天）
                    if (alarms[selected].repeatDays == 0x7F) {
                        alarms[selected].repeatDays = 0;
                    } else {
                        alarms[selected].repeatDays++;
                    }
                    drawEditView(selected);
                } else if (editField == 3) {
                    alarms[selected].enabled = !alarms[selected].enabled;
                    drawEditView(selected);
                } else if (editField == 4) {
                    // 选择铃声MP3
                    String picked = pickMp3File();
                    if (picked.length() > 0) {
                        strncpy(alarms[selected].ringtone, picked.c_str(), 63);
                        alarms[selected].ringtone[63] = 0;
                    } else {
                        memset(alarms[selected].ringtone, 0, 64);
                    }
                    saveAlarms();
                    drawEditView(selected);
                } else {
                    // 时/分字段：OK确认，返回列表
                    editing = false;
                    saveAlarms();
                    drawAlarmList();
                }
            }
        }
        delay(50);
    }

    saveAlarms();
    inMenu = true;
    drawMenu();
}

// ========== 番茄钟 ==========
#define POMODORO_WORK_MIN 25
#define POMODORO_BREAK_MIN 5

void pomodoroTimer() {
    inMenu = false;
    loadFontSafe();

    unsigned long endTime = 0;
    bool running = false;
    bool isBreak = false;
    uint8_t sessions = 0;
    unsigned long remaining = POMODORO_WORK_MIN * 60 * 1000;

    auto drawPomodoroUI = [&]() {
        tft.fillScreen(TFT_BLACK);
        tft.setTextColor(TFT_CYAN, TFT_BLACK);
        tft.drawString("Pomodoro Timer", 10, 10);
        tft.drawLine(0, 28, tft.width(), 28, TFT_DARKGREY);

        // 状态
        tft.setTextColor(isBreak ? TFT_GREEN : TFT_ORANGE, TFT_BLACK);
        tft.drawString(isBreak ? "BREAK" : "WORK", 10, 38);

        // 倒计时 (大字)
        unsigned long sec = remaining / 1000;
        int min = sec / 60;
        int s = sec % 60;
        char timeStr[10];
        snprintf(timeStr, sizeof(timeStr), "%02d:%02d", min, s);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.setTextSize(3);
        tft.drawString(timeStr, 40, 70);
        tft.setTextSize(1);

        // 进度条
        unsigned long totalMs = isBreak ? (POMODORO_BREAK_MIN * 60 * 1000) : (POMODORO_WORK_MIN * 60 * 1000);
        unsigned long elapsed = totalMs - remaining;
        int barX = 10, barY = 120, barW = tft.width() - 20, barH = 12;
        tft.drawRect(barX, barY, barW, barH, TFT_DARKGREY);
        tft.fillRect(barX + 1, barY + 1, (int)((uint64_t)(barW - 2) * elapsed / totalMs), barH - 2,
                     isBreak ? TFT_GREEN : TFT_ORANGE);

        // 已完成轮数
        tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
        tft.drawString("Sessions: " + String(sessions), 10, 145);

        // 操作提示
        tft.drawString("OK: " + String(running ? "Pause" : "Start"), 10, 175);
        tft.drawString("UP: Reset", 10, 191);
        tft.drawString("LEFT: Exit", 10, 207);
    };

    drawPomodoroUI();

    while (true) {
        // 倒计时逻辑
        if (running) {
            unsigned long now = millis();
            if (now >= endTime) {
                remaining = 0;
                running = false;

                // LED 闪烁提示
                for (int i = 0; i < 6; i++) {
                    pixels.setPixelColor(0, i % 2 ? 0 : pixels.Color(0, 255, 0));
                    pixels.show();
                    delay(200);
                }
                pixels.setPixelColor(0, 0);
                pixels.show();

                if (!isBreak) {
                    sessions++;
                    isBreak = true;
                    remaining = POMODORO_BREAK_MIN * 60 * 1000;
                } else {
                    isBreak = false;
                    remaining = POMODORO_WORK_MIN * 60 * 1000;
                }
                drawPomodoroUI();
            } else {
                remaining = endTime - now;
                // 每秒刷新显示
                static unsigned long lastRefresh = 0;
                if (millis() - lastRefresh >= 1000) {
                    lastRefresh = millis();
                    drawPomodoroUI();
                }
            }
        }

        // 按键处理
        if (isBtnPressed(BTN_OK)) {
            if (!running) {
                running = true;
                endTime = millis() + remaining;
            } else {
                running = false;
            }
            drawPomodoroUI();
        }
        if (isBtnPressed(BTN_UP)) {
            running = false;
            isBreak = false;
            sessions = 0;
            remaining = POMODORO_WORK_MIN * 60 * 1000;
            drawPomodoroUI();
        }
        if (isBtnPressed(BTN_LEFT)) {
            break;
        }

        delay(50);
    }

    inMenu = true;
    drawMenu();
}

// FC (NES) 设置界面 - 音量调节，持久化到 flash
void fcSettings() {
    inMenu = false;
    loadFontSafe();

    Preferences prefs;
    prefs.begin("fc-config", true);
    uint8_t vol = prefs.getUChar("volume", 50);
    uint8_t stretch = prefs.getUChar("stretch", 0);  // 0=等比, 1=强制
    uint16_t nesW = prefs.getUShort("nesW", 240);
    uint16_t nesH = prefs.getUShort("nesH", 140);
    prefs.end();

    int field = 0;  // 0=音量, 1=拉伸, 2=宽度, 3=高度

    auto drawUI = [&]() {
        tft.fillScreen(TFT_BLACK);
        tft.setTextColor(TFT_CYAN, TFT_BLACK);
        tft.drawString("FC Settings", 10, 6);
        tft.drawLine(0, 24, 240, 24, TFT_DARKGREY);

        int y = 32;
        auto fieldRow = [&](int idx, const char* label, String val, bool isToggle = false) {
            bool sel = (idx == field);
            uint16_t bg = sel ? TFT_DARKCYAN : TFT_BLACK;
            uint16_t fg = sel ? TFT_YELLOW : TFT_WHITE;
            tft.fillRect(0, y, 240, 22, bg);
            tft.setTextColor(TFT_LIGHTGREY, bg);
            tft.drawString(label, 8, y + 3);
            tft.setTextColor(fg, bg);
            tft.drawString(val, 130, y + 3);
            y += 24;
        };

        // 音量进度条
        bool sel0 = (0 == field);
        tft.fillRect(0, y, 240, 22, sel0 ? TFT_DARKCYAN : TFT_BLACK);
        tft.setTextColor(TFT_LIGHTGREY, sel0 ? TFT_DARKCYAN : TFT_BLACK);
        tft.drawString("音量", 8, y + 3);
        tft.setTextColor(sel0 ? TFT_YELLOW : TFT_WHITE, sel0 ? TFT_DARKCYAN : TFT_BLACK);
        char volBuf[8]; snprintf(volBuf, sizeof(volBuf), "%d%%", vol);
        tft.drawString(volBuf, 130, y + 3);
        // 进度条
        int barX = 170, barW = 60;
        tft.drawRect(barX, y + 4, barW, 14, TFT_DARKGREY);
        tft.fillRect(barX + 1, y + 5, (barW - 2) * vol / 100, 12, TFT_GREEN);
        y += 24;

        fieldRow(1, "拉伸", stretch == 0 ? "等比" : "强制", true);
        char wBuf[8], hBuf[8];
        snprintf(wBuf, sizeof(wBuf), "%d", nesW);
        snprintf(hBuf, sizeof(hBuf), "%d", nesH);
        fieldRow(2, "宽度", String(wBuf));
        fieldRow(3, "高度", String(hBuf));

        // 底部提示
        tft.fillRect(0, 210, 240, 30, TFT_BLACK);
        tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
        tft.drawString("UD:选择 LR:调节 OK:输入", 16, 214);
        tft.drawString("LEFT:返回", 70, 230);
    };

    drawUI();

    while (true) {
        if (isBtnPressed(BTN_UP)) {
            if (field > 0) field--;
            else field = 3;
            drawUI();
        }
        if (isBtnPressed(BTN_DOWN)) {
            if (field < 3) field++;
            else field = 0;
            drawUI();
        }
        if (isBtnPressed(BTN_LEFT)) {
            if (field == 0) { if (vol >= 5) vol -= 5; drawUI(); }
            else if (field == 1) { stretch = 0; drawUI(); }
            else break;  // 返回
        }
        if (isBtnPressed(BTN_RIGHT)) {
            if (field == 0) { if (vol <= 95) vol += 5; drawUI(); }
            else if (field == 1) { stretch = 1; drawUI(); }
        }
        if (isBtnPressed(BTN_OK)) {
            if (field == 1) {
                stretch = (stretch == 0) ? 1 : 0;
                drawUI();
            } else if (field == 2) {
                unloadFontSafe();
                String s = inputTextDialog("宽度:", String(nesW));
                loadFontSafe();
                int v = s.toInt();
                if (v >= 120 && v <= 240) { nesW = v; }
                drawUI();
            } else if (field == 3) {
                unloadFontSafe();
                String s = inputTextDialog("高度:", String(nesH));
                loadFontSafe();
                int v = s.toInt();
                if (v >= 80 && v <= 240) { nesH = v; }
                drawUI();
            }
        }
        delay(30);
    }

    prefs.begin("fc-config", false);
    prefs.putUChar("volume", vol);
    prefs.putUChar("stretch", stretch);
    prefs.putUShort("nesW", nesW);
    prefs.putUShort("nesH", nesH);
    prefs.end();

    unloadFontSafe();
    inMenu = true;
    drawMenu();
}

// ========== FC游戏中心 - 全彩游戏浏览器 (v3) ==========
// 加载目录全部条目 (文件夹+ROM文件)
static void loadDirEntries(String dirPath, std::vector<FcGameEntry>& out) {
    out.clear();
    File dir = SD.open(dirPath);
    if (!dir || !dir.isDirectory()) return;
    File f = dir.openNextFile();
    while (f) {
        String name = f.name();
        if (name != "." && name != "..") {
            if (f.isDirectory()) {
                out.push_back({name, true, 0});
            } else {
                String lower = name;
                lower.toLowerCase();
                if (lower.endsWith(".nes") || lower.endsWith(".fc") || lower.endsWith(".fds")) {
                    out.push_back({name, false, (uint32_t)f.size()});
                }
            }
        }
        f = dir.openNextFile();
    }
    dir.close();
    std::sort(out.begin(), out.end(), [](const FcGameEntry& a, const FcGameEntry& b) {
        if (a.isDir != b.isDir) return a.isDir;
        return a.name < b.name;
    });
}

void fcGameBrowser() {
    inMenu = false;
    onDesktop = false;
    loadFontSafe();

    String basePath = "/fc游戏";
    if (!SD.exists(basePath)) {
        basePath = "/NES";
        if (!SD.exists(basePath)) SD.mkdir(basePath);
    }

    // 视图: 分类卡片 / 统一列表
    enum FcView { FC_CATS, FC_LIST };
    FcView view = FC_CATS;

    // 分类 (顶层文件夹)
    std::vector<FcGameEntry> cats;
    int catCursor = 0, catScroll = 0;

    // 统一列表 (文件夹+ROM混合)
    std::vector<FcGameEntry> entries;
    int cursor = 0, scroll = 0;
    int maxVis = 7;

    String currentPath = basePath;
    String listTitle = "";
    std::vector<String> pathStack;

    // 跑马灯
    int marqueeIdx = -1;
    int marqueeOff = 0;
    unsigned long marqueeTimer = 0;

    uint16_t catColors[] = {
        0xF800, 0x07E0, 0x001F, 0xFFE0, 0xF81F, 0x07FF,
        0xFD20, 0xA800, 0x0520, 0x4010, 0x8010, 0xC018
    };

    // ---- 加载顶层分类 ----
    auto loadCats = [&]() {
        cats.clear();
        catCursor = 0; catScroll = 0;
        loadDirEntries(basePath, cats);
    };

    // ---- 加载当前目录 ----
    auto loadCurrent = [&]() {
        loadDirEntries(currentPath, entries);
        cursor = 0; scroll = 0;
        marqueeIdx = -1; marqueeOff = 0;
    };

    // ---- 分类卡片参数 ----
    int catCardW = 108, catCardH = 64, catGapX = 12, catGapY = 10;
    int catStartX = (240 - catCardW * 2 - catGapX) / 2;
    int catStartY = 36;
    int catMaxRows = (212 - catStartY) / (catCardH + catGapY);

    // ---- 绘制单张分类卡片 (局部刷新用) ----
    auto drawCatItem = [&](int i, bool sel) {
        int col = i % 2, row = i / 2 - catScroll;
        int cx = catStartX + col * (catCardW + catGapX);
        int cy = catStartY + row * (catCardH + catGapY);
        if (cy + catCardH < 30 || cy > 222) return;

        uint16_t bg = sel ? 0x2945 : 0x2124;
        uint16_t border = sel ? 0xFFE0 : 0x4208;
        uint16_t iconC = catColors[i % 12];

        tft.fillRoundRect(cx, cy, catCardW, catCardH, 8, bg);
        tft.drawRoundRect(cx, cy, catCardW, catCardH, 8, border);

        int ix = cx + 14, iy = cy + 10;
        tft.fillRoundRect(ix, iy, 28, 18, 4, iconC);
        tft.fillCircle(ix + 8, iy + 9, 3, 0x0000);
        tft.fillCircle(ix + 20, iy + 9, 3, 0x0000);
        tft.fillRect(ix + 12, iy + 5, 4, 2, 0x0000);
        tft.fillCircle(ix + 20, iy + 5, 2, 0xFFFF);
        tft.fillCircle(ix + 25, iy + 9, 2, 0xFFE0);

        String disp = cats[i].name;
        if (disp.length() > 6) disp = disp.substring(0, 6);
        tft.setTextColor(sel ? 0xFFE0 : 0xFFFF, bg);
        int tw = tft.textWidth(disp);
        tft.drawString(disp, cx + (catCardW - tw) / 2, cy + 36);

        char countBuf[8];
        snprintf(countBuf, sizeof(countBuf), "[%d]", i + 1);
        tft.setTextColor(0x8410, bg);
        tft.drawString(countBuf, cx + 4, cy + 50);
    };

    // ---- 绘制分类卡片 (全屏绘制) ----
    auto drawCats = [&]() {
        tft.fillScreen(0x10A2);
        unloadFontSafe(); loadFontSafe();

        tft.fillRect(0, 0, 240, 28, 0x2124);
        tft.setTextColor(0x07FF, 0x2124);
        tft.drawString("FC Game Center", 8, 6);
        tft.setTextColor(0x8410, 0x2124);
        tft.drawString("L:Exit", 190, 6);
        tft.drawLine(0, 28, 240, 28, 0x4208);

        if (cats.empty()) {
            tft.setTextColor(0xFFFF, 0x10A2);
            tft.drawString("No ROM folders", 50, 100);
            tft.setTextColor(0xFFE0, 0x10A2);
            tft.drawString("Create folders in:", 50, 120);
            tft.drawString(basePath, 50, 140);
            tft.setTextColor(0x8410, 0x10A2);
            tft.drawString("and put .nes files inside", 50, 160);
            tft.fillRect(0, 224, 240, 16, 0x10A2);
            tft.setTextColor(0x8410, 0x10A2);
            tft.drawString("L:Exit", 88, 226);
            return;
        }

        for (int i = catScroll * 2; i < (int)cats.size() && i < (catScroll + catMaxRows) * 2; i++) {
            drawCatItem(i, i == catCursor);
        }

        tft.fillRect(0, 224, 240, 16, 0x10A2);
        tft.setTextColor(0x8410, 0x10A2);
        tft.drawString("UD:Nav OK:Open L:Exit", 28, 226);
    };

    // ---- 列表行参数 ----
    int listRowH = 28;

    // ---- 绘制单行列表项 (局部刷新用) ----
    auto drawListItem = [&](int idx, bool sel) {
        if (idx < 0 || idx >= (int)entries.size()) return;
        int visIdx = idx - scroll;
        if (visIdx < 0 || visIdx >= maxVis) return;
        int y = 32 + visIdx * listRowH;

        // 清除该行区域
        tft.fillRect(4, y, 232, listRowH - 2, sel ? 0x2945 : 0x10A2);
        if (sel) tft.drawRoundRect(4, y, 232, listRowH - 2, 4, 0xFFE0);

        // 图标
        if (entries[idx].isDir) {
            tft.fillRoundRect(10, y + 4, 18, 14, 2, 0xFFE0);
            tft.fillRect(10, y + 4, 8, 3, 0xFD20);
        } else {
            tft.fillRoundRect(10, y + 3, 18, 16, 2, 0x8410);
            tft.fillRect(12, y + 5, 14, 12, 0xA800);
            tft.fillRect(14, y + 7, 4, 4, 0x0000);
        }

        // 文件名
        String disp = entries[idx].name;
        if (!entries[idx].isDir) {
            if (disp.endsWith(".nes")) disp = disp.substring(0, disp.length() - 4);
            else if (disp.endsWith(".fc")) disp = disp.substring(0, disp.length() - 3);
            else if (disp.endsWith(".fds")) disp = disp.substring(0, disp.length() - 4);
        }

        tft.setTextColor(sel ? 0xFFE0 : 0xFFFF, sel ? 0x2945 : 0x10A2);
        int nameW = 140;
        int textW = tft.textWidth(disp);

        if (sel && textW > nameW) {
            int off = marqueeOff % (textW + 40);
            String scrollText = disp + "   " + disp;
            int charW = textW / disp.length();
            if (charW < 1) charW = 1;
            int startChar = off / charW;
            if (startChar < 0) startChar = 0;
            String visible = scrollText.substring(startChar);
            if (visible.length() > 20) visible = visible.substring(0, 20);
            tft.drawString(visible, 34, y + 6);
        } else {
            if (disp.length() > 16) disp = disp.substring(0, 16);
            tft.drawString(disp, 34, y + 6);
        }

        // 文件大小
        if (!entries[idx].isDir) {
            tft.setTextColor(0x8410, sel ? 0x2945 : 0x10A2);
            char sz[10];
            if (entries[idx].size >= 1024) snprintf(sz, sizeof(sz), "%dK", entries[idx].size / 1024);
            else snprintf(sz, sizeof(sz), "%dB", entries[idx].size);
            tft.drawString(sz, 195, y + 6);
        }
    };

    // ---- 绘制滚动条 ----
    auto drawScrollbar = [&]() {
        if ((int)entries.size() > maxVis) {
            int barY = 32 + 196 * scroll / (int)entries.size();
            int barH = 196 * maxVis / (int)entries.size();
            if (barH < 8) barH = 8;
            tft.fillRect(236, 32, 4, 196, 0x2124);
            tft.fillRect(236, barY, 4, barH, 0x8410);
        }
    };

    // ---- 绘制统一列表 (全屏绘制) ----
    auto drawList = [&]() {
        tft.fillScreen(0x10A2);
        unloadFontSafe(); loadFontSafe();

        tft.fillRect(0, 0, 240, 28, 0x2124);
        tft.setTextColor(0xFFE0, 0x2124);
        String title = listTitle;
        if (title.length() > 14) title = title.substring(0, 14);
        tft.drawString(title, 8, 6);
        tft.setTextColor(0xAFE0, 0x2124);
        char countBuf[12];
        snprintf(countBuf, sizeof(countBuf), "%d", (int)entries.size());
        tft.drawString(countBuf, 200, 6);
        tft.drawLine(0, 28, 240, 28, 0x4208);

        if (entries.empty()) {
            tft.setTextColor(0xFFFF, 0x10A2);
            tft.drawString("Empty folder", 60, 100);
            tft.setTextColor(0x8410, 0x10A2);
            tft.drawString("Add .nes files or", 45, 120);
            tft.drawString("subfolders here", 55, 140);
            tft.fillRect(0, 224, 240, 16, 0x10A2);
            tft.setTextColor(0x8410, 0x10A2);
            tft.drawString("L:Back", 88, 226);
            return;
        }

        int visCount = (int)entries.size() < maxVis ? (int)entries.size() : maxVis;
        for (int i = 0; i < visCount; i++) {
            drawListItem(scroll + i, (scroll + i) == cursor);
        }

        drawScrollbar();

        tft.fillRect(0, 224, 240, 16, 0x10A2);
        tft.setTextColor(0x8410, 0x10A2);
        tft.drawString("UD:Nav OK:Run L:Back", 28, 226);
    };

    // ---- 初始加载 ----
    loadCats();
    drawCats();

    while (true) {
        bool redraw = false;

        if (view == FC_CATS) {
            if (isBtnPressed(BTN_UP)) {
                if (catCursor > 0) {
                    int oldCursor = catCursor;
                    catCursor--;
                    if (catCursor < catScroll * 2) {
                        catScroll--;
                        drawCats();  // 翻页需要全屏重绘
                    } else {
                        drawCatItem(oldCursor, false);
                        drawCatItem(catCursor, true);
                    }
                }
            }
            if (isBtnPressed(BTN_DOWN)) {
                if (catCursor < (int)cats.size() - 1) {
                    int oldCursor = catCursor;
                    catCursor++;
                    int maxRows = (212 - 36) / 74;
                    if (catCursor >= (catScroll + maxRows) * 2) {
                        catScroll++;
                        drawCats();  // 翻页需要全屏重绘
                    } else {
                        drawCatItem(oldCursor, false);
                        drawCatItem(catCursor, true);
                    }
                }
            }
            if (isBtnPressed(BTN_LEFT)) {
                // 退出到桌面
                break;
            }
            if (isBtnPressed(BTN_OK)) {
                if (!cats.empty()) {
                    pathStack.clear();
                    currentPath = basePath + "/" + cats[catCursor].name;
                    listTitle = cats[catCursor].name;
                    loadCurrent();
                    view = FC_LIST;
                    drawList();
                    continue;
                }
            }
        } else {
            if (isBtnPressed(BTN_UP)) {
                if (cursor > 0) {
                    int oldCursor = cursor;
                    cursor--;
                    if (cursor < scroll) {
                        scroll = cursor;
                        marqueeOff = 0;
                        drawList();  // 翻页全屏重绘
                    } else {
                        marqueeOff = 0;
                        drawListItem(oldCursor, false);
                        drawListItem(cursor, true);
                        drawScrollbar();
                    }
                }
            }
            if (isBtnPressed(BTN_DOWN)) {
                if (cursor < (int)entries.size() - 1) {
                    int oldCursor = cursor;
                    cursor++;
                    if (cursor >= scroll + maxVis) {
                        scroll = cursor - maxVis + 1;
                        marqueeOff = 0;
                        drawList();  // 翻页全屏重绘
                    } else {
                        marqueeOff = 0;
                        drawListItem(oldCursor, false);
                        drawListItem(cursor, true);
                        drawScrollbar();
                    }
                }
            }
            if (isBtnPressed(BTN_LEFT)) {
                if (!pathStack.empty()) {
                    currentPath = pathStack.back();
                    pathStack.pop_back();
                    listTitle = currentPath.substring(currentPath.lastIndexOf('/') + 1);
                    loadCurrent();
                    drawList();
                    continue;
                } else {
                    // 返回分类卡片
                    view = FC_CATS;
                    currentPath = basePath;
                    loadCats();
                    drawCats();
                    continue;
                }
            }
            if (isBtnPressed(BTN_OK)) {
                if (entries.empty()) continue;
                if (entries[cursor].isDir) {
                    pathStack.push_back(currentPath);
                    currentPath += "/" + entries[cursor].name;
                    listTitle = entries[cursor].name;
                    loadCurrent();
                    drawList();
                    continue;
                } else {
                    String romPath = currentPath + "/" + entries[cursor].name;
                    tft.fillScreen(TFT_BLACK);
                    runNesEmulator(romPath);
                    loadFontSafe();
                    drawList();
                    continue;
                }
            }

            if (marqueeIdx == cursor) {
                if (millis() - marqueeTimer > 150) {
                    marqueeTimer = millis();
                    marqueeOff++;
                    drawListItem(cursor, true);  // 跑马灯只重绘选中行
                }
            } else {
                marqueeIdx = cursor;
                marqueeOff = 0;
                marqueeTimer = millis();
            }
        }

        // 音量变化等需要全屏重绘的情况
        if (redraw) {
            if (view == FC_CATS) drawCats();
            else drawList();
        }

        delay(15);
        yield();
    }

    unloadFontSafe();
    animateTransition();
    onDesktop = true;
    inMenu = false;
    drawDesktop();
    animateRestore();
}

// ========== 系统仪表盘 ==========
void systemDashboard() {
    onDesktop = false;
    inMenu = false;
    loadFontSafe();

    int page = 0;
    int totalPages = 5;

    auto drawPage = [&]() {
        tft.fillScreen(0x0000);
        tft.fillRect(0, 0, 240, 24, 0x2124);
        tft.setTextColor(0x07FF, 0x2124);

        char titleBuf[32];
        snprintf(titleBuf, sizeof(titleBuf), "系统信息 [%d/%d]", page + 1, totalPages);
        tft.drawString(titleBuf, 8, 4);

        // 页码点
        for (int p = 0; p < totalPages; p++) {
            tft.fillCircle(100 + p * 10, 14, 3, p == page ? 0xFFFF : 0x4208);
        }

        tft.drawLine(0, 24, 240, 24, 0x4208);

        int y = 32;
        auto label = [&](const char* key, const char* val, uint16_t kc = 0x8410, uint16_t vc = 0xFFFF) {
            tft.setTextColor(kc, 0x0000);
            tft.drawString(key, 8, y);
            tft.setTextColor(vc, 0x0000);
            tft.drawString(val, 100, y);
            y += 20;
        };

        auto labelInt = [&](const char* key, int val, const char* unit = "", uint16_t vc = 0xFFFF) {
            char buf[24];
            snprintf(buf, sizeof(buf), "%d%s", val, unit);
            label(key, buf, 0x8410, vc);
        };

        if (page == 0) {
            tft.setTextColor(0xFFE0, 0x0000);
            tft.drawString("设备信息", 88, y); y += 24;

            label("芯片", ESP.getChipModel());
            labelInt("CPU频率", ESP.getCpuFreqMHz(), "MHz");
            labelInt("闪存", ESP.getFlashChipSize() / 1024, "KB");
            int psramK = ESP.getPsramSize() / 1024;
            labelInt("PSRAM", psramK, "KB", psramK > 0 ? 0x07E0 : 0xF800);
            label("SDK", ESP.getSdkVersion());
            labelInt("核心数", ESP.getChipCores());
        }
        else if (page == 1) {
            tft.setTextColor(0xFFE0, 0x0000);
            tft.drawString("内存状态", 88, y); y += 24;

            int freeHeap = ESP.getFreeHeap() / 1024;
            int minHeap = ESP.getMinFreeHeap() / 1024;
            int maxAlloc = ESP.getMaxAllocHeap() / 1024;
            labelInt("空闲堆", freeHeap, "KB", freeHeap < 30 ? 0xF800 : 0x07E0);
            labelInt("最低空闲", minHeap, "KB");
            labelInt("最大分配", maxAlloc, "KB");

            y += 8;
            int freePs = ESP.getFreePsram() / 1024;
            int usedPs = (ESP.getPsramSize() - ESP.getFreePsram()) / 1024;
            labelInt("PSRAM空闲", freePs, "KB", 0x07E0);
            labelInt("PSRAM已用", usedPs, "KB");

            int barY = y + 4;
            int totalHeap = ESP.getHeapSize() / 1024;
            int usedHeap = totalHeap - freeHeap;
            tft.drawRect(8, barY, 224, 10, 0x8410);
            tft.fillRect(9, barY + 1, 222 * usedHeap / totalHeap, 8, 0xFD20);
            tft.setTextColor(0x8410, 0x0000);
            char heapBuf[24];
            snprintf(heapBuf, sizeof(heapBuf), "堆: %d/%dKB", freeHeap, totalHeap);
            tft.drawString(heapBuf, 50, barY + 12);
        }
        else if (page == 2) {
            tft.setTextColor(0xFFE0, 0x0000);
            tft.drawString("存储空间", 88, y); y += 24;

            uint64_t totalBytes = SD.totalBytes();
            uint64_t usedBytes = SD.usedBytes();
            uint64_t freeBytes = totalBytes - usedBytes;
            labelInt("SD总量", totalBytes / 1024, "KB");
            labelInt("SD已用", usedBytes / 1024, "KB", 0xFD20);
            labelInt("SD剩余", freeBytes / 1024, "KB", 0x07E0);

            int barY = y + 4;
            tft.drawRect(8, barY, 224, 10, 0x8410);
            if (totalBytes > 0) {
                tft.fillRect(9, barY + 1, (int)(222 * usedBytes / totalBytes), 8, 0x001F);
            }
            char sdBuf[24];
            snprintf(sdBuf, sizeof(sdBuf), "%.1f/%.1fMB", usedBytes / 1048576.0, totalBytes / 1048576.0);
            tft.setTextColor(0x8410, 0x0000);
            tft.drawString(sdBuf, 60, barY + 12);

            y += 36;
            labelInt("Flash大小", ESP.getFlashChipSize() / 1024, "KB");
            labelInt("Flash频率", ESP.getFlashChipSpeed() / 1000000, "MHz");
        }
        else if (page == 3) {
            tft.setTextColor(0xFFE0, 0x0000);
            tft.drawString("网络状态", 88, y); y += 24;

            if (WiFi.isConnected()) {
                label("SSID", WiFi.SSID().c_str(), 0x8410, 0x07FF);
                char ipBuf[20];
                snprintf(ipBuf, sizeof(ipBuf), "%s", WiFi.localIP().toString().c_str());
                label("IP", ipBuf);

                int rssi = WiFi.RSSI();
                char rssiBuf[16];
                snprintf(rssiBuf, sizeof(rssiBuf), "%d dBm", rssi);
                uint16_t rssiColor = rssi >= -50 ? 0x07E0 : rssi >= -70 ? 0xFFE0 : 0xF800;
                label("信号", rssiBuf, 0x8410, rssiColor);

                label("MAC", WiFi.macAddress().c_str(), 0x8410, 0x8410);
                label("网关", WiFi.gatewayIP().toString().c_str());
                label("DNS", WiFi.dnsIP().toString().c_str());
            } else {
                tft.setTextColor(0xF800, 0x0000);
                tft.drawString("WiFi未连接", 70, 80);
            }

            y += 8;
            tft.setTextColor(0x8410, 0x0000);
            char ntpBuf[24];
            snprintf(ntpBuf, sizeof(ntpBuf), "NTP: %s", ntpSynced ? "已同步" : "未同步");
            tft.drawString(ntpBuf, 8, y);
        }
        else if (page == 4) {
            tft.setTextColor(0xFFE0, 0x0000);
            tft.drawString("运行状态", 88, y); y += 24;

            unsigned long ms = millis();
            int sec = ms / 1000;
            int min = sec / 60;
            int hr = min / 60;
            char upBuf[20];
            snprintf(upBuf, sizeof(upBuf), "%d时%d分%d秒", hr, min % 60, sec % 60);
            label("运行时间", upBuf, 0x8410, 0x07FF);

            if (ntpSynced) {
                char timeBuf[20];
                snprintf(timeBuf, sizeof(timeBuf), "%04d-%02d-%02d %02d:%02d:%02d",
                    year(), month(), day(), hour(), minute(), second());
                label("当前时间", timeBuf);
            }

            labelInt("采样率", 24000, "Hz");
            Preferences p;
            p.begin("fc-config", true);
            uint8_t v = p.getUChar("volume", 50);
            p.end();
            labelInt("音量", v, "%");

            y += 4;
            tft.setTextColor(0x8410, 0x0000);
            const char* ledModes[] = {"关闭", "常亮", "闪烁", "呼吸"};
            if (ledMode < 4) {
                label("LED模式", ledModes[ledMode]);
            }

            y += 4;
            tft.setTextColor(0x8410, 0x0000);
            tft.drawString("按键状态", 8, y);
            tft.setTextColor(0xFFFF, 0x0000);
            char btnBuf[20];
            snprintf(btnBuf, sizeof(btnBuf), "U:%d O:%d D:%d L:%d R:%d",
                digitalRead(4), digitalRead(5), digitalRead(16), digitalRead(17), digitalRead(47));
            tft.drawString(btnBuf, 8, y + 16);
        }

        // 底部提示
        tft.fillRect(0, 226, 240, 14, 0x0000);
        tft.setTextColor(0x8410, 0x0000);
        tft.drawString("上下翻页  左键返回", 48, 228);
    };

    drawPage();

    while (true) {
        if (isBtnPressed(BTN_UP)) {
            if (page > 0) { page--; drawPage(); }
        }
        if (isBtnPressed(BTN_DOWN)) {
            if (page < totalPages - 1) { page++; drawPage(); }
        }
        if (isBtnPressed(BTN_LEFT) || isBtnPressed(BTN_OK)) {
            break;
        }
        delay(30);
        yield();
    }

    unloadFontSafe();
    animateTransition();
    onDesktop = true;
    inMenu = false;
    drawDesktop();
    animateRestore();
}

// 主WiFi设置屏幕
void wifiSettings() {
    inMenu = false;
    loadFontSafe();
    
    // 1. 扫描可用网络
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString("正在扫描WiFi...", 10, 10);
    
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);
    
    int n = WiFi.scanNetworks();
    std::vector<String> ssidList;
    for (int i = 0; i < n; ++i) {
        ssidList.push_back(WiFi.SSID(i));
    }
    
    if (ssidList.empty()) {
        tft.setTextColor(TFT_RED, TFT_BLACK);
        tft.drawString("未发现任何网络", 10, 30);
        delay(2000);
        inMenu = true;
        drawMenu();
        return;
    }
    
    // 2. 显示列表并选择
    int listIdx = 0;
    int listTop = 0;
    int maxItems = (tft.height() - 70) / 18;
    
    auto drawWifiList = [&]() {
        tft.fillScreen(TFT_BLACK);
        tft.setTextColor(TFT_GREEN, TFT_BLACK);
        tft.drawString("选择WiFi网络:", 10, 10);
        tft.drawLine(0, 25, tft.width(), 25, TFT_DARKGREY);
        
        for (int i = 0; i < maxItems; i++) {
            int idx = listTop + i;
            if (idx >= ssidList.size()) break;
            int yPos = 30 + i * 18;
            if (idx == listIdx) {
                tft.fillRect(0, yPos, tft.width(), 18, TFT_DARKCYAN);
                tft.setTextColor(TFT_WHITE, TFT_DARKCYAN);
            } else {
                tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
            }
            tft.drawString(ssidList[idx], 10, yPos);
        }
    };
    
    drawWifiList();
    
    // 3. 列表交互
    bool ssidSelected = false;
    while (!ssidSelected) {
        if (isBtnPressed(BTN_UP)) {
            if (listIdx > 0) { listIdx--; }
            if (listIdx < listTop) { listTop = listIdx; }
            drawWifiList();
        }
        if (isBtnPressed(BTN_DOWN)) {
            if (listIdx < ssidList.size() - 1) { listIdx++; }
            if (listIdx >= listTop + maxItems) { listTop = listIdx - maxItems + 1; }
            drawWifiList();
        }
        if (isBtnPressed(BTN_OK)) {
            ssidSelected = true;
        }
        if (isBtnPressed(BTN_LEFT)) {
            // 按左键返回
            inMenu = true;
            drawMenu();
            return;
        }
    }
    
    // 4. 调用输入法输入密码
    String selectedSSID = ssidList[listIdx];
    String password = inputTextDialog("密码:", "");
    
    if (password.length() == 0) {
        tft.fillScreen(TFT_BLACK);
        tft.setTextColor(TFT_RED, TFT_BLACK);
        tft.drawString("密码为空，已取消", 10, 30);
        delay(1500);
        inMenu = true;
        drawMenu();
        return;
    }
    
    // 5. 尝试连接
    connectToWiFi(selectedSSID, password);
    
    inMenu = true;
    drawMenu();
}
// ----------------------------------------------------------------------------
// [009] GUI 子系统：系统状态监视器 (System Monitor)
// ----------------------------------------------------------------------------
void displaySystemMonitor() {
    CyberLogger::info("GUI_SYS", "Opening System Monitor...");
    loadFontSafe();
    tft.fillScreen(TFT_BLACK);
    
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.drawString("== 核心系统资源监视器 ==", 5, 10);
    tft.drawLine(0, 30, tft.width(), 30, TFT_DARKGREY);
    
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    int y = 40;
    int spacing = 20;
    
    // CPU Info
    tft.drawString("CPU 核心频率: " + String(ESP.getCpuFreqMHz()) + " MHz", 5, y); y += spacing;
    tft.drawString("芯片内核温度: " + String(temperatureRead(), 1) + " C", 5, y); y += spacing;
    
    // RAM Info
    tft.drawString("系统可用 RAM: " + String(ESP.getFreeHeap() / 1024) + " KB", 5, y); y += spacing;
    tft.drawString("最大连续 RAM: " + String(ESP.getMaxAllocHeap() / 1024) + " KB", 5, y); y += spacing;
    
    // Storage Info
    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    uint64_t totalBytes = SD.totalBytes() / (1024 * 1024);
    uint64_t usedBytes = SD.usedBytes() / (1024 * 1024);
    tft.drawString("SD卡物理容量: " + String((unsigned long)cardSize) + " MB", 5, y); y += spacing;
    tft.drawString("文件系统总计: " + String((unsigned long)totalBytes) + " MB", 5, y); y += spacing;
    tft.drawString("文件系统已用: " + String((unsigned long)usedBytes) + " MB", 5, y); y += spacing;
    // PSRAM Info
uint32_t psramTotal = ESP.getPsramSize() / 1024;              // KB
uint32_t psramFree  = ESP.getFreePsram() / 1024;
uint32_t psramMax   = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM) / 1024;
tft.drawString("PSRAM 总容量: " + String(psramTotal) + " KB", 5, y); y += spacing;
tft.drawString("PSRAM 可用: " + String(psramFree) + " KB", 5, y); y += spacing;
tft.drawString("PSRAM 最大连续块: " + String(psramMax) + " KB", 5, y); y += spacing;
    // Network Info
   // tft.drawString("无线网络 MAC: " + WiFi.softAPmacAddress(), 5, y); y += spacing;
    
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.drawString("", 5, tft.height() - 25);
    
    while (!checkStopAction()) {
       // server.handleClient();
        yield();
    }
}
// ----------------------------------------------------------------------------
// [010] 媒体引擎 I: 高级文本与 LRC 歌词解析


// ============================================================================
// 终极极简内存版文本阅读器
// 特性：每行最多15个汉字，自动换行；只记录当前顶部行号，翻页时动态扫描
// 内存占用：忽略不计，绝不OOM
// ============================================================================

#include <vector>   // 仅用于存储当前页每行的字符串（极少量）

// ---------------------------------------------------------------------------
// [010] 媒体引擎 I: 高级文本与 LRC 歌词解析 — PSRAM 终极高速版
// ---------------------------------------------------------------------------

// 全局变量（不再依赖文件对象，改用内存指针 + 偏移表）
File     g_txtFile;                 // 保留，便于统一接口（此处不再使用文件操作）
uint8_t* g_memFile = nullptr;       // 整个文本文件的内存镜像（PSRAM）
int      g_totalRenderLines = 0;    // 总渲染行数
int      g_currentTopLine = 0;      // 当前屏幕第一行的渲染行号
int      g_lineHeight = 18;         // 字体行高
int      g_linesPerPage = 0;        // 每屏可显示行数

// 行偏移表（PSRAM）
uint32_t* g_lineOffsets = nullptr;  // 每行的起始字节偏移
uint16_t* g_lineLengths = nullptr;  // 每行的字节长度

// 前置声明
void buildLineTable();              // 扫描 g_memFile 构建偏移表
void renderCurrentPage(bool hasBookmark, int savedLine);
void drawStatusBar(int topLine, bool hasBookmark, int savedLine);

// ------------------------------------------------------------
// 构建行偏移表（按原规则：每行最多15个汉字，自动折行）
// ------------------------------------------------------------
void buildLineTable() {
    if (!g_memFile) return;
    // 先扫描一遍获取总行数（用于分配数组）
    int lines = 0;
    size_t pos = 0;
    int col = 0;
    while (g_memFile[pos] != 0) {
        char c = g_memFile[pos];
        if (c == '\n' || c == '\r') {
            if (col > 0) lines++;
            col = 0;
            if (c == '\r' && g_memFile[pos+1] == '\n') pos++; // 跳过 \r\n
        } else {
            // 处理 UTF-8 多字节
            int bytes = 1;
            if ((c & 0xE0) == 0xC0) bytes = 2;
            else if ((c & 0xF0) == 0xE0) bytes = 3;
            else if ((c & 0xF8) == 0xF0) bytes = 4;
            pos += (bytes - 1);
            col++;
            if (col >= 15) {
                lines++;
                col = 0;
            }
        }
        pos++;
    }
    if (col > 0) lines++;
    g_totalRenderLines = lines;

    // 分配行偏移表（PSRAM）
    g_lineOffsets = (uint32_t*)heap_caps_malloc(lines * sizeof(uint32_t), MALLOC_CAP_SPIRAM);
    g_lineLengths = (uint16_t*)heap_caps_malloc(lines * sizeof(uint16_t), MALLOC_CAP_SPIRAM);
    if (!g_lineOffsets || !g_lineLengths) {
        CyberLogger::error("TXT_ENG", "PSRAM allocation for line table failed!");
        if (g_lineOffsets) { free(g_lineOffsets); g_lineOffsets = nullptr; }
        if (g_lineLengths) { free(g_lineLengths); g_lineLengths = nullptr; }
        return;
    }

    // 第二遍扫描，填充偏移和长度
    int lineIdx = 0;
    pos = 0;
    col = 0;
    size_t lineStart = 0;
    while (g_memFile[pos] != 0) {
        char c = g_memFile[pos];
        if (c == '\n' || c == '\r') {
            if (col > 0) {
                g_lineOffsets[lineIdx] = lineStart;
                g_lineLengths[lineIdx] = pos - lineStart;
                lineIdx++;
            }
            col = 0;
            if (c == '\r' && g_memFile[pos+1] == '\n') pos++;
            pos++;
            lineStart = pos;
            continue;
        }

        int bytes = 1;
        if ((c & 0xE0) == 0xC0) bytes = 2;
        else if ((c & 0xF0) == 0xE0) bytes = 3;
        else if ((c & 0xF8) == 0xF0) bytes = 4;
        pos += bytes;

        col++;
        if (col >= 15) {
            g_lineOffsets[lineIdx] = lineStart;
            g_lineLengths[lineIdx] = pos - lineStart;
            lineIdx++;
            col = 0;
            lineStart = pos;
        }
    }
    if (col > 0) {
        g_lineOffsets[lineIdx] = lineStart;
        g_lineLengths[lineIdx] = pos - lineStart;
    }
}

// ------------------------------------------------------------
// 渲染当前页（使用内存偏移直接获取行文本）
// ------------------------------------------------------------
void renderCurrentPage(bool hasBookmark, int savedLine) {
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);

    for (int i = 0; i < g_linesPerPage; i++) {
        int lineIdx = g_currentTopLine + i;
        if (lineIdx >= g_totalRenderLines) break;
        uint32_t offset = g_lineOffsets[lineIdx];
        uint16_t length = g_lineLengths[lineIdx];
        // === 内存泄漏修复：使用unique_ptr自动管理内存 ===
        std::unique_ptr<char[]> lineBuf(new char[length + 1]);
        if (!lineBuf) continue;
        memcpy(lineBuf.get(), &g_memFile[offset], length);
        lineBuf[length] = '\0';
        tft.setCursor(0, i * g_lineHeight);
        tft.print(lineBuf.get());
        // 超出作用域自动释放，无需显式free()
    }
    drawStatusBar(g_currentTopLine, hasBookmark, savedLine);
}

// ------------------------------------------------------------
// 绘制底部状态栏（同原版）
// ------------------------------------------------------------
void drawStatusBar(int topLine, bool hasBookmark, int savedLine) {
    tft.fillRect(0, tft.height() - 20, tft.width(), 20, TFT_DARKGREY);
    tft.setTextColor(TFT_WHITE, TFT_DARKGREY);
    String status = "行 " + String(topLine + 1) + "/" + String(g_totalRenderLines);
    if (hasBookmark) status += "  [书签:" + String(savedLine + 1) + "]";
    tft.drawString(status, 5, tft.height() - 18);
    tft.drawString("OK:退出  长按上:删书签  长按下:恢复", 80, tft.height() - 18);
}
// ------------------------------------------------------------
// 书签辅助函数（必须放在 displayTxtFile 前面）
// ------------------------------------------------------------
String getBookmarkPath(String txtPath) {
    if (txtPath.endsWith("/")) return "";
    int lastSlash = txtPath.lastIndexOf('/');
    int lastDot   = txtPath.lastIndexOf('.');
    String base   = (lastDot > lastSlash) ? txtPath.substring(0, lastDot) : txtPath;
    return base + ".bmk";
}

int readBookmark(String txtPath) {
    String bmkPath = getBookmarkPath(txtPath);
    if (!SD.exists(bmkPath)) return -1;
    File f = SD.open(bmkPath, FILE_READ);
    if (!f) return -1;
    String content = f.readString();
    f.close();
    return content.toInt();
}

void writeBookmark(String txtPath, int line) {
    String bmkPath = getBookmarkPath(txtPath);
    File f = SD.open(bmkPath, FILE_WRITE);
    if (f) {
        f.print(line);
        f.close();
    }
}

void deleteBookmark(String txtPath) {
    String bmkPath = getBookmarkPath(txtPath);
    if (SD.exists(bmkPath)) SD.remove(bmkPath);
}
// ------------------------------------------------------------
// 主入口：完全 PSRAM 化，UI 不变，翻页瞬间完成
// ------------------------------------------------------------
void displayTxtFile(File &file) {
    CyberLogger::info("TXT_ENG", "Starting High‑Speed PSRAM Text Reader");
    loadFontSafe();

    // === 内存泄漏修复：清理上次调用的残留数据 ===
    if (g_memFile) {
        heap_caps_free(g_memFile);
        g_memFile = nullptr;
    }
    if (g_lineOffsets) {
        heap_caps_free(g_lineOffsets);
        g_lineOffsets = nullptr;
    }
    if (g_lineLengths) {
        heap_caps_free(g_lineLengths);
        g_lineLengths = nullptr;
    }

    // 读取整个文件到 PSRAM
    size_t fileSize = file.size();
    g_memFile = (uint8_t*)heap_caps_malloc(fileSize + 1, MALLOC_CAP_SPIRAM);
    if (!g_memFile) {
        CyberLogger::error("TXT_ENG", "PSRAM allocation for file failed, OOM");
        file.close();
        return;
    }
    file.read(g_memFile, fileSize);
    g_memFile[fileSize] = 0; // 终止符
    file.close();

    // 构建行偏移表
    buildLineTable();
    if (!g_lineOffsets || !g_lineLengths) {
        // 内存不足，清理并退出
        free(g_memFile);
        return;
    }

    g_lineHeight = 18;
    g_linesPerPage = tft.height() / g_lineHeight;

    // 保存文件路径用于书签
    String currentPath = file.name();
    if (!currentPath.startsWith("/")) currentPath = "/" + currentPath;

    // 读取书签
    int savedLine = readBookmark(currentPath);
    bool hasBookmark = (savedLine >= 0 && savedLine < g_totalRenderLines);

    // 书签恢复提示（界面同原版）
    if (hasBookmark) {
        tft.fillScreen(TFT_BLACK);
        tft.setTextColor(TFT_YELLOW, TFT_BLACK);
        tft.drawString("检测到阅读进度", 10, 30);
        tft.drawString("上次读到第 " + String(savedLine + 1) + " 行", 10, 50);
        tft.drawString("长按 [下键] 恢复进度", 10, 70);
        tft.drawString("按其他键继续阅读", 10, 90);
        unsigned long hintStart = millis();
        bool choiceMade = false;
        while (!choiceMade) {
            if (digitalRead(BTN_DOWN) == LOW) {
                unsigned long press = millis();
                while (digitalRead(BTN_DOWN) == LOW) { yield(); }
                if (millis() - press > 500) {
                    g_currentTopLine = savedLine;
                    choiceMade = true;
                } else {
                    choiceMade = true;
                }
            }
            if (digitalRead(BTN_OK) == LOW || digitalRead(BTN_UP) == LOW) {
                delay(150);
                choiceMade = true;
            }
            if (millis() - hintStart > 5000) choiceMade = true;
            yield();
        }
        tft.fillScreen(TFT_BLACK);
    } else {
        g_currentTopLine = 0;
    }

    renderCurrentPage(hasBookmark, savedLine);

    // 主循环（按键逻辑不变）
    while (true) {
        bool action = false;
        int step = 0;

        if (digitalRead(BTN_OK) == LOW) {
            unsigned long press = millis();
            while (digitalRead(BTN_OK) == LOW) { yield(); }
            if (millis() - press < 500) {
                writeBookmark(currentPath, g_currentTopLine);
                CyberLogger::info("TXT_ENG", "Bookmark saved at line " + String(g_currentTopLine + 1));
                break;  // 退出
            }
        }

        if (digitalRead(BTN_UP) == LOW) {
            unsigned long press = millis();
            while (digitalRead(BTN_UP) == LOW) { yield(); }
            if (millis() - press > 500) {
                deleteBookmark(currentPath);
                hasBookmark = false;
                renderCurrentPage(hasBookmark, savedLine);
                CyberLogger::info("TXT_ENG", "Bookmark deleted.");
            } else {
                step = -5;
                action = true;
            }
        }

        if (digitalRead(BTN_DOWN) == LOW) {
            unsigned long press = millis();
            while (digitalRead(BTN_DOWN) == LOW) { yield(); }
            if (millis() - press < 500) {
                step = 5;
                action = true;
            }
        }

        if (action) {
            int newTop = g_currentTopLine + step;
            if (step > 0) {
                int maxTop = g_totalRenderLines - g_linesPerPage;
                if (maxTop < 0) maxTop = 0;
                if (newTop > maxTop) newTop = maxTop;
            } else {
                if (newTop < 0) newTop = 0;
            }
            if (newTop != g_currentTopLine) {
                g_currentTopLine = newTop;
                renderCurrentPage(hasBookmark, savedLine);
            }
        }
        yield();
    }

    // 清理 PSRAM
    free(g_memFile);
    free(g_lineOffsets);
    free(g_lineLengths);
    g_memFile = nullptr;
    g_lineOffsets = nullptr;
    g_lineLengths = nullptr;
    tft.fillScreen(TFT_BLACK);
}
// ----------------------------------------------------------------------------
// [011] 媒体引擎 II: LRC 卡拉OK歌词滚动解析
// ----------------------------------------------------------------------------
// 结构体：存储单行歌词与时间戳
struct LrcLine {
    uint32_t timestamp; // 毫秒
    String text;
};

void displayLrcFile(String path) {
    CyberLogger::info("LRC_ENG", "Starting Karaoke LRC Parser for: " + path);
    loadFontSafe();
    tft.fillScreen(TFT_BLACK);
    
    File file = SD.open(path, FILE_READ);
    if (!file) return;

    std::vector<LrcLine> lrcData;
    
    // 解析文件
    while (file.available()) {
        String line = file.readStringUntil('\n');
        line.trim();
        if (line.length() < 10) continue;
        
        // 简单正则提取 [mm:ss.xx] 
        if (line.charAt(0) == '[' && line.charAt(3) == ':' && line.charAt(6) == '.') {
            String minStr = line.substring(1, 3);
            String secStr = line.substring(4, 6);
            String msStr = line.substring(7, 9);
            
            uint32_t totalMs = (minStr.toInt() * 60000) + (secStr.toInt() * 1000) + (msStr.toInt() * 10);
            int bracketEnd = line.indexOf(']');
            if (bracketEnd != -1) {
                String lyricText = line.substring(bracketEnd + 1);
                lyricText.trim();
                
                LrcLine ll;
                ll.timestamp = totalMs;
                ll.text = lyricText;
                lrcData.push_back(ll);
            }
        }
    }
    file.close();
    
    if (lrcData.empty()) {
        tft.setTextColor(TFT_RED, TFT_BLACK);
        tft.drawString("无法识别的 LRC 时间戳格式", 10, tft.height() / 2);
        delay(2000);
        return;
    }
    
    CyberLogger::info("LRC_ENG", "Successfully parsed " + String(lrcData.size()) + " lyric lines.");
    
    unsigned long startTime = millis();
    int currentLine = 0;
    int lineHeight = tft.fontHeight();
    
    // 模拟播放时间轴
    while (!checkStopAction()) {
        unsigned long elapsed = millis() - startTime;
        
        // 查找当前应该高亮哪一行
        int newTargetLine = currentLine;
        for (int i = 0; i < lrcData.size(); i++) {
            if (elapsed >= lrcData[i].timestamp) {
                newTargetLine = i;
            } else {
                break;
            }
        }
        
        // 如果歌词进入下一句，则重新绘制屏幕
        if (newTargetLine != currentLine || currentLine == 0) {
            currentLine = newTargetLine;
            tft.fillScreen(TFT_BLACK);
            
            int centerY = tft.height() / 2 - (lineHeight / 2);
            
            // 绘制前两行 (暗色)
            tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
            for (int i = 2; i >= 1; i--) {
                if (currentLine - i >= 0) {
                    tft.drawString(lrcData[currentLine - i].text, 5, centerY - (i * lineHeight));
                }
            }
            
            // 绘制当前行 (高亮黄色)
            tft.setTextColor(TFT_YELLOW, TFT_BLACK);
            tft.drawString(lrcData[currentLine].text, 5, centerY);
            
            // 绘制后四行 (暗色)
            tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
            for (int i = 1; i <= 4; i++) {
                if (currentLine + i < lrcData.size()) {
                    tft.drawString(lrcData[currentLine + i].text, 5, centerY + (i * lineHeight));
                }
            }
        }
        
        //server.handleClient();
        delay(50); // 降低刷新率节省 CPU
        
        // 如果播放完毕
        if (currentLine >= lrcData.size() - 1 && elapsed > lrcData.back().timestamp + 5000) {
            break;
        }
    }
}

// ----------------------------------------------------------------------------
// [012] 媒体引擎 III: CSV 数据表格网格渲染器
// ----------------------------------------------------------------------------
void displayCsvFile(String path) {
    CyberLogger::info("CSV_ENG", "Rendering grid data view for: " + path);
    loadFontSafe();
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    
    File file = SD.open(path, FILE_READ);
    if (!file) return;

    int y = 5;
    int row = 0;
    
    tft.drawString("📊 数据表格视图 (首屏速览):", 5, y);
    y += 25;
    tft.drawLine(0, y, tft.width(), y, TFT_DARKGREY);
    y += 5;

    while (file.available() && y < tft.height() - 20) {
        String line = file.readStringUntil('\n');
        line.trim();
        if (line.length() == 0) continue;

        int colX = 5;
        int startIdx = 0;
        
        while (startIdx < line.length() && colX < tft.width()) {
            int commaIdx = line.indexOf(',', startIdx);
            String cell = "";
            if (commaIdx == -1) {
                cell = line.substring(startIdx);
                startIdx = line.length();
            } else {
                cell = line.substring(startIdx, commaIdx);
                startIdx = commaIdx + 1;
            }
            
            // 限制内容长度并画网格框
            if (cell.length() > 6) cell = cell.substring(0, 5) + ".";
            tft.drawString(cell, colX, y);
            tft.drawRect(colX - 2, y - 2, tft.textWidth(cell) + 8, 20, TFT_DARKGREY);
            colX += tft.textWidth(cell) + 15;
        }
        y += 22;
        row++;
        if (row > 15) break; 
    }
    file.close();
    
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.drawString(">> 按 OK 键退出数据面板 <<", 5, tft.height() - 20);

    while (!checkStopAction()) {
       // //server.handleClient(); 
        yield();
    }
}

// ----------------------------------------------------------------------------
// [013] 媒体引擎 IV: 终极底层十六进制嗅探器 (Deep Hex Inspector)
// ----------------------------------------------------------------------------
void displayHexInspector(String path) {
    CyberLogger::warn("HEX_ENG", "Unsupported binary format detected, engaging Deep Hex Inspector!");
    unloadFontSafe(); // 必须使用等宽英文字体保证对齐
    tft.setTextSize(1);
    tft.fillScreen(TFT_BLACK);
    
    File file = SD.open(path, FILE_READ);
    if (!file) return;

    uint32_t offset = 0;
    int bytesPerRow = 8; 
    int maxRows = tft.height() / 10 - 2; 
    
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.setCursor(0, 0);
    tft.println("== DEEP HEX INSPECTOR ==");
    tft.println(path);
    tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);

    // 逐行解析二进制
    for (int r = 0; r < maxRows; r++) {
        if (!file.available()) break;
        
        char offStr[10];
        sprintf(offStr, "%08X:", offset);
        tft.print(offStr);
        
        uint8_t buffer[8];
        int bytesRead = file.read(buffer, bytesPerRow);
        
        for (int i = 0; i < bytesPerRow; i++) {
            if (i < bytesRead) {
                char hexStr[4];
                sprintf(hexStr, " %02X", buffer[i]);
                tft.setTextColor(TFT_CYAN, TFT_BLACK);
                tft.print(hexStr);
            } else {
                tft.print("   ");
            }
        }
        
        tft.print(" | ");
        
        tft.setTextColor(TFT_YELLOW, TFT_BLACK);
        for (int i = 0; i < bytesRead; i++) {
            char c = buffer[i];
            if (c >= 32 && c <= 126) {
                tft.print(c);
            } else {
                tft.print('.'); 
            }
        }
        tft.println();
        offset += bytesRead;
    }
    
    file.close();
    
    tft.fillRect(0, tft.height() - 15, tft.width(), 15, TFT_DARKGREY);
    tft.setTextColor(TFT_WHITE, TFT_DARKGREY);
    tft.setCursor(5, tft.height() - 13);
    tft.print("RAW BINARY MODE - PRESS OK TO EXIT");

    while (!checkStopAction()) {
        ////server.handleClient(); 
        yield();
    }
}

// ----------------------------------------------------------------------------
// [014] 媒体引擎 V: 静态图像解码群组 (JPG, PNG, GIF)
// ----------------------------------------------------------------------------
bool jpgOutput(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
    if (y >= tft.height()) return 0; 
    tft.startWrite(); 
    tft.pushImage(x, y, w, h, bitmap); 
    tft.endWrite();
    return 1;
}

void displayJpgFile(String path) {
    CyberLogger::info("IMG_ENG", "Hardware decoding JPG: " + path);
    unloadFontSafe(); 
    TJpgDec.setCallback(jpgOutput);
    TJpgDec.drawSdJpg(0, 0, path.c_str());
    
    unsigned long startWait = millis();
    while (millis() - startWait < 4000) {
        if (checkStopAction()) return;
       // //server.handleClient(); 
        yield();
    }
}

int PNGDraw(PNGDRAW *pDraw) {
    uint16_t lineBuffer[pDraw->iWidth];
    png->getLineAsRGB565(pDraw, lineBuffer, PNG_RGB565_LITTLE_ENDIAN, 0xffffffff);
    tft.pushImage(pngXpos, pngYpos + pDraw->y, pDraw->iWidth, 1, lineBuffer);
    return 1;
}

void displayPngFile(String path) {
    CyberLogger::info("IMG_ENG", "Software decoding PNG into RAM: " + path);
    unloadFontSafe(); 
    
    File pngFile = SD.open(path, FILE_READ);
    if (!pngFile) return;
    
    size_t fileSize = pngFile.size();
    uint8_t* pngData = (uint8_t*)heap_caps_malloc(fileSize, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!pngData) { 
        CyberLogger::error("IMG_ENG", "PNG File too large, OOM generated.");
        pngFile.close(); 
        return; 
    }
    
    pngFile.read(pngData, fileSize);
    pngFile.close();

    // === RAM优化：png改为延迟初始化的static指针 ===
    if (!png) png = new PNG();
    if (png->openRAM(pngData, fileSize, PNGDraw) == PNG_SUCCESS) {
        tft.startWrite();
        png->decode(NULL, 0);
        png->close();
        tft.endWrite();
    }
    free(pngData);
    
    unsigned long startWait = millis();
    while (millis() - startWait < 4000) {
        if (checkStopAction()) return;
        ////server.handleClient(); 
        yield();
    }
}

void GIFDraw(GIFDRAW *pDraw) {
    uint8_t *s = pDraw->pPixels; 
    uint16_t *usPalette = pDraw->pPalette;
    int y = pDraw->iY + pDraw->y;
    int iWidth = pDraw->iWidth > tft.width() ? tft.width() : pDraw->iWidth;
    
    tft.startWrite();
    for (int x = 0; x < iWidth; x++) {
        tft.drawPixel(x + pDraw->iX, y, usPalette[*s++]);
    }
    tft.endWrite();
}

void displayGifFile(String path) {
    CyberLogger::info("IMG_ENG", "Starting GIF infinite loop engine...");
    unloadFontSafe(); 
    
    File gifFile = SD.open(path, FILE_READ);
    if (!gifFile) return;
    
    size_t fileSize = gifFile.size();
    uint8_t* gifData = (uint8_t*)heap_caps_malloc(fileSize, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!gifData) {
        CyberLogger::error("IMG_ENG", "GIF File too large, OOM.");
        return;
    }
    
    gifFile.read(gifData, fileSize);
    gifFile.close();

    // === RAM优化：gif改为函数内局部变量 ===
    AnimatedGIF gif;
    gif.begin(LITTLE_ENDIAN_PIXELS);
    if (gif.open(gifData, fileSize, GIFDraw)) {
        while (gif.playFrame(true, NULL)) { 
            if (checkStopAction()) break; 
            yield(); 
            //server.handleClient(); 
        }
        gif.close();
    }
    free(gifData);
}

// ----------------------------------------------------------------------------
// [015] 媒体引擎 VI: 纯手工生肉底层图像解码 (BMP, TGA, PCX)
// ----------------------------------------------------------------------------
// 读取 16/32 位小端数据工具函数
uint16_t read16(File &f) { uint16_t res; ((uint8_t *)&res)[0] = f.read(); ((uint8_t *)&res)[1] = f.read(); return res; }
uint32_t read32(File &f) { uint32_t res; ((uint8_t *)&res)[0] = f.read(); ((uint8_t *)&res)[1] = f.read(); ((uint8_t *)&res)[2] = f.read(); ((uint8_t *)&res)[3] = f.read(); return res; }

// 手写 BMP 解析器
void displayBmpFile(String path) {
    CyberLogger::info("RAW_IMG", "Handcrafted BMP Rasterizer triggered.");
    unloadFontSafe();
    tft.fillScreen(TFT_BLACK);
    
    File bmpFile = SD.open(path, FILE_READ);
    if (!bmpFile) return;

    if (read16(bmpFile) != 0x4D42) { 
        CyberLogger::error("RAW_IMG", "Invalid BMP magic number.");
        bmpFile.close(); return;
    }
    
    read32(bmpFile); read32(bmpFile); 
    uint32_t imageOffset = read32(bmpFile); 

    read32(bmpFile); 
    int32_t bmpWidth = read32(bmpFile);
    int32_t bmpHeight = read32(bmpFile);
    if (read16(bmpFile) != 1) { bmpFile.close(); return; } 
    uint16_t bmpDepth = read16(bmpFile);
    
    if (bmpDepth != 24) {
        CyberLogger::error("RAW_IMG", "Only 24-bit uncompressed BMP is supported by handwritten decoder.");
        bmpFile.close(); return;
    }
    read32(bmpFile); 

    bool flip = true;
    if (bmpHeight < 0) { bmpHeight = -bmpHeight; flip = false; }
    
    uint32_t rowSize = (bmpWidth * 3 + 3) & ~3;
    uint8_t sdbuffer[3 * 20]; 
    uint16_t lcdbuffer[20];
    
    bmpFile.seek(imageOffset);
    tft.startWrite();
    
    for (int row = 0; row < bmpHeight; row++) {
        if (checkStopAction()) break;
        int y = flip ? (bmpHeight - 1 - row) : row;
        if (y >= tft.height()) {
            bmpFile.seek(bmpFile.position() + rowSize);
            continue;
        }

        int col = 0;
        while (col < bmpWidth) {
            int bytesToRead = min((int)20, (int)(bmpWidth - col)) * 3;
            bmpFile.read(sdbuffer, bytesToRead);
            
            for (int i = 0; i < bytesToRead / 3; i++) {
                lcdbuffer[i] = tft.color565(sdbuffer[i * 3 + 2], sdbuffer[i * 3 + 1], sdbuffer[i * 3]); 
            }
            
            tft.pushImage(col, y, bytesToRead / 3, 1, lcdbuffer);
            col += bytesToRead / 3;
         //   server.handleClient();
        }
        if (rowSize - (bmpWidth * 3) > 0) bmpFile.seek(bmpFile.position() + (rowSize - (bmpWidth * 3)));
    }
    tft.endWrite();
    bmpFile.close();

    unsigned long startWait = millis();
    while (millis() - startWait < 4000) { if (checkStopAction()) return; //server.handleClient(); 
    yield(); }
}

// ----------------------------------------------------------------------------
// [016] 媒体引擎 VII: 纯手工 TGA (Targa) 图像解码器
// ----------------------------------------------------------------------------
void displayTgaFile(String path) {
    CyberLogger::info("RAW_IMG", "Handcrafted TGA Rasterizer triggered: " + path);
    unloadFontSafe();
    tft.fillScreen(TFT_BLACK);
    
    File tgaFile = SD.open(path, FILE_READ);
    if (!tgaFile) return;

    // 读取 TGA 18字节头部
    uint8_t idLen = tgaFile.read();
    uint8_t colorMapType = tgaFile.read();
    uint8_t imageType = tgaFile.read();
    
    if (imageType != 2) { 
        CyberLogger::error("RAW_IMG", "Only Uncompressed True-Color TGA (Type 2) is supported.");
        tgaFile.close(); return;
    }
    
    tgaFile.seek(12); // 跳到宽高信息
    uint16_t tgaWidth = read16(tgaFile);
    uint16_t tgaHeight = read16(tgaFile);
    uint8_t tgaDepth = tgaFile.read();
    uint8_t imageDesc = tgaFile.read();
    
    if (tgaDepth != 24 && tgaDepth != 32) {
        CyberLogger::error("RAW_IMG", "Only 24/32-bit TGA supported.");
        tgaFile.close(); return;
    }

    tgaFile.seek(18 + idLen); // 跳过 ID 区，来到像素区
    bool flipY = !(imageDesc & 0x20); // TGA 默认也是从下往上，除非 Bit 5 为 1
    
    uint8_t bytesPerPixel = tgaDepth / 8;
    uint8_t pixelBuf[4];
    
    tft.startWrite();
    for (int y = 0; y < tgaHeight; y++) {
        if (checkStopAction()) break;
        int drawY = flipY ? (tgaHeight - 1 - y) : y;
        
        if (drawY >= tft.height()) {
            tgaFile.seek(tgaFile.position() + (tgaWidth * bytesPerPixel));
            continue;
        }
        for (int x = 0; x < tgaWidth; x++) {
            if (x < tft.width()) {
                tgaFile.read(pixelBuf, bytesPerPixel);
                // TGA 像素顺序为 B G R (A)
                uint16_t color = tft.color565(pixelBuf[2], pixelBuf[1], pixelBuf[0]);
                tft.drawPixel(x, drawY, color);
            } else {
                tgaFile.seek(tgaFile.position() + bytesPerPixel);
            }
        }
        if (y % 10 == 0) { yield(); //server.handleClient();
         }
    }
    tft.endWrite();
    tgaFile.close();

    unsigned long startWait = millis();
    while (millis() - startWait < 4000) { if (checkStopAction()) return; //server.handleClient(); 
    yield();
     }
}

// ----------------------------------------------------------------------------
// [017] 媒体引擎 VIII: 纯手工 PCX (ZSoft Paintbrush) 图像解码器
// ----------------------------------------------------------------------------
void displayPcxFile(String path) {
    CyberLogger::info("RAW_IMG", "ZSoft PCX Hardware Decoder initialized.");
    unloadFontSafe();
    tft.fillScreen(TFT_BLACK);
    
    File pcx = SD.open(path, FILE_READ);
    if (!pcx) return;

    if (pcx.read() != 10) { // PCX 魔数始终为 10
        CyberLogger::error("RAW_IMG", "Invalid PCX Magic Number.");
        pcx.close(); return;
    }
    
    uint8_t version = pcx.read();
    uint8_t encoding = pcx.read();
    uint8_t bpp = pcx.read(); // 每像素位数
    
    uint16_t xmin = read16(pcx);
    uint16_t ymin = read16(pcx);
    uint16_t xmax = read16(pcx);
    uint16_t ymax = read16(pcx);
    
    uint16_t width = xmax - xmin + 1;
    uint16_t height = ymax - ymin + 1;
    
    pcx.seek(65); // 跳过大部分头信息
    uint8_t planes = pcx.read();
    uint16_t bytesPerLine = read16(pcx);
    
    pcx.seek(128); // 像素数据开始
    
    if (bpp == 8 && planes == 3) {
        // 24-bit PCX 逐行 RLE 解码
        tft.startWrite();
        for (int y = 0; y < height; y++) {
            if (checkStopAction()) break;
            if (y >= tft.height()) break;
            
            uint8_t scanline[3][bytesPerLine];
            for (int p = 0; p < 3; p++) {
                int offset = 0;
                while (offset < bytesPerLine) {
                    uint8_t data = pcx.read();
                    if ((data & 0xC0) == 0xC0) {
                        uint8_t runLen = data & 0x3F;
                        uint8_t val = pcx.read();
                        for (int i = 0; i < runLen && offset < bytesPerLine; i++) scanline[p][offset++] = val;
                    } else {
                        scanline[p][offset++] = data;
                    }
                }
            }
            // 写入屏幕 (RGB)
            for (int x = 0; x < width && x < tft.width(); x++) {
                tft.drawPixel(x, y, tft.color565(scanline[0][x], scanline[1][x], scanline[2][x]));
            }
            if (y % 10 == 0) { yield(); //server.handleClient(); 
            }
        }
        tft.endWrite();
    } else {
        CyberLogger::error("RAW_IMG", "Only 24-bit TrueColor PCX is supported currently.");
    }
    
    pcx.close();
    unsigned long startWait = millis();
    while (millis() - startWait < 4000) { if (checkStopAction()) return; //server.handleClient();
     yield(); }
}

// ----------------------------------------------------------------------------
// [018] 音频引擎 III: WAV 格式解析与实时示波器 (WAV Visualizer)
// ----------------------------------------------------------------------------
void displayWavVisualizer(String path) {
    CyberLogger::info("WAV_ENG", "Starting WAV header parser and Oscilloscope: " + path);
    tft.fillScreen(TFT_BLACK);
    loadFontSafe();
    
    File wav = SD.open(path, FILE_READ);
    if (!wav) return;

    // RIFF Header check
    char riff[5] = {0};
    wav.read((uint8_t*)riff, 4);
    if (String(riff) != "RIFF") {
        CyberLogger::error("WAV_ENG", "Invalid RIFF magic header.");
        tft.setTextColor(TFT_RED, TFT_BLACK);
        tft.drawString("非法的 WAV 封装格式！", 10, 30);
        delay(2000); wav.close(); return;
    }

    wav.seek(22);
    uint16_t channels = read16(wav);
    uint32_t sampleRate = read32(wav);
    wav.seek(34);
    uint16_t bitDepth = read16(wav);

    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.drawString("⚡ WAV 频段分析与物理示波器", 5, 10);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString("基频采样: " + String(sampleRate) + " Hz", 5, 40);
    tft.drawString("硬件声道: " + String(channels) + " CH", 5, 60);
    tft.drawString("量化位深: " + String(bitDepth) + " Bit", 5, 80);
    
    tft.drawRect(4, 109, tft.width() - 8, 102, TFT_CYAN);
    
    wav.seek(44); // 粗略定位到 PCM 数据区
    int x = 6;
    int baseline = 160;

    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.drawString(">> 保持按住 OK 键紧急退出 <<", 5, tft.height() - 25);

    // 开始绘制假装很牛逼的实时波形
    while (wav.available() && !checkStopAction()) {
        if (x > tft.width() - 6) {
            x = 6;
            tft.fillRect(5, 110, tft.width() - 10, 100, TFT_BLACK); // 扫描线扫完清屏
        }
        
        uint8_t byteVal = wav.read();
        int yOffset = map(byteVal, 0, 255, -45, 45); // 将 8bit PCM 映射到波形幅度
        tft.drawLine(x, baseline, x, baseline + yOffset, TFT_GREEN);
        
        wav.seek(wav.position() + 128); // 跳帧读取，模拟时间流逝
        x += 2;
        
        //server.handleClient();
        delay(5); // 调节示波器扫描速度
    }
    
    wav.close();
}

// ----------------------------------------------------------------------------
// [019] 终极外挂模块：CHIP-8 掌机虚拟机 (Game Emulator) 【增加左右键映射】
// ----------------------------------------------------------------------------
// 几百行硬核 CPU 模拟指令，将 ESP32 直接变成上世纪的复古游戏机！
void runChip8Emulator(String romPath) {
    CyberLogger::info("EMU_SYS", "Booting CHIP-8 Hardware Emulator Mode!");
    unloadFontSafe();
    tft.fillScreen(TFT_BLACK);
    tft.setTextSize(1);
    
    File rom = SD.open(romPath, FILE_READ);
    if (!rom) {
        CyberLogger::warn("EMU_SYS", "ROM File not found!");
        return;
    }
    
    // 动态内存分配 (Heap)
    uint8_t* memory = (uint8_t*)malloc(4096);
    uint8_t* gfx = (uint8_t*)malloc(64 * 32);
    uint16_t* stack = (uint16_t*)malloc(16 * sizeof(uint16_t));

    if (!memory || !gfx || !stack) {
        CyberLogger::error("EMU_SYS", "FATAL: Out of Heap Memory!");
        if (memory) free(memory);
        if (gfx) free(gfx);
        if (stack) free(stack);
        rom.close();
        return;
    }

    memset(memory, 0, 4096);
    memset(gfx, 0, 64 * 32);
    memset(stack, 0, 16 * sizeof(uint16_t));

    uint8_t V[16] = {0};
    uint16_t I = 0;
    uint16_t pc = 0x200;
    uint8_t sp = 0;
    uint8_t delay_timer = 0;
    uint8_t sound_timer = 0;
    bool drawFlag = false;
    
    // 系统字库
    uint8_t fontset[80] = { 
      0xF0, 0x90, 0x90, 0x90, 0xF0, 0x20, 0x60, 0x20, 0x20, 0x70,
      0xF0, 0x10, 0xF0, 0x80, 0xF0, 0xF0, 0x10, 0xF0, 0x10, 0xF0,
      0x90, 0x90, 0xF0, 0x10, 0x10, 0xF0, 0x80, 0xF0, 0x10, 0xF0,
      0xF0, 0x80, 0xF0, 0x90, 0xF0, 0xF0, 0x10, 0x20, 0x40, 0x40,
      0xF0, 0x90, 0xF0, 0x90, 0xF0, 0xF0, 0x90, 0xF0, 0x10, 0xF0,
      0xF0, 0x90, 0xF0, 0x90, 0x90, 0xE0, 0x90, 0xE0, 0x90, 0xE0,
      0xF0, 0x80, 0x80, 0x80, 0xF0, 0xE0, 0x90, 0x90, 0x90, 0xE0,
      0xF0, 0x80, 0xF0, 0x80, 0xF0, 0xF0, 0x80, 0xF0, 0x80, 0x80
    };
    for(int i = 0; i < 80; ++i) memory[i] = fontset[i];
    
    int bufferSize = rom.read(&memory[0x200], 3584);
    rom.close();
    
    unsigned long lastTick = millis();
    tft.drawRect(0, 0, tft.width(), tft.height(), TFT_BLUE);
    
    int scaleX = tft.width() / 64;
    int scaleY = tft.height() / 32;
    int scale = min(scaleX, scaleY);
    int offsetX = (tft.width() - (64 * scale)) / 2;
    int offsetY = (tft.height() - (32 * scale)) / 2;
    
    // 🚨 长按退出逻辑状态变量
    unsigned long okPressTime = 0;
    bool okIsPressed = false;
    bool emuRunning = true;

    // 核心循环：架空 checkStopAction，用我们自己的 emuRunning 控制！
    while (emuRunning) {
        //server.handleClient();
        yield();
        
        // ================= 🚨 物理按键劫持系统 🚨 =================
        if (digitalRead(BTN_OK) == LOW) {
            if (!okIsPressed) {
                okIsPressed = true;
                okPressTime = millis();
            } else if (millis() - okPressTime > 800) { 
                // 长按超过 800 毫秒，直接击穿循环，强制退出模拟器！
                CyberLogger::info("EMU_SYS", "Long press OK detected! Exiting game...");
                break; 
            }
        } else {
            okIsPressed = false;
        }
        // =========================================================

        uint16_t opcode = memory[pc] << 8 | memory[pc + 1];
        
        switch(opcode & 0xF000) {
            case 0x0000:
                switch(opcode & 0x000F) {
                    case 0x0000: memset(gfx, 0, 64*32); drawFlag = true; pc += 2; break;
                    case 0x000E: if(sp > 0) { --sp; pc = stack[sp]; } pc += 2; break;
                    default: pc += 2; break;
                } break;
            case 0x1000: pc = opcode & 0x0FFF; break;
            case 0x2000: stack[sp] = pc; ++sp; pc = opcode & 0x0FFF; break;
            case 0x3000: if(V[(opcode & 0x0F00) >> 8] == (opcode & 0x00FF)) pc += 4; else pc += 2; break;
            case 0x4000: if(V[(opcode & 0x0F00) >> 8] != (opcode & 0x00FF)) pc += 4; else pc += 2; break;
            case 0x5000: if(V[(opcode & 0x0F00) >> 8] == V[(opcode & 0x00F0) >> 4]) pc += 4; else pc += 2; break;
            case 0x6000: V[(opcode & 0x0F00) >> 8] = (opcode & 0x00FF); pc += 2; break;
            case 0x7000: V[(opcode & 0x0F00) >> 8] += (opcode & 0x00FF); pc += 2; break;
            case 0x8000:
                switch(opcode & 0x000F) {
                    case 0x0000: V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x00F0) >> 4]; pc += 2; break;
                    case 0x0001: V[(opcode & 0x0F00) >> 8] |= V[(opcode & 0x00F0) >> 4]; pc += 2; break;
                    case 0x0002: V[(opcode & 0x0F00) >> 8] &= V[(opcode & 0x00F0) >> 4]; pc += 2; break;
                    case 0x0003: V[(opcode & 0x0F00) >> 8] ^= V[(opcode & 0x00F0) >> 4]; pc += 2; break;
                    case 0x0004: 
                        V[(opcode & 0x0F00) >> 8] += V[(opcode & 0x00F0) >> 4]; 
                        V[0xF] = (V[(opcode & 0x00F0) >> 4] > (0xFF - V[(opcode & 0x0F00) >> 8])) ? 1 : 0;
                        pc += 2; break;
                    case 0x0005: 
                        V[0xF] = (V[(opcode & 0x0F00) >> 8] > V[(opcode & 0x00F0) >> 4]) ? 1 : 0;
                        V[(opcode & 0x0F00) >> 8] -= V[(opcode & 0x00F0) >> 4];
                        pc += 2; break;
                    case 0x0006: 
                        V[0xF] = V[(opcode & 0x0F00) >> 8] & 0x1;
                        V[(opcode & 0x0F00) >> 8] >>= 1; pc += 2; break;
                    case 0x0007: 
                        V[0xF] = (V[(opcode & 0x00F0) >> 4] > V[(opcode & 0x0F00) >> 8]) ? 1 : 0;
                        V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x00F0) >> 4] - V[(opcode & 0x0F00) >> 8];
                        pc += 2; break;
                    case 0x000E: 
                        V[0xF] = V[(opcode & 0x0F00) >> 8] >> 7;
                        V[(opcode & 0x0F00) >> 8] <<= 1; pc += 2; break;
                    default: pc += 2; break;
                } break;
            case 0x9000: if(V[(opcode & 0x0F00) >> 8] != V[(opcode & 0x00F0) >> 4]) pc += 4; else pc += 2; break;
            case 0xA000: I = opcode & 0x0FFF; pc += 2; break;
            case 0xB000: pc = (opcode & 0x0FFF) + V[0]; break;
            case 0xC000: V[(opcode & 0x0F00) >> 8] = (rand() % 0xFF) & (opcode & 0x00FF); pc += 2; break;
            case 0xD000: 
            {
                uint16_t x = V[(opcode & 0x0F00) >> 8];
                uint16_t y = V[(opcode & 0x00F0) >> 4];
                uint16_t height = opcode & 0x000F;
                uint16_t pixel;
                V[0xF] = 0;
                for (int yline = 0; yline < height; yline++) {
                    pixel = memory[I + yline];
                    for(int xline = 0; xline < 8; xline++) {
                        if((pixel & (0x80 >> xline)) != 0) {
                            if(gfx[(x + xline + ((y + yline) * 64))] == 1) V[0xF] = 1;
                            gfx[x + xline + ((y + yline) * 64)] ^= 1;
                        }
                    }
                }
                drawFlag = true; pc += 2;
            } break;
            case 0xE000:
                switch(opcode & 0x00FF) {
                    case 0x009E: 
                        // 扩展按键映射：上下左右 + OK
                        if (V[(opcode & 0x0F00) >> 8] == 4 && digitalRead(BTN_UP) == LOW) pc += 4;
                        else if (V[(opcode & 0x0F00) >> 8] == 6 && digitalRead(BTN_DOWN) == LOW) pc += 4;
                        else if (V[(opcode & 0x0F00) >> 8] == 5 && digitalRead(BTN_OK) == LOW) pc += 4;
                        else if (V[(opcode & 0x0F00) >> 8] == 7 && digitalRead(BTN_LEFT) == LOW) pc += 4;
                        else if (V[(opcode & 0x0F00) >> 8] == 8 && digitalRead(BTN_RIGHT) == LOW) pc += 4;
                        else pc += 2; break;
                    case 0x00A1: 
                        if (V[(opcode & 0x0F00) >> 8] == 4 && digitalRead(BTN_UP) != LOW) pc += 4;
                        else if (V[(opcode & 0x0F00) >> 8] == 6 && digitalRead(BTN_DOWN) != LOW) pc += 4;
                        else if (V[(opcode & 0x0F00) >> 8] == 5 && digitalRead(BTN_OK) != LOW) pc += 4;
                        else if (V[(opcode & 0x0F00) >> 8] == 7 && digitalRead(BTN_LEFT) != LOW) pc += 4;
                        else if (V[(opcode & 0x0F00) >> 8] == 8 && digitalRead(BTN_RIGHT) != LOW) pc += 4;
                        else pc += 2; break;
                    default: pc += 2; break;
                } break;
            case 0xF000: 
                switch(opcode & 0x00FF) {
                    case 0x0007: V[(opcode & 0x0F00) >> 8] = delay_timer; pc += 2; break;
                    case 0x000A: 
                        // 扩展按键等待
                        if (digitalRead(BTN_UP) == LOW) { V[(opcode & 0x0F00) >> 8] = 4; pc += 2; }
                        else if (digitalRead(BTN_DOWN) == LOW) { V[(opcode & 0x0F00) >> 8] = 6; pc += 2; }
                        else if (digitalRead(BTN_OK) == LOW) { V[(opcode & 0x0F00) >> 8] = 5; pc += 2; }
                        else if (digitalRead(BTN_LEFT) == LOW) { V[(opcode & 0x0F00) >> 8] = 7; pc += 2; }
                        else if (digitalRead(BTN_RIGHT) == LOW) { V[(opcode & 0x0F00) >> 8] = 8; pc += 2; }
                        break;
                    case 0x0015: delay_timer = V[(opcode & 0x0F00) >> 8]; pc += 2; break;
                    case 0x0018: sound_timer = V[(opcode & 0x0F00) >> 8]; pc += 2; break;
                    case 0x001E: I += V[(opcode & 0x0F00) >> 8]; pc += 2; break;
                    case 0x0029: I = V[(opcode & 0x0F00) >> 8] * 0x5; pc += 2; break;
                    case 0x0033: 
                        memory[I] = V[(opcode & 0x0F00) >> 8] / 100;
                        memory[I + 1] = (V[(opcode & 0x0F00) >> 8] / 10) % 10;
                        memory[I + 2] = (V[(opcode & 0x0F00) >> 8] % 100) % 10;
                        pc += 2; break;
                    case 0x0055: 
                        for (int i = 0; i <= ((opcode & 0x0F00) >> 8); ++i) memory[I + i] = V[i];
                        pc += 2; break;
                    case 0x0065: 
                        for (int i = 0; i <= ((opcode & 0x0F00) >> 8); ++i) V[i] = memory[I + i];
                        pc += 2; break;
                    default: pc += 2; break;
                } break;
        }

        if (millis() - lastTick > 16) {
            lastTick = millis();
            if (delay_timer > 0) --delay_timer;
            // 蜂鸣器已移除，忽略 sound_timer 处理
        }

        if (drawFlag) {
            for (int y = 0; y < 32; ++y) {
                for (int x = 0; x < 64; ++x) {
                    if (gfx[(y * 64) + x] == 1) {
                        tft.fillRect(offsetX + (x * scale), offsetY + (y * scale), scale, scale, TFT_GREEN);
                    } else {
                        tft.fillRect(offsetX + (x * scale), offsetY + (y * scale), scale, scale, TFT_BLACK);
                    }
                }
            }
            drawFlag = false;
        }
        delay(2); 
    }
    
    // 回收内存，清理战场
    free(memory);
    free(gfx);
    free(stack);
    
    // 强制延时防抖，防止退出的瞬间误触发菜单里的 OK 键！
    delay(500); 
    
    CyberLogger::info("EMU_SYS", "VM Shutdown. Heap memory reclaimed!");
}
// ----------------------------------------------------------------------------
// [020] 操作系统最高指令调度总线
// ============================================================================
// 补回的模块：史诗级 MIDI 解析与硬件合成 (蜂鸣器部分已屏蔽)
// ============================================================================
void midiCallback(midi_event *pev) {
    // 蜂鸣器已移除，回调仅做空处理或日志
    if (pev->data[0] >= 0x90 && pev->data[0] <= 0x9F) { 
        // 原 tone 调用已删除
    } 
    else if (pev->data[0] >= 0x80 && pev->data[0] <= 0x8F) { 
        // 原 noTone 调用已删除
    }
}

void playMidiFile(String path) {
    CyberLogger::info("MIDI_ENG", "总线切换，接管音频合成 (蜂鸣器已禁用): " + path);
    tft.fillScreen(TFT_BLACK);
    loadFontSafe(); 
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.drawString("🎵 MIDI 交响乐引擎启动...", 10, 30);
    tft.drawString(path.substring(path.lastIndexOf('/') + 1), 10, 50);
    
    SD.end(); delay(100);
    if (!sdFat.begin(SdSpiConfig(SD_CS, SHARED_SPI, SD_SCK_MHZ(10), &sdSPI))) {
        CyberLogger::error("MIDI_ENG", "SdFat 挂载失败！");
        SD.begin(SD_CS, sdSPI, 18000000); return;
    }

    SMF.begin(&sdFat); 
    SMF.setMidiHandler(midiCallback);
    
    if (SMF.load(path.c_str()) == 0) {
        while (!SMF.isEOF()) {
            if (checkStopAction()) break; 
            SMF.getNextEvent();     
            //server.handleClient(); 
            yield();
        }
        SMF.close();
    }
    
    delay(100);
    SD.begin(SD_CS, sdSPI, 18000000); 
}

// ============================================================================
// 补回的模块：巨型视频序列解码器 (ANIM Sequences) 绝对秒退版
// ============================================================================
void playAnimSequence(String animPath) {
    CyberLogger::info("ANIM_ENG", "启动视频序列引擎: " + animPath);
    File animFile = SD.open(animPath, FILE_READ);
    if (!animFile) return;

    unloadFontSafe(); 
    tft.fillScreen(TFT_BLACK);
    TJpgDec.setCallback(jpgOutput);

    while (animFile.available()) {
        // 🚨 外层循环斩立决
        if (checkStopAction()) { animFile.close();loadFontSafe();  return; } 
        
        String line = animFile.readStringUntil('\n'); line.trim();
        if (line.length() == 0) continue;

        String folderName = line; int fps = 10;
        int sepIdx = line.lastIndexOf('@');
        if (sepIdx > 0) { 
            folderName = line.substring(0, sepIdx); 
            fps = line.substring(sepIdx + 1).toInt(); 
            if (fps <= 0) fps = 10; 
        }
        if (!folderName.startsWith("/")) folderName = "/movies/" + folderName;
        
        int frameDelay = 1000 / fps;
        int i = 1; bool needFallback = false; 

        // 阶段一：极速盲读
        for (;; i++) { 
            // 🚨 内层循环斩立决
            if (checkStopAction()) { animFile.close();loadFontSafe();  return; } 
            
            String filePath = folderName; if (!filePath.endsWith("/")) filePath += "/";
            filePath += "frame_" + String(i) + ".jpg";
            
            if (!SD.exists(filePath)) { 
                if (i > 50) break; // 正常播完这个分块，进入下一段，用 break
                else { needFallback = true; break; } 
            }
            
            unsigned long start = millis();
            TJpgDec.drawSdJpg(0, 0, filePath.c_str());
            unsigned long cost = millis() - start;
            
            if (cost < frameDelay) {
                // 🚨 延时侦测斩立决
                if (smartDelay(frameDelay - cost)) { animFile.close();loadFontSafe();  return; } 
            }
          //  server.handleClient();
        }

        // 阶段二：安全扫描兜底
        if (needFallback && !checkStopAction()) {
            std::vector<String> fallbacks = listJpgFiles(folderName);
            for (int j = i - 1; j < fallbacks.size(); j++) {
                // 🚨 兜底循环斩立决
                if (checkStopAction()) { animFile.close();loadFontSafe();  return; } 
                
                unsigned long start = millis();
                String filePath = folderName; if (!filePath.endsWith("/")) filePath += "/"; filePath += fallbacks[j];
                TJpgDec.drawSdJpg(0, 0, filePath.c_str());
                unsigned long cost = millis() - start;
                
                if (cost < frameDelay) {
                    // 🚨 延时侦测斩立决
                    if (smartDelay(frameDelay - cost)) { animFile.close();loadFontSafe();  return; } 
                }
               // server.handleClient();
            }
        }
    }
    animFile.close();
    loadFontSafe();
}

void playAviMjpeg(String path) {
    File aviFile = SD.open(path, FILE_READ);
    if (!aviFile) return;

    unloadFontSafe();
    tft.fillScreen(TFT_BLACK);
    TJpgDec.setCallback(jpgOutput);

    // ---- 解析 RIFF/AVI 头 ----
    uint8_t riffBuf[12];
    if (aviFile.read(riffBuf, 12) != 12) { aviFile.close(); loadFontSafe(); return; }

    uint32_t fps = 10;
    bool foundMovi = false;

    while (aviFile.available()) {
        if (checkStopAction()) { aviFile.close(); loadFontSafe(); return; }

        uint8_t hdr[8];
        if (aviFile.read(hdr, 8) != 8) break;
        uint32_t sz = hdr[4] | (hdr[5]<<8) | (hdr[6]<<16) | (hdr[7]<<24);
        char id[5] = {0};
        memcpy(id, hdr, 4);

        if (strcmp(id, "LIST") == 0) {
            char listType[5] = {0};
            if (aviFile.read((uint8_t*)listType, 4) != 4) break;
            if (strcmp(listType, "movi") == 0) {
                foundMovi = true;
                break;
            }
        } else if (strcmp(id, "avih") == 0) {
            uint8_t avih[56];
            aviFile.read(avih, 56);
            uint32_t usecPerFrame, totalFrames, w, h;
            memcpy(&usecPerFrame, avih, 4);
            memcpy(&totalFrames, avih+8, 4);
            memcpy(&w, avih+16, 4);
            memcpy(&h, avih+20, 4);
            if (usecPerFrame > 0) fps = 1000000 / usecPerFrame;
            fps = constrain(fps, 1, 30);
            Serial.printf("[AVI] %dx%d @ %ufps, %u frames\n", w, h, fps, totalFrames);
        } else {
            aviFile.seek(aviFile.position() + ((sz + 1) & ~1));
        }
    }

    if (!foundMovi) { aviFile.close(); loadFontSafe(); return; }

    // ---- 播放 movi 数据帧 ----
    int frameDelay = 1000 / fps;

    while (aviFile.available()) {
        if (checkStopAction()) break;

        uint8_t hdr[8];
        if (aviFile.read(hdr, 8) != 8) break;
        uint32_t sz = hdr[4] | (hdr[5]<<8) | (hdr[6]<<16) | (hdr[7]<<24);
        char chunkId[5] = {0};
        memcpy(chunkId, hdr, 4);

        if (strcmp(chunkId, "00dc") == 0) {
            uint8_t* buf = (uint8_t*)heap_caps_malloc(sz, MALLOC_CAP_SPIRAM);
            if (buf) {
                aviFile.read(buf, sz);
                unsigned long t0 = millis();
                TJpgDec.drawJpg(0, 0, buf, sz);
                unsigned long cost = millis() - t0;
                heap_caps_free(buf);
                if (cost < (unsigned long)frameDelay) {
                    if (smartDelay(frameDelay - cost)) break;
                }
            } else {
                aviFile.seek(aviFile.position() + ((sz + 1) & ~1));
            }
        } else {
            aviFile.seek(aviFile.position() + ((sz + 1) & ~1));
        }
    }

    aviFile.close();
    loadFontSafe();
}

void openAndDisplayFile(String path) {
    path.trim();
    CyberLogger::info("SYS_CORE", "Executing File Association Bus for: " + path);
    
    // 拦截内建系统软件
    if (path == "/SYS_MONITOR") {
        inMenu = false;
        displaySystemMonitor();
        hasTask = false; inMenu = true;  
        loadFontSafe(); drawMenu(); 
        return;
    }
    
    if (!SD.exists(path)) {
        CyberLogger::error("SYS_CORE", "File Pointer points to nowhere.");
        loadFontSafe(); 
        tft.fillScreen(TFT_BLACK);
        tft.setTextColor(TFT_RED, TFT_BLACK);
        tft.drawString("严重错误: 目标块丢失", 10, 50);
        delay(2000);
        return;
    }

    String filename = path.substring(path.lastIndexOf('/') + 1);
    String ext;
    if (!getFileExtension(filename, ext)) {
        displayHexInspector(path); // 连后缀都没有，直接十六进制强上
        hasTask = false; inMenu = true; loadFontSafe(); drawMenu(); return;
    }

    inMenu = false; 
    loadFontSafe(); 
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.drawString("分配底层解码矩阵: ." + ext, 10, 10);
    CyberLogger::mem("Before Decoder Switch");

    // ======= 神级大一统全格式分配路由 =======
    if (ext == "anim") playAnimSequence(path);
    else if (ext == "avi") playAviMjpeg(path);
    else if (ext == "txt" || ext == "ini" || ext == "log" || ext == "json") { File txtFile = SD.open(path, FILE_READ); if (txtFile) displayTxtFile(txtFile); }
    else if (ext == "lrc") displayLrcFile(path);
    else if (ext == "csv") displayCsvFile(path);
    else if (ext == "jpg" || ext == "jpeg") displayJpgFile(path);
    else if (ext == "png") displayPngFile(path);
    else if (ext == "gif") displayGifFile(path);
    else if (ext == "bmp") displayBmpFile(path);
    else if (ext == "tga") displayTgaFile(path);
    else if (ext == "pcx") displayPcxFile(path);
    else if (ext == "mid" || ext == "midi") playMidiFile(path);
    else if (ext == "mid" || ext == "midi") playMidiFile(path);
else if (ext == "mp3" || ext == "wav" || ext == "m4a" || ext == "aac" || ext == "flac" || ext == "ogg") playAudioFile(path);
    else if (ext == "wav1") displayWavVisualizer(path);
    else if (ext == "ch8" || ext == "rom") runChip8Emulator(path); // CHIP-8 掌机模拟器
    else if (ext == "nes") runNesEmulator(path); // NES/FC游戏机模拟器
    else displayHexInspector(path); // 万能兜底黑客矩阵
    
    CyberLogger::info("SYS_CORE", "Execution cycle finalized. Reclaiming UI control.");
    hasTask = false;
    inMenu = true;  
    loadFontSafe(); 
    drawMenu(); 
}

// ----------------------------------------------------------------------------
// [021] Web OS 核心: 流式上传接收器
// ----------------------------------------------------------------------------
/*void handleFileUpload() {
    HTTPUpload& upload = server.upload();
    
    if (upload.status == UPLOAD_FILE_START) {
        String dir = server.hasArg("dir") ? urlDecode(server.arg("dir")) : "/";
        if (!dir.endsWith("/")) dir += "/";
        String filename = dir + upload.filename;
        
        CyberLogger::warn("WEB_API", "Incoming data stream allocation: " + filename);
        uploadFile = SD.open(filename, FILE_WRITE);
        if (!uploadFile) CyberLogger::error("WEB_API", "Failed to write hardware block!");
    } 
    else if (upload.status == UPLOAD_FILE_WRITE) {
        if (uploadFile) {
            uploadFile.write(upload.buf, upload.currentSize);
        }
    } 
    else if (upload.status == UPLOAD_FILE_END) {
        if (uploadFile) {
            uploadFile.close();
            CyberLogger::info("WEB_API", "Payload completely assembled and saved.");
        }
    }
}
*/
// ----------------------------------------------------------------------------
// [022] Web OS 核心: 极简白色 H5 前端界面 (重构版)
// ----------------------------------------------------------------------------

/*void displayFileList() {
    CyberLogger::debug("WEB_UI", "Generating Clean White Single Page Application...");
    
    String path = "/";
    if (server.hasArg("dir")) {
        path = urlDecode(server.arg("dir"));
        if (!path.startsWith("/")) path = "/" + path;
    }
    
    File dir = SD.open(path);
    if (!dir || !dir.isDirectory()) { 
        server.send(500, "text/html", "<h2>Directory not found</h2>"); 
        return; 
    }

    // 黑名单智能解析引擎 (隐藏 .anim 引用的图片文件夹)
    std::set<String> hiddenFolders; 
    std::vector<String> animFiles;
    
    File entry = dir.openNextFile();
    while (entry) {
        if (!entry.isDirectory() && String(entry.name()).endsWith(".anim")) {
            animFiles.push_back(entry.name());
            String fullAnimPath = path; 
            if (!fullAnimPath.endsWith("/")) fullAnimPath += "/"; 
            fullAnimPath += entry.name();
            
            File script = SD.open(fullAnimPath, FILE_READ);
            if (script) {
                while (script.available()) {
                    String line = script.readStringUntil('\n'); 
                    line.trim();
                    if (line.length() == 0) continue;
                    
                    int sepIdx = line.lastIndexOf('@');
                    String folderName = (sepIdx > 0) ? line.substring(0, sepIdx) : line;
                    if (folderName.startsWith("/")) folderName = folderName.substring(1);
                    hiddenFolders.insert(folderName);
                }
                script.close();
            }
        }
        entry = dir.openNextFile();
    }
    dir.close(); 
    
    dir = SD.open(path);

    // 收集文件和文件夹
    std::vector<String> folders, files, anims;
    entry = dir.openNextFile();
    while (entry) {
        String name = entry.name();
        if (name != "." && name != "..") {
            if (entry.isDirectory()) {
                if (hiddenFolders.find(name) == hiddenFolders.end()) folders.push_back(name);
            } else {
                if (name.endsWith(".anim")) anims.push_back(name);
                else files.push_back(name);
            }
        }
        entry = dir.openNextFile();
    }
    dir.close();

    std::sort(folders.begin(), folders.end()); 
    std::sort(files.begin(), files.end()); 
    std::sort(anims.begin(), anims.end());

    // 构建白色简约 HTML
    String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1'>";
    html += "<title>ESP32 File Manager</title><style>";
    html += "body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,Helvetica,Arial,sans-serif;margin:0;padding:20px;background:#f5f5f5;color:#333;}";
    html += ".container{max-width:900px;margin:0 auto;background:#fff;border-radius:16px;box-shadow:0 4px 12px rgba(0,0,0,0.05);padding:24px;}";
    html += "h2{font-weight:400;margin-top:0;border-bottom:1px solid #eee;padding-bottom:12px;color:#222;}";
    html += ".breadcrumb{margin-bottom:20px;font-size:14px;background:#fafafa;padding:12px;border-radius:8px;}";
    html += ".breadcrumb a{color:#0066cc;text-decoration:none;}";
    html += ".upload-area{margin:20px 0;padding:20px;border:2px dashed #ddd;border-radius:12px;background:#fafafa;}";
    html += "input[type=file]{margin:8px 0;width:100%;}";
    html += ".btn{padding:8px 18px;border:1px solid #ccc;background:#fff;border-radius:20px;font-size:14px;cursor:pointer;transition:0.2s;}";
    html += ".btn-primary{background:#0066cc;border-color:#0066cc;color:#fff;}";
    html += ".btn-primary:hover{background:#004499;}";
    html += ".file-list{list-style:none;padding:0;margin:0;}";
    html += ".file-item{display:flex;align-items:center;padding:12px 8px;border-bottom:1px solid #f0f0f0;}";
    html += ".file-item:hover{background:#f9f9f9;}";
    html += ".icon{width:32px;text-align:center;margin-right:12px;font-size:20px;}";
    html += ".file-name{flex:1;word-break:break-all;}";
    html += ".file-name a{color:#333;text-decoration:none;font-weight:500;}";
    html += ".file-name a:hover{color:#0066cc;}";
    html += ".folder .icon{color:#ffb300;}";
    html += ".video .icon{color:#e74c3c;}";
    html += ".stats{display:flex;gap:24px;margin:16px 0;color:#666;font-size:14px;}";
    html += "@media (max-width:600px){body{padding:12px;}.container{padding:16px;}}";
    html += "</style></head><body><div class='container'>";
    
    // 标题 + 状态
    html += "<h2>📁 ESP32 Cyber OS</h2>";
    html += "<div class='stats'>";
    html += "<span>💾 Free: " + String(ESP.getFreeHeap()/1024) + " KB</span>";
    html += "<span>📀 SD: " + String((unsigned long)(SD.totalBytes()/1024/1024)) + " MB</span>";
    html += "<span>🌡️ Temp: " + String(temperatureRead(),1) + "°C</span>";
    html += "</div>";
    
    // 路径导航
    html += "<div class='breadcrumb'>📍 ";
    if (path != "/") {
        String parent = path.substring(0, path.lastIndexOf('/'));
        if (parent.length() == 0) parent = "/";
        html += "<a href='/?dir=" + urlEncode(parent) + "'>⬅️ Parent</a> | ";
    }
    html += "Current: " + path + "</div>";
    
    // 上传区
    html += "<div class='upload-area'><strong>⬆️ Upload file to this folder</strong><br>";
    html += "<form method='POST' action='/upload?dir=" + urlEncode(path) + "' enctype='multipart/form-data'>";
    html += "<input type='file' name='upload_file' required>";
    html += "<button type='submit' class='btn btn-primary' style='margin-top:8px;'>Upload</button>";
    html += "</form></div>";
    
    // 文件列表
    html += "<ul class='file-list'>";
    
    for (const String& f : folders) { 
        String fp = path; if (!fp.endsWith("/")) fp += "/"; 
        html += "<li class='file-item folder'><span class='icon'>📂</span><span class='file-name'><a href='/?dir=" + urlEncode(fp + f) + "'>" + f + "/</a></span></li>";
    }
    for (const String& f : anims) { 
        String fp = path; if (!fp.endsWith("/")) fp += "/"; 
        String bn = f.substring(0, f.length() - 5);
        html += "<li class='file-item video'><span class='icon'>🎬</span><span class='file-name'><a href='/open?file=" + urlEncode(fp + f) + "'>" + bn + " (VIDEO)</a></span></li>";
    }
    for (const String& f : files) {
        String fp = path; if (!fp.endsWith("/")) fp += "/";
        String icon = "📄";
        if (f.endsWith(".mid")||f.endsWith(".midi")) icon = "🎵";
        else if (f.endsWith(".txt")||f.endsWith(".csv")||f.endsWith(".lrc")) icon = "📝";
        else if (f.endsWith(".jpg")||f.endsWith(".png")||f.endsWith(".bmp")) icon = "🖼️";
        else if (f.endsWith(".ch8")||f.endsWith(".rom")) icon = "🎮";
        else if (f.endsWith(".nes")) icon = "🎮";
        
        html += "<li class='file-item'><span class='icon'>" + icon + "</span><span class='file-name'><a href='/open?file=" + urlEncode(fp + f) + "'>" + f + "</a></span></li>";
    }
    
    html += "</ul>";
    html += "<div style='margin-top:20px;color:#999;font-size:12px;text-align:center;'>ESP32 Cyber OS v5.0 · White Edition</div>";
    html += "</div></body></html>";
    
    server.send(200, "text/html", html);
    CyberLogger::debug("WEB_UI", "Clean HTML DOM transmitted.");
}
*/
/*
void setupWebServer() {
    CyberLogger::info("WEB_INIT", "Binding REST API routes...");
    server.on("/", displayFileList);
    
    server.on("/open", []() {
        if (server.hasArg("file")) {
            String fileName = urlDecode(server.arg("file"));
            if (!fileName.startsWith("/")) fileName = "/" + fileName;
            
            CyberLogger::warn("WEB_API", "Intercepted execution request: " + fileName);
            pendingTask = fileName; 
            hasTask = true;
            
            String html = "<html><body style='font-family:sans-serif;text-align:center;padding:40px;'><h2>✅ Task dispatched</h2><p>Check device screen.</p><a href='javascript:history.back()'>← Back</a></body></html>";
            server.send(200, "text/html", html);
        } else {
            server.send(400, "text/html", "<h1>Missing file argument</h1>");
        }
    });

    server.on("/upload", HTTP_POST, []() {
        server.sendHeader("Connection", "close");
        String html = "<html><body style='font-family:sans-serif;text-align:center;padding:40px;'><h2>✅ Upload completed</h2><a href='javascript:history.back()'>← Back</a></body></html>";
        server.send(200, "text/html", html);
    }, handleFileUpload);
    
    server.begin();
    CyberLogger::info("WEB_INIT", "HTTP Daemon listening on Port 80.");
}
*/

// ----------------------------------------------------------------------------
// [NEW] 实体计算器与AI助手模块
// ----------------------------------------------------------------------------

// 简易表达式求值 (增强支持小数、变量、函数与括号)
#include <math.h>

double parseExpression(const char*& str, double x_val);
double parseTerm(const char*& str, double x_val);
double parseFactor(const char*& str, double x_val);
double parseMulDiv(const char*& str, double x_val);

double parseTerm(const char*& str, double x_val) {
    while (*str == ' ') str++;
    double val = 0;
    if (*str == '(') {
        str++;
        val = parseExpression(str, x_val);
        if (*str == ')') str++;
    } else if (*str == 'x' || *str == 'X') {
        str++;
        val = x_val;
    } else if (strncmp(str, "sin", 3) == 0) { str += 3; val = sin(parseTerm(str, x_val)); }
    else if (strncmp(str, "cos", 3) == 0) { str += 3; val = cos(parseTerm(str, x_val)); }
    else if (strncmp(str, "tan", 3) == 0) { str += 3; val = tan(parseTerm(str, x_val)); }
    else if (strncmp(str, "√", 3) == 0) { str += 3; val = sqrt(parseTerm(str, x_val)); }
    else if (strncmp(str, "sqrt", 4) == 0) { str += 4; val = sqrt(parseTerm(str, x_val)); }
    else if (strncmp(str, "ln", 2) == 0) { str += 2; val = log(parseTerm(str, x_val)); }
    else {
        val = strtod(str, (char**)&str);
    }
    return val;
}

double parseFactor(const char*& str, double x_val) {
    double val = parseTerm(str, x_val);
    while (*str == ' ') str++;
    if (*str == '^') {
        str++;
        double power = parseFactor(str, x_val); // 右结合次方
        val = pow(val, power);
    }
    return val;
}

double parseMulDiv(const char*& str, double x_val) {
    double val = parseFactor(str, x_val);
    while (true) {
        while (*str == ' ') str++;
        if (*str == '*') {
            str++;
            val *= parseFactor(str, x_val);
        } else if (*str == '/') {
            str++;
            val /= parseFactor(str, x_val);
        } else if (*str == 'x' || *str == 'X' || *str == '(' || strncmp(str, "sin", 3) == 0 || strncmp(str, "cos", 3) == 0 || strncmp(str, "tan", 3) == 0 || strncmp(str, "ln", 2) == 0 || strncmp(str, "√", 3) == 0 || strncmp(str, "sqrt", 4) == 0) {
            // Implicit multiplication support (e.g. 2x -> 2 * x)
            val *= parseFactor(str, x_val);
        } else break;
    }
    return val;
}

double parseExpression(const char*& str, double x_val) {
    while (*str == ' ') str++;
    double val = 0;
    bool negative = false;
    if (*str == '-') { negative = true; str++; }
    else if (*str == '+') { str++; }
    val = parseMulDiv(str, x_val);
    if (negative) val = -val;

    while (true) {
        while (*str == ' ') str++;
        if (*str == '+') {
            str++;
            val += parseMulDiv(str, x_val);
        } else if (*str == '-') {
            str++;
            val -= parseMulDiv(str, x_val);
        } else break;
    }
    return val;
}

double calcEval(String expr, double x_val = 0) {
    const char* str = expr.c_str();
    return parseExpression(str, x_val);
}

void runStandardCalc() {
    String expr = "";
    String lastResult = "";
    int selRow = 0;
    int selCol = 0;
    
    // 5行4列标准布局
    const char* keys[5][4] = {
        {"C", "DEL", "(", ")"},
        {"7", "8", "9", "/"},
        {"4", "5", "6", "*"},
        {"1", "2", "3", "-"},
        {"0", ".", "=", "+"}
    };
    
    auto drawUI = [&]() {
        tft.fillScreen(TFT_BLACK);
        loadFontSafe();
        
        // 顶部显示区
        tft.fillRect(5, 5, tft.width()-10, 60, tft.color565(30,30,30));
        tft.setTextColor(TFT_WHITE);
        tft.drawString(expr, 10, 10);
        tft.setTextColor(TFT_GREEN);
        tft.drawString(lastResult, 10, 40);
        
        // 按键网格
        int btnW = (tft.width() - 10) / 4;
        int btnH = (tft.height() - 75) / 5;
        int startY = 70;
        
        for (int r = 0; r < 5; r++) {
            for (int c = 0; c < 4; c++) {
                int px = 5 + c * btnW;
                int py = startY + r * btnH;
                if (r == selRow && c == selCol) {
                    tft.fillRect(px, py, btnW-2, btnH-2, TFT_BLUE);
                } else {
                    tft.fillRect(px, py, btnW-2, btnH-2, tft.color565(60,60,60));
                }
                tft.setTextColor(TFT_WHITE);
                tft.drawString(keys[r][c], px + btnW/2 - 10, py + btnH/2 - 10);
            }
        }
    };
    
    drawUI();
    
    while(true) {
        if (isBtnPressed(BTN_LEFT)) { selCol = (selCol == 0) ? 3 : selCol - 1; drawUI(); }
        if (isBtnPressed(BTN_RIGHT)) { selCol = (selCol == 3) ? 0 : selCol + 1; drawUI(); }
        if (isBtnPressed(BTN_UP)) { selRow = (selRow == 0) ? 4 : selRow - 1; drawUI(); }
        if (isBtnPressed(BTN_DOWN)) { selRow = (selRow == 4) ? 0 : selRow + 1; drawUI(); }
        
        if (digitalRead(BTN_OK) == LOW) {
            unsigned long startWait = millis();
            while(digitalRead(BTN_OK) == LOW) { yield(); }
            if (millis() - startWait > 600) {
                return; // 长按OK退出
            }
            
            String k = keys[selRow][selCol];
            if (k == "C") {
                expr = ""; lastResult = "";
            } else if (k == "DEL") {
                if(expr.length() > 0) expr.remove(expr.length() - 1);
            } else if (k == "=") {
                lastResult = String(calcEval(expr), 6);
            } else {
                expr += k;
            }
            drawUI();
        }
        delay(10);
    }
}

String mathSoftKeyboard(String title, String initExpr, bool isInteractiveCalc) {
    String expr = initExpr;
    String lastResult = "";
    int selRow = 0;
    int selCol = 0;
    
    struct KeyDef { const char* label; const char* val; };
    KeyDef keys[6][5] = {
        {{"sin", "sin("}, {"cos", "cos("}, {"tan", "tan("}, {"ln", "ln("}, {"√", "√("}},
        {{"7", "7"}, {"8", "8"}, {"9", "9"}, {"/", "/"}, {"C", "C"}},
        {{"4", "4"}, {"5", "5"}, {"6", "6"}, {"*", "*"}, {"^", "^"}},
        {{"1", "1"}, {"2", "2"}, {"3", "3"}, {"-", "-"}, {"DEL", "DEL"}},
        {{"0", "0"}, {".", "."}, {"(", "("}, {")", ")"}, {"+", "+"}},
        {{"x", "x"}, {"", ""}, {"", ""}, {isInteractiveCalc ? "=" : "OK", isInteractiveCalc ? "=" : "OK"}, {"EXT", "EXT"}}
    };

    auto drawUI = [&]() {
        tft.fillScreen(TFT_BLACK);
        loadFontSafe();
        tft.fillRect(0, 0, tft.width(), 60, tft.color565(30,30,30));
        tft.setTextColor(TFT_WHITE);
        tft.drawString(title, 5, 2);
        
        tft.setTextColor(TFT_WHITE);
        tft.drawString(expr, 5, 20);
        if (isInteractiveCalc) {
            tft.setTextColor(TFT_GREEN);
            tft.drawString(lastResult, 5, 40);
        }
        
        int btnW = tft.width() / 5;
        int btnH = (tft.height() - 65) / 6;
        int startY = 65;
        for (int r = 0; r < 6; r++) {
            for (int c = 0; c < 5; c++) {
                int px = c * btnW;
                int py = startY + r * btnH;
                if (r == selRow && c == selCol) {
                    tft.fillRect(px, py, btnW-2, btnH-2, TFT_BLUE);
                } else {
                    tft.fillRect(px, py, btnW-2, btnH-2, tft.color565(60,60,60));
                }
                tft.setTextColor(TFT_WHITE);
                tft.drawString(keys[r][c].label, px + 4, py + btnH/2 - 8);
            }
        }
    };
    
    drawUI();
    while(true) {
        if (isBtnPressed(BTN_LEFT)) { selCol = (selCol == 0) ? 4 : selCol - 1; drawUI(); }
        if (isBtnPressed(BTN_RIGHT)) { selCol = (selCol == 4) ? 0 : selCol + 1; drawUI(); }
        if (isBtnPressed(BTN_UP)) { selRow = (selRow == 0) ? 5 : selRow - 1; drawUI(); }
        if (isBtnPressed(BTN_DOWN)) { selRow = (selRow == 5) ? 0 : selRow + 1; drawUI(); }
        if (digitalRead(BTN_OK) == LOW) {
            unsigned long wp = millis();
            while(digitalRead(BTN_OK) == LOW) yield();
            if (millis() - wp > 600) return isInteractiveCalc ? "" : expr;
            
            String v = keys[selRow][selCol].val;
            if (v == "") {} // do nothing
            else if (v == "C") { expr = ""; lastResult = ""; }
            else if (v == "DEL") { if (expr.length() > 0) expr.remove(expr.length() - 1); }
            else if (v == "EXT") return "";
            else if (v == "OK") return expr;
            else if (v == "=") {
                if (isInteractiveCalc) lastResult = String(calcEval(expr), 6);
                else return expr;
            }
            else expr += v;
            drawUI();
        }
        delay(10);
    }
}

void runScientificCalc() {
    mathSoftKeyboard("科学计算器", "", true);
}

void runFuncGraphReal(String expr) {
    if (expr.length() == 0) return;
    
    double x_center = 0.0;
    double y_center = 0.0;
    double scale = 10.0; // 表示屏幕一半代表的坐标系单位长度
    
    auto draw = [&]() {
        tft.fillScreen(TFT_BLACK);
        loadFontSafe();
        int cx = tft.width() / 2;
        int cy = tft.height() / 2;
        
        double x_min = x_center - scale;
        double x_max = x_center + scale;
        double y_min = y_center - scale;
        double y_max = y_center + scale;
        
        // 计算原点在屏幕上的位置
        int origin_x = cx - (int)(x_center / (2*scale) * tft.width());
        int origin_y = cy + (int)(y_center / (2*scale) * tft.height());
        
        // 绘制坐标系轴
        if (origin_y >= 0 && origin_y < tft.height()) {
            tft.drawLine(0, origin_y, tft.width(), origin_y, TFT_WHITE);
        }
        if (origin_x >= 0 && origin_x < tft.width()) {
            tft.drawLine(origin_x, 0, origin_x, tft.height(), TFT_WHITE);
        }
        
        // 自适应刻度步长
        double step = 1.0;
        if (scale > 20) step = 5.0;
        if (scale > 50) step = 10.0;
        
        tft.setTextSize(1); // 自适应大小刻度
        tft.setTextColor(TFT_DARKGREY);
        // X轴刻度
        if (origin_y >= 0 && origin_y < tft.height()) {
            for (double t = ceil(x_min/step)*step; t <= x_max; t += step) {
                int px = cx + (int)((t - x_center) / (2*scale) * tft.width());
                if (px >= 0 && px < tft.width() && abs(t) > 0.01) {
                    tft.drawLine(px, origin_y - 2, px, origin_y + 2, TFT_WHITE);
                    tft.drawString(String((int)t), px - 4, origin_y + 4);
                }
            }
        }
        // Y轴刻度
        if (origin_x >= 0 && origin_x < tft.width()) {
            for (double t = ceil(y_min/step)*step; t <= y_max; t += step) {
                int py = cy - (int)((t - y_center) / (2*scale) * tft.height());
                if (py >= 0 && py < tft.height() && abs(t) > 0.01) {
                    tft.drawLine(origin_x - 2, py, origin_x + 2, py, TFT_WHITE);
                    tft.drawString(String((int)t), origin_x + 6, py - 4);
                }
            }
        }
        tft.setTextSize(2); // 恢复字体大小

        // 绘制函数曲线并寻找交点
        int last_sx = -1, last_sy = -1;
        double prev_y = calcEval(expr, x_min);
        int intersections = 0;
        String x_inter_str = "";

        for (int sx = 0; sx < tft.width(); sx++) {
            double x = x_min + (x_max - x_min) * sx / tft.width();
            double y = calcEval(expr, x);
            
            // X轴交点检测 (由正变负或由负变正)
            if (sx > 0 && prev_y * y <= 0 && abs(y - prev_y) < scale) {
                if (intersections < 3) {
                    x_inter_str += String(x, 1) + " ";
                    intersections++;
                }
            }
            prev_y = y;
            
            if (!isnan(y) && !isinf(y)) {
                int sy = cy - (int)((y - y_center) / (2*scale) * tft.height());
                if (last_sx != -1 && sy >= 0 && sy < tft.height() && last_sy >= 0 && last_sy < tft.height()) {
                    tft.drawLine(last_sx, last_sy, sx, sy, TFT_RED);
                }
                if (sy >= 0 && sy < tft.height()) {
                    last_sx = sx; last_sy = sy;
                } else {
                    last_sx = -1;
                }
            } else {
                last_sx = -1;
            }
        }
        
        // 显示文本信息
        tft.setTextSize(1);
        tft.setTextColor(TFT_WHITE);
        tft.drawString("y=" + expr, 5, 5);
        
        double y_f0 = calcEval(expr, 0);
        tft.setTextColor(TFT_CYAN);
        tft.drawString("Y轴交点: (0, " + String(y_f0, 2) + ")", 5, 20);
        
        if (x_inter_str.length() > 0) {
            tft.drawString("X轴交点: " + x_inter_str, 5, 35);
        }
        
        tft.setTextColor(TFT_YELLOW);
        tft.drawString("[OK退 | 方向键移区]", 5, tft.height() - 15);
        tft.setTextSize(2);
    };
    
    draw();
    while (true) {
        if (isBtnPressed(BTN_OK)) return;
        if (isBtnPressed(BTN_LEFT)) { x_center -= scale * 0.2; draw(); }
        if (isBtnPressed(BTN_RIGHT)) { x_center += scale * 0.2; draw(); }
        if (isBtnPressed(BTN_UP)) { y_center += scale * 0.2; draw(); }
        if (isBtnPressed(BTN_DOWN)) { y_center -= scale * 0.2; draw(); }
        delay(10);
    }
}

void runFuncGraph() {
    int mode = 0;
    auto drawMenu = [&]() {
        tft.fillScreen(TFT_BLACK);
        loadFontSafe();
        tft.setTextColor(TFT_WHITE);
        tft.drawString("选择函数类型", 10, 10);
        
        if (mode == 0) tft.fillRect(15, 35, 220, 24, TFT_GREEN);
        tft.setTextColor(mode == 0 ? TFT_BLACK : TFT_WHITE);
        tft.drawString("1. 一次 (y=kx+b)", 20, 40);
        
        if (mode == 1) tft.fillRect(15, 65, 220, 24, TFT_GREEN);
        tft.setTextColor(mode == 1 ? TFT_BLACK : TFT_WHITE);
        tft.drawString("2. 二次 (y=ax^2+bx+c)", 20, 70);
        
        if (mode == 2) tft.fillRect(15, 95, 220, 24, TFT_GREEN);
        tft.setTextColor(mode == 2 ? TFT_BLACK : TFT_WHITE);
        tft.drawString("3. 反比例 (y=k/x)", 20, 100);
        
        if (mode == 3) tft.fillRect(15, 125, 220, 24, TFT_GREEN);
        tft.setTextColor(mode == 3 ? TFT_BLACK : TFT_WHITE);
        tft.drawString("4. 自定义解析式", 20, 130);
    };
    
    drawMenu();
    while(true) {
        if(isBtnPressed(BTN_LEFT)) return;
        if(isBtnPressed(BTN_UP)) { mode = (mode + 3) % 4; drawMenu(); }
        if(isBtnPressed(BTN_DOWN)) { mode = (mode + 1) % 4; drawMenu(); }
        if(digitalRead(BTN_OK) == LOW) {
            unsigned long wp = millis();
            while(digitalRead(BTN_OK) == LOW) yield();
            String expr = "";
            if (mode == 0) {
                String k = mathSoftKeyboard("输入 k 值:", "1", false);
                if(k=="") { drawMenu(); continue; }
                String b = mathSoftKeyboard("输入 b 值:", "0", false);
                if(b=="") { drawMenu(); continue; }
                expr = "(" + k + ")x+(" + b + ")";
            }
            else if (mode == 1) {
                String a = mathSoftKeyboard("输入 a 值:", "1", false);
                if(a=="") { drawMenu(); continue; }
                String b = mathSoftKeyboard("输入 b 值:", "0", false);
                if(b=="") { drawMenu(); continue; }
                String c = mathSoftKeyboard("输入 c 值:", "0", false);
                if(c=="") { drawMenu(); continue; }
                expr = "(" + a + ")x^2+(" + b + ")x+(" + c + ")";
            }
            else if (mode == 2) {
                String k = mathSoftKeyboard("反比例输入 k 值:", "1", false);
                if(k=="") { drawMenu(); continue; }
                expr = "(" + k + ")/x";
            }
            else if (mode == 3) {
                expr = mathSoftKeyboard("解析式(y=后部分):", "", false);
            }
            
            if (expr.length() > 0) {
                runFuncGraphReal(expr);
            }
            drawMenu();
        }
        delay(10);
    }
}

void runCalculator() {
    int mode = 0;
    auto drawMenu = [&]() {
        tft.fillScreen(TFT_BLACK);
        loadFontSafe();
        tft.setTextColor(TFT_WHITE);
        tft.drawString("选择计算器模式", 10, 20);
        
        if (mode == 0) tft.fillRect(15, 55, 200, 24, TFT_GREEN);
        tft.setTextColor(mode == 0 ? TFT_BLACK : TFT_WHITE);
        tft.drawString("1. 标准计算器", 20, 60);
        
        if (mode == 1) tft.fillRect(15, 85, 200, 24, TFT_GREEN);
        tft.setTextColor(mode == 1 ? TFT_BLACK : TFT_WHITE);
        tft.drawString("2. 科学计算器", 20, 90);
        
        if (mode == 2) tft.fillRect(15, 115, 200, 24, TFT_GREEN);
        tft.setTextColor(mode == 2 ? TFT_BLACK : TFT_WHITE);
        tft.drawString("3. 函数绘制", 20, 120);
        
        tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
        tft.drawString("[左键退 上下动 OK进]", 10, 200);
    };
    
    drawMenu();
    while (true) {
        if (isBtnPressed(BTN_LEFT)) return;
        if (isBtnPressed(BTN_UP)) { mode = (mode + 2) % 3; drawMenu(); }
        if (isBtnPressed(BTN_DOWN)) { mode = (mode + 1) % 3; drawMenu(); }
        if (isBtnPressed(BTN_OK)) {
            unsigned long startWait = millis();
            while(digitalRead(BTN_OK) == LOW) { yield(); }
            
            if (mode == 0) runStandardCalc();
            else if (mode == 1) runScientificCalc();
            else if (mode == 2) runFuncGraph();
            drawMenu();
        }
        delay(10);
    }
}

bool ttsEnabled = false;

struct ChatMsg {
    String role;
    String content;
};

void runAIChat() {
    std::vector<ChatMsg> history;
    int scrollOffset = 0;
    
    if (SD.exists("/ai_history.json")) {
        File f = SD.open("/ai_history.json", FILE_READ);
        if (f) {
            String jsonStr = f.readString();
            f.close();
            DynamicJsonDocument doc(16384);
            deserializeJson(doc, jsonStr);
            JsonArray arr = doc.as<JsonArray>();
            for (JsonObject obj : arr) {
                history.push_back({obj["role"].as<String>(), obj["content"].as<String>()});
            }
        }
    }
    
    if (history.empty()) {
        history.push_back({"system", "You are a helpful assistant. Please answer in Chinese."});
    }

    auto saveHistory = [&]() {
        DynamicJsonDocument doc(16384);
        for(auto &m : history) {
            JsonObject obj = doc.createNestedObject();
            obj["role"] = m.role;
            obj["content"] = m.content;
        }
        File f = SD.open("/ai_history.json", FILE_WRITE);
        if(f) {
            serializeJson(doc, f);
            f.close();
        }
    };
    
    auto drawUI = [&]() {
        tft.fillScreen(TFT_BLACK);
        loadFontSafe();
        
        // 顶部区
        tft.fillRect(0, 0, tft.width(), 30, tft.color565(30, 80, 150));
        tft.setTextColor(TFT_WHITE);
        tft.drawString("AI 助手 (长按OK输入)", 10, 5);
        
        int y = 40 - scrollOffset;
        for (int i = 1; i < history.size(); i++) {
            if (y > tft.height() - 40) break;
            
            bool isUser = history[i].role == "user";
            tft.setTextColor(isUser ? TFT_GREEN : TFT_CYAN);
            String text = (isUser?"你: ":"AI: ") + history[i].content;
            
            int charsPerLine = tft.width() / 20; 
            for (int k = 0; k < text.length(); k += charsPerLine) {
                if (y > 30 && y < tft.height() - 40) {
                    tft.drawString(text.substring(k, k + charsPerLine), 10, y);
                }
                y += 25;
            }
            y += 10;
        }
        
        // 底边栏
        tft.fillRect(0, tft.height() - 30, tft.width(), 30, tft.color565(50,50,50));
        tft.setTextColor(TFT_WHITE);
        String ttsStatus = ttsEnabled ? "语音:开启" : "语音:关闭";
        tft.drawString("[左]返回 [右]" + ttsStatus, 5, tft.height() - 25);
    };

    drawUI();
    
    while(true) {
        if (isBtnPressed(BTN_LEFT)) break;
        if (isBtnPressed(BTN_RIGHT)) {
            ttsEnabled = !ttsEnabled;
            drawUI();
        }
        if (isBtnPressed(BTN_UP)) {
            scrollOffset = max(0, scrollOffset - 25);
            drawUI();
        }
        if (isBtnPressed(BTN_DOWN)) {
            scrollOffset += 25;
            drawUI();
        }
        
        if (digitalRead(BTN_OK) == LOW) {
            unsigned long startWait = millis();
            while(digitalRead(BTN_OK) == LOW) { yield(); }
            if (millis() - startWait > 600) {
                // 长按OK -> 输入问题
                String userInput = inputTextDialog("你:", "");
                if (userInput.length() > 0) {
                    history.push_back({"user", userInput});
                    saveHistory();
                    
                    tft.fillScreen(TFT_BLACK);
                    loadFontSafe();
                    tft.setTextColor(TFT_YELLOW);
                    tft.drawString("思考中...", 10, tft.height()/2);
                    
                    WiFiClientSecure aiClient;
                    aiClient.setInsecure();
                    HTTPClient http;
                    http.begin(aiClient, "https://open.bigmodel.cn/api/paas/v4/chat/completions");
                    http.addHeader("Content-Type", "application/json");
                    http.addHeader("Authorization", "Bearer 47ca424换自己的mFnkF00c");
                    
                    DynamicJsonDocument req(16384);
                    req["model"] = "GLM-4.7-Flash";
                    JsonArray msgs = req.createNestedArray("messages");
                    for (auto &m : history) {
                        JsonObject o = msgs.createNestedObject();
                        o["role"] = m.role;
                        o["content"] = m.content;
                    }
                    String reqStr;
                    serializeJson(req, reqStr);
                    
                    CyberLogger::info("AI_CHAT", "Sending to OpenAI...");
                    CyberLogger::info("AI_CHAT", "Payload: " + reqStr);

                    unsigned long apiStart = millis();
                    int httpCode = http.POST(reqStr);
                    unsigned long duration = millis() - apiStart;
                    CyberLogger::info("AI_CHAT", "HTTP Response Code: " + String(httpCode) + " Time: " + String(duration) + "ms");

                    if (httpCode == 200) {
                        DynamicJsonDocument res(16384);
                        deserializeJson(res, http.getString());
                        String reply = res["choices"][0]["message"]["content"].as<String>();
                        CyberLogger::info("AI_CHAT", "AI Reply: " + reply);
                        history.push_back({"assistant", reply});
                        saveHistory();
                        
                        if (ttsEnabled) {
                            tft.fillScreen(TFT_BLACK);
                            tft.drawString("正在生成语音...", 10, tft.height()/2);
                            HTTPClient ttsHttp;
                            String ttsUrl = "https://openapi.dwo.cc/api/xm_tts?text=" + urlEncode(reply);
                            CyberLogger::info("AI_TTS", "Requesting TTS URL: " + ttsUrl);
                            ttsHttp.begin(ttsUrl);
                            int ttsCode = ttsHttp.GET();
                            CyberLogger::info("AI_TTS", "TTS Response Code: " + String(ttsCode));
                            if (ttsCode == 200) {
                                DynamicJsonDocument ttsRes(4096);
                                deserializeJson(ttsRes, ttsHttp.getString());
                                String audioUrl = ttsRes["data"]["audio_url"].as<String>();
                                if (audioUrl.length() > 0) {
                                    tft.fillScreen(TFT_BLACK);
                                    tft.drawString("正在播放...", 10, tft.height()/2);
                                    Audio audio;
                                    audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
                                    audio.setVolume(10);
                                    pinMode(MAX98357A_SD, OUTPUT);
                                    digitalWrite(MAX98357A_SD, HIGH);
                                    audio.connecttohost(audioUrl.c_str());
                                    while(audio.isRunning()) {
                                        audio.loop();
                                        if (digitalRead(BTN_OK) == LOW) {
                                            break;
                                        }
                                    }
                                    digitalWrite(MAX98357A_SD, LOW);
                                }
                            }
                            ttsHttp.end();
                        }
                    } else {
                        history.push_back({"assistant", "[网络请求失败]"});
                    }
                    http.end();
                    scrollOffset = max(0, (int)history.size() * 25 - tft.height() + 80); 
                    drawUI();
                }
            } else {
                drawUI(); // 短按OK无操作，重绘
            }
        }
        delay(10);
    }
}

// ----------------------------------------------------------------------------
// [023] GUI 屏幕主菜单系统驱动 (增加左右键翻页)
// ----------------------------------------------------------------------------
// 文件管理器全局状态
static std::set<String> _fmHiddenFolders;
static int _fmTotalFolders = 0;
static int _fmTotalAnims = 0;
static int _fmTotalFiles = 0;
static int _fmTotalItems = 0;  // 不含 ".. (Back)"

// 流式加载: 扫描隐藏文件夹 + 计数 (快速, 不存储文件名)
void loadMenu(String path) {
    CyberLogger::info("GUI_SYS", "Scanning: " + path);
    menuItems.clear(); menuIsDir.clear();
    _fmHiddenFolders.clear();

    if (path != "/") {
        menuItems.push_back(".. (Back)");
        menuIsDir.push_back(true);
    }

    File dir = SD.open(path);
    if (!dir || !dir.isDirectory()) { menuIdx = 0; menuTop = 0; return; }

    // 第一遍: 扫描 .anim 隐藏文件夹
    File entry = dir.openNextFile();
    while (entry) {
        if (!entry.isDirectory() && String(entry.name()).endsWith(".anim")) {
            String fullAnimPath = path;
            if (!fullAnimPath.endsWith("/")) fullAnimPath += "/";
            fullAnimPath += entry.name();
            File script = SD.open(fullAnimPath, FILE_READ);
            if (script) {
                while (script.available()) {
                    String line = script.readStringUntil('\n'); line.trim();
                    if (line.length() == 0) continue;
                    int sepIdx = line.lastIndexOf('@');
                    String folderName = (sepIdx > 0) ? line.substring(0, sepIdx) : line;
                    if (folderName.startsWith("/")) folderName = folderName.substring(1);
                    _fmHiddenFolders.insert(folderName);
                }
                script.close();
            }
        }
        entry = dir.openNextFile();
    }
    dir.close();

    // 第二遍: 一次性读取所有条目到内存
    std::vector<String> folders, anims, files;
    dir = SD.open(path);
    entry = dir.openNextFile();
    while (entry) {
        String name = entry.name();
        if (name != "." && name != "..") {
            if (entry.isDirectory()) {
                if (_fmHiddenFolders.find(name) == _fmHiddenFolders.end())
                    folders.push_back(name);
            } else {
                if (name.endsWith(".anim")) anims.push_back(name);
                else files.push_back(name);
            }
        }
        entry = dir.openNextFile();
    }
    dir.close();

    // 排序: 文件夹在前, 然后anim, 然后普通文件
    std::sort(folders.begin(), folders.end());
    std::sort(anims.begin(), anims.end());
    std::sort(files.begin(), files.end());

    for (auto& f : folders) { menuItems.push_back(f); menuIsDir.push_back(true); }
    for (auto& f : anims)   { menuItems.push_back(f); menuIsDir.push_back(false); }
    for (auto& f : files)   { menuItems.push_back(f); menuIsDir.push_back(false); }

    _fmTotalItems = (int)menuItems.size();
    menuIdx = 0; menuTop = 0;
}

void drawMenu() {
    int itemHeight = 22;
    int maxItems = (tft.height() - 30) / itemHeight;
    int total = (int)menuItems.size();
    if (total == 0) { tft.fillScreen(TFT_BLACK); tft.setTextColor(TFT_GREEN, TFT_BLACK); tft.drawString(currentDir, 5, 5); return; }

    tft.unloadFont();
    tft.loadFont(t18);
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.drawString(currentDir, 5, 5);
    tft.drawLine(0, 25, tft.width(), 25, TFT_DARKGREY);

    for (int i = 0; i < maxItems; i++) {
        int idx = menuTop + i;
        if (idx >= total) break;
        int y = 30 + i * itemHeight;

        if (idx == menuIdx) {
            tft.fillRect(0, y, tft.width(), itemHeight, TFT_DARKCYAN);
            tft.setTextColor(TFT_WHITE, TFT_DARKCYAN);
        } else {
            tft.fillRect(0, y, tft.width(), itemHeight, TFT_BLACK);
            tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
        }

        String prefix = "    ";
        if (menuItems[idx] == "SYS_MONITOR") prefix = "[状态] ";
        else if (menuIsDir[idx]) prefix = "[文件夹] ";
        else if (menuItems[idx].endsWith(".anim") || menuItems[idx].endsWith(".avi")) prefix = "[视频] ";
        else if (menuItems[idx].endsWith(".mid") || menuItems[idx].endsWith(".midi")) prefix = "[音乐] ";
        else if (menuItems[idx].endsWith(".txt") || menuItems[idx].endsWith(".csv") || menuItems[idx].endsWith(".lrc")) prefix = "[文本] ";
        else if (menuItems[idx].endsWith(".wav")) prefix = "[音频] ";
        else if (menuItems[idx].endsWith(".ch8")) prefix = "[游戏] ";
        else if (menuItems[idx].endsWith(".nes")) prefix = "[游戏] ";
        else if (menuItems[idx] == "LED_CTRL") prefix = "[灯光] ";
        else if (menuItems[idx] == "IME_TEST") prefix = "[输入法] ";
        else if (menuItems[idx] == "SPACEMAN_CLOCK") prefix = "[时钟] ";
        else if (menuItems[idx] == "APPLE_BLE") prefix = "[蓝牙] ";
        else if (menuItems[idx] == "FC设置") prefix = "[FC] ";
        else if (menuItems[idx] == "闹钟") prefix = "[闹] ";
        else if (menuItems[idx] == "番茄钟") prefix = "[番茄] ";
        else if (menuItems[idx] == "WIFI_SETTINGS") prefix = "[ wifi设置] ";
        else if (menuItems[idx] == "ONLINE_MUSIC") prefix = "[云音乐] ";
        else if (menuItems[idx].endsWith(".jpg") || menuItems[idx].endsWith(".png") || menuItems[idx].endsWith(".bmp") || menuItems[idx].endsWith(".gif")) prefix = "[图] ";
        else prefix = "[?] ";

        tft.drawString(prefix + menuItems[idx], 5, y);
    }
}

// ----------------------------------------------------------------------------
// [024] 引导加载程序 (Boot Sequence) 【移除蜂鸣器相关初始化】
// ----------------------------------------------------------------------------
void setup() {
    Serial.begin(115200); delay(200); 
    Serial.println("\n\n");
    CyberLogger::info("BIOS", "Waking up ESP32 Core Matrix...");

    // 蜂鸣器引脚已移除，无需配置
    setupButtons();

    pinMode(TFT_BL, OUTPUT); 
    digitalWrite(TFT_BL, HIGH);
    {
        Preferences prefs;
        prefs.begin("sys-config", true);
        g_brightness = prefs.getUChar("brightness", 255);
        uint16_t st = prefs.getUShort("sleepTimeout", 15);
        prefs.end();
        analogWrite(TFT_BL, g_brightness);
        g_screenSleepMs = st > 0 ? st * 1000UL : 0;
        lastDesktopActivity = millis();
    }
    tft.init(); 
    tft.setRotation(0); 
    tft.invertDisplay(true); 
    tft.fillScreen(TFT_BLACK);
    
    // 黑客帝国开机动画
    tft.setTextColor(TFT_GREEN, TFT_BLACK); tft.setTextSize(1);
    tft.println("ESP32 Cyber OS v5.0 Monster");
    tft.println("Initializing Microkernel..."); delay(300);
    tft.print("CPU Clock: "); tft.print(ESP.getCpuFreqMHz()); tft.println(" MHz [OK]"); delay(200);
    tft.print("SRAM Size: "); tft.print(ESP.getHeapSize() / 1024); tft.println(" KB [OK]"); delay(200);
    
    tft.print("Mounting Ext4/FAT32 Bus...");
    pinMode(SD_CS, OUTPUT); 
    digitalWrite(SD_CS, HIGH);
    sdSPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
    
    if (!SD.begin(SD_CS, sdSPI, 18000000)) {
        tft.setTextColor(TFT_RED, TFT_BLACK); tft.println(" [FAIL]");
        tft.println("\nKERNEL PANIC: ROOTFS NOT FOUND.");
        while (1) delay(1000); // 彻底死机
    }
    tft.println(" [OK]"); delay(300);

    tft.print("Injecting Font Matrix...");
    loadFontSafe(); 
    tft.println(" [OK]"); delay(200);

    TJpgDec.setJpgScale(1); TJpgDec.setSwapBytes(true);
    /*
    tft.print("Network Link (APMode)...");
    WiFi.softAP(ap_ssid, ap_password);
    tft.println(" [OK]"); delay(200);
    
    tft.print("Spawning Web Daemon...");
    setupWebServer();
    tft.println(" [OK]"); delay(400);
*/
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.println("\nALL SYSTEMS NOMINAL.");
    tft.println("HANDING CONTROL TO USER SPACE...");
    CyberLogger::info("BIOS", "Boot sequence finalized. Loading Shell...");
    delay(1200);
    pixels.begin();
pixels.clear();
pixels.show();
ledMode = LED_STATIC;
 pinMode(MAX98357A_SD, OUTPUT);
  digitalWrite(MAX98357A_SD, LOW); // 初始静音，避免开机”噗”声
//updateLEDFromSaved(); // 可选：从EEPROM恢复上次设置
pinyin_ime_init();        // 加载内置拼音字典
    // === RAM优化：菜单延迟加载，首次使用时再初始化 ===
    // loadMenu(currentDir);  // 删除：不在setup()中加载
    // 尝试自动连接WiFi
loadWiFiCredentials();
loadAlarms();
    // 扫描 /apps/ 下的脚本应用
    scanApps();
}

// ----------------------------------------------------------------------------
// [025] 无尽主事件轮询总线 (Main Kernel Loop) 【增加左右键翻页菜单】
// ----------------------------------------------------------------------------
void executeMenuAction() {
    if (menuIdx < 0 || menuIdx >= (int)menuItems.size()) return;
    String itemName = menuItems[menuIdx];
    bool itemIsDir = menuIsDir[menuIdx];

    if (itemName == "SYS_MONITOR") {
        pendingTask = "/SYS_MONITOR";
        hasTask = true;
    }
    else if (itemName == "LED_CTRL") {
        inMenu = false;
        ledControlScreen();
        inMenu = true;
        drawMenu();
    }
    else if (itemName == "IME_TEST") {
        inMenu = false;
        String result = inputTextDialog("请输入文字:", "");
        tft.fillScreen(TFT_BLACK);
        loadFontSafe();
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.drawString("你输入了:", 10, 30);
        tft.drawString(result, 10, 60);
        tft.drawString("按任意键返回...", 10, 100);
        while (!checkStopAction()) { delay(10); }
        inMenu = true;
        loadFontSafe();
        drawMenu();
    }
    else if (itemName == "WIFI_SETTINGS") {
        wifiSettings();
    }
    else if (itemName == "ONLINE_MUSIC") {
        searchMusicOnline();
    }
    else if (itemName == "SPACEMAN_CLOCK") {
        inMenu = false;
        spacemanClock();
        inMenu = true;
        drawMenu();
    }
    else if (itemName == "APPLE_BLE") {
        inMenu = false;
        appleJuiceControl();
        inMenu = true;
        drawMenu();
    }
    else if (itemName == "计算器") {
        inMenu = false;
        runCalculator();
        inMenu = true;
        drawMenu();
    }
    else if (itemName == "AI助手") {
        inMenu = false;
        runAIChat();
        inMenu = true;
        drawMenu();
    }
    else if (itemName == "FC设置") {
        fcSettings();
    }
    else if (itemName == "闹钟") {
        inMenu = false;
        alarmSettings();
        inMenu = true;
        drawMenu();
    }
    else if (itemName == "番茄钟") {
        inMenu = false;
        pomodoroTimer();
        inMenu = true;
        drawMenu();
    }
    else if (itemIsDir) {
        if (itemName == ".. (Back)") {
            int lastSlash = currentDir.lastIndexOf('/', currentDir.length() - 2);
            if (lastSlash >= 0) {
                currentDir = currentDir.substring(0, lastSlash + 1);
            } else {
                inMenu = false;
                animateTransition();
                onDesktop = true;
                drawDesktop();
                animateRestore();
                return;
            }
        } else {
            if (currentDir == "/") currentDir += itemName;
            else currentDir += "/" + itemName;
        }
        loadMenu(currentDir);
        drawMenu();
    }
    else {
        String filePath = currentDir;
        if (filePath != "/") filePath += "/";
        filePath += itemName;
        pendingTask = filePath;
        hasTask = true;
    }
}
void loop() {
    // === WiFi后台连接监控（非阻塞）===
    static bool wifiConnected = false;
    if (wifiConfigured && !wifiConnected && WiFi.isConnected()) {
        wifiConnected = true;
        CyberLogger::info("WIFI", "后台连接成功! IP: " + WiFi.localIP().toString());
    } else if (wifiConnected && !WiFi.isConnected()) {
        wifiConnected = false;
        CyberLogger::warn("WIFI", "WiFi连接已断开");
    }

    // === WiFi连接成功后，后台NTP对时（仅一次）===
    static bool ntpSyncAttempted = false;
    if (wifiConnected && !ntpSyncAttempted) {
        ntpSyncAttempted = true;
        timeClient.begin();
        timeClient.update();
        if (timeClient.isTimeSet()) {
            setTime(timeClient.getEpochTime());
            ntpSynced = true;
            memset(alarmFiredToday, 0, sizeof(alarmFiredToday));
            CyberLogger::info("NTP", "时间同步成功");
            // 同步成功后刷新桌面显示时间
            if (onDesktop) drawDesktop();
        }
    }

    // === 桌面模式 ===
    if (onDesktop) {
        static bool desktopDrawn = false;
        if (!desktopDrawn) {
            drawDesktop();
            desktopDrawn = true;
        }

        // === 熄屏唤醒 (任意键唤醒) ===
        if (screenSleeping) {
            if (digitalRead(BTN_UP) == LOW || digitalRead(BTN_OK) == LOW ||
                digitalRead(BTN_DOWN) == LOW || digitalRead(BTN_LEFT) == LOW ||
                digitalRead(BTN_RIGHT) == LOW) {
                delay(30);
                wakeScreen();
            }
            return;
        }

        if (isBtnPressed(BTN_UP)) {
            if (desktopCursorY > 0) {
                int idx = desktopPage * DESKTOP_PER_PAGE + (desktopCursorY - 1) * DESKTOP_COLS + desktopCursorX;
                if (idx < desktopTotal()) {
                    desktopCursorY--;
                    drawDesktop();
                }
            }
        }
        if (isBtnPressed(BTN_DOWN)) {
            if (desktopCursorY < DESKTOP_ROWS - 1) {
                int idx = desktopPage * DESKTOP_PER_PAGE + (desktopCursorY + 1) * DESKTOP_COLS + desktopCursorX;
                if (idx < desktopTotal()) {
                    desktopCursorY++;
                    drawDesktop();
                }
            }
        }
        if (isBtnPressed(BTN_LEFT)) {
            if (desktopCursorX > 0) {
                desktopCursorX--;
                drawDesktop();
            } else if (desktopPage > 0) {
                int newPage = desktopPage - 1;
                int cx = DESKTOP_COLS - 1;
                int cy = desktopCursorY;
                // 检查目标位置是否有应用
                int idx;
                do {
                    idx = newPage * DESKTOP_PER_PAGE + cy * DESKTOP_COLS + cx;
                    if (idx < desktopTotal()) break;
                    cx--;
                } while (cx > 0);
                animateDesktopSlide(desktopPage, newPage, -1, cx, cy);
            }
        }
        if (isBtnPressed(BTN_RIGHT)) {
            int totalPages = (desktopTotal() + DESKTOP_PER_PAGE - 1) / DESKTOP_PER_PAGE;
            if (desktopCursorX < DESKTOP_COLS - 1) {
                int idx = desktopPage * DESKTOP_PER_PAGE + desktopCursorY * DESKTOP_COLS + (desktopCursorX + 1);
                if (idx < desktopTotal()) {
                    desktopCursorX++;
                    drawDesktop();
                }
            } else if (desktopPage < totalPages - 1) {
                int newPage = desktopPage + 1;
                int cx = 0, cy = desktopCursorY;
                // 检查目标位置是否有应用
                int idx = newPage * DESKTOP_PER_PAGE + cy * DESKTOP_COLS + cx;
                if (idx >= desktopTotal()) {
                    // 最后一页不满，定位到最后一个应用
                    int remain = desktopTotal() - newPage * DESKTOP_PER_PAGE;
                    cx = (remain - 1) % DESKTOP_COLS;
                    cy = (remain - 1) / DESKTOP_COLS;
                }
                animateDesktopSlide(desktopPage, newPage, +1, cx, cy);
            }
        }
        if (isBtnPressed(BTN_OK)) {
            int idx = desktopPage * DESKTOP_PER_PAGE + desktopCursorY * DESKTOP_COLS + desktopCursorX;
            if (idx < desktopTotal()) {
                desktopDrawn = false;  // 应用退出后重绘桌面
                launchDesktopApp(desktopAppAction(idx));
            }
        }
        // === 自动熄屏检测 (仅桌面模式) ===
        checkScreenSleep();
    }
    // === 文件管理器模式 ===
    else if (inMenu) {
        // RAM优化：菜单按需加载
        if (!menuLoaded) {
            loadMenu(currentDir);
            menuLoaded = true;
        }
        int maxItemsPerPage = (tft.height() - 30) / 22;

        // ---- 方向键处理（不变） ----
        if (isBtnPressed(BTN_UP)) {
            if (menuIdx > 0) menuIdx--;
            if (menuIdx < menuTop) menuTop = menuIdx;
            drawMenu();
        }
        if (isBtnPressed(BTN_DOWN)) {
            int total = (currentDir == "/") ? _fmTotalItems : _fmTotalItems + 1;
            if (menuIdx < total - 1) menuIdx++;
            if (menuIdx >= menuTop + maxItemsPerPage) menuTop = menuIdx - maxItemsPerPage + 1;
            drawMenu();
        }
        if (isBtnPressed(BTN_LEFT)) {
            if (currentDir == "/" || currentDir == "") {
                // 从根目录返回桌面
                inMenu = false;
                animateTransition();
                onDesktop = true;
                drawDesktop();
                animateRestore();
                return;
            } else if (menuTop > 0) {
                menuTop = max(0, menuTop - maxItemsPerPage);
                menuIdx = menuTop;
                drawMenu();
            }
        }
        if (isBtnPressed(BTN_RIGHT)) {
            int total = (currentDir == "/") ? _fmTotalItems : _fmTotalItems + 1;
            if (menuTop + maxItemsPerPage < total) {
                menuTop = min(total - maxItemsPerPage, menuTop + maxItemsPerPage);
                menuIdx = menuTop;
                drawMenu();
            }
        }

        // ---- OK 键：长按/短按区分 ----
        if (digitalRead(BTN_OK) == LOW) {
            unsigned long pressStart = millis();
            while (digitalRead(BTN_OK) == LOW) { yield(); }  // 等待释放
            unsigned long pressDuration = millis() - pressStart;
            delay(50);  // 释放消抖

            if (pressDuration > 600) {   // 长按阈值 600ms
                showLongPressMenu(menuIdx);
            } else {
                executeMenuAction();     // 短按：执行原有打开逻辑
            }
        }
    }

    // 处理全局任务（如从网络层下发的打开文件请求）
    if (hasTask && inMenu) {
        String taskToRun = pendingTask;
        hasTask = false;
        openAndDisplayFile(taskToRun);
    }

    // === 闹钟检测（每秒一次，NTP已同步时）===
    if (ntpSynced && !alarmTriggered) {
        static unsigned long lastAlarmCheck = 0;
        if (millis() - lastAlarmCheck >= 1000) {
            lastAlarmCheck = millis();
            checkAlarms();
        }
    }
    if (alarmTriggered) {
        handleAlarmRinging();
    }

    handleLedAnimation();
}

// ============================= EOF =======================================
