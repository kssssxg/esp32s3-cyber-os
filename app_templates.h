#ifndef __APP_TEMPLATES_H
#define __APP_TEMPLATES_H

// 模板应用系统 — 替代BASIC脚本引擎
// 预编译的app框架，通过app.info配置文件自定义内容

#include <Arduino.h>
#include <SD.h>
#include <TFT_eSPI.h>
#include <TimeLib.h>
#include <vector>
#include <map>

extern TFT_eSPI tft;
extern const uint8_t t18[];
extern void loadFontSafe();
extern void unloadFontSafe();

// 按键引脚
#define TPL_BTN_UP    4
#define TPL_BTN_OK    5
#define TPL_BTN_DOWN  16
#define TPL_BTN_LEFT  17
#define TPL_BTN_RIGHT 47

// 非阻塞按键检测 (带消抖)
static bool tplBtnPressed(uint8_t pin) {
    if (digitalRead(pin) == LOW) {
        unsigned long start = millis();
        while (digitalRead(pin) == LOW && millis() - start < 300) { yield(); }
        delay(30);
        return true;
    }
    return false;
}

static bool tplAnyBtn() {
    return digitalRead(TPL_BTN_UP) == LOW || digitalRead(TPL_BTN_DOWN) == LOW ||
           digitalRead(TPL_BTN_LEFT) == LOW || digitalRead(TPL_BTN_RIGHT) == LOW ||
           digitalRead(TPL_BTN_OK) == LOW;
}

// ============================================================
// AppConfig — 解析 app.info 配置文件
// ============================================================

struct AppConfig {
    String name;
    uint16_t color;
    String tmpl;
    String title;
    std::vector<String> items;
    String style;
    std::map<String, String> kv;

    String get(const String& key, const String& def = "") {
        auto it = kv.find(key);
        return (it != kv.end()) ? it->second : def;
    }
    int getInt(const String& key, int def = 0) {
        String v = get(key);
        return v.length() > 0 ? v.toInt() : def;
    }

    bool load(const String& infoPath) {
        File f = SD.open(infoPath);
        if (!f) return false;

        name = "App";
        color = 0xFFFF;
        tmpl = "info";

        while (f.available()) {
            String line = f.readStringUntil('\n');
            line.trim();
            if (line.length() == 0 || line.startsWith("#")) continue;
            int eq = line.indexOf('=');
            if (eq < 0) continue;
            String key = line.substring(0, eq); key.trim();
            String val = line.substring(eq + 1); val.trim();
            kv[key] = val;

            if (key == "name") name = val;
            else if (key == "color") color = (uint16_t)strtol(val.c_str(), nullptr, 0);
            else if (key == "template") tmpl = val;
            else if (key == "title") title = val;
            else if (key == "style") style = val;
            else if (key == "items") {
                items.clear();
                int start = 0;
                for (int i = 0; i <= (int)val.length(); i++) {
                    if (i == (int)val.length() || val[i] == ',') {
                        String item = val.substring(start, i); item.trim();
                        if (item.length() > 0) items.push_back(item);
                        start = i + 1;
                    }
                }
            }
        }
        f.close();
        if (title.length() == 0) title = name;
        return true;
    }
};

// ============================================================
// tplInfo — 信息展示页
// ============================================================
void tplInfo(AppConfig& cfg) {
    tft.fillScreen(TFT_BLACK);
    tft.loadFont(t18);

    uint16_t titleColor = cfg.getInt("title_color", 0x07FF);
    uint16_t textColor = cfg.getInt("text_color", 0xFFFF);
    uint16_t lineColor = cfg.getInt("line_color", 0x4208);

    // 标题
    tft.setTextColor(titleColor, TFT_BLACK);
    tft.drawString(cfg.title, 10, 10);
    tft.drawLine(0, 30, 240, 30, lineColor);

    // 文本行
    tft.setTextColor(textColor, TFT_BLACK);
    for (int i = 1; i <= 8; i++) {
        String key = "line" + String(i);
        String val = cfg.get(key);
        if (val.length() == 0) break;
        tft.drawString(val, 10, 35 + (i - 1) * 24);
    }

    // 底部提示
    tft.setTextColor(0x8410, TFT_BLACK);
    tft.drawString("Press OK to exit", 40, 218);

    while (!tplBtnPressed(TPL_BTN_OK)) { delay(50); }
    tft.unloadFont();
}

// ============================================================
// tplClock — 数字时钟
// ============================================================
void tplClock(AppConfig& cfg) {
    tft.loadFont(t18);
    bool showSec = cfg.getInt("show_sec", 1);
    int fmt = cfg.getInt("format", 24);
    uint16_t titleColor = cfg.getInt("title_color", 0x07FF);
    uint16_t timeColor = cfg.getInt("time_color", 0xFFFF);
    uint16_t secColor = cfg.getInt("sec_color", 0x8410);

    unsigned long lastDraw = 0;
    bool needRedraw = true;

    while (true) {
        unsigned long now_ms = millis();
        if (now_ms - lastDraw >= 1000 || needRedraw) {
            lastDraw = now_ms;
            needRedraw = false;

            tft.fillScreen(TFT_BLACK);
            tft.setTextColor(titleColor, TFT_BLACK);
            tft.setTextDatum(TC_DATUM);
            tft.drawString(cfg.title, 120, 20);
            tft.setTextDatum(TL_DATUM);

            int h = hour(), m = minute(), s = second();
            String ampm = "";
            if (fmt == 12) {
                if (h >= 12) { ampm = " PM"; if (h > 12) h -= 12; }
                else { ampm = " AM"; if (h == 0) h = 12; }
            }

            char buf[16];
            if (showSec) snprintf(buf, sizeof(buf), "%02d:%02d:%02d", h, m, s);
            else snprintf(buf, sizeof(buf), "%02d:%02d", h, m);

            tft.setTextColor(timeColor, TFT_BLACK);
            tft.setTextDatum(TC_DATUM);
            tft.setTextSize(2);
            tft.drawString(String(buf) + ampm, 120, 80);
            tft.setTextSize(1);

            // 日期
            char dateBuf[32];
            snprintf(dateBuf, sizeof(dateBuf), "%04d-%02d-%02d", year(), month(), day());
            tft.setTextColor(secColor, TFT_BLACK);
            tft.drawString(dateBuf, 120, 140);

            const char* weekDays[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
            int wd = weekday() - 1;
            tft.drawString(weekDays[wd], 120, 165);

            tft.setTextDatum(TL_DATUM);
            tft.setTextColor(0x8410, TFT_BLACK);
            tft.drawString("Press OK to exit", 40, 218);
        }

        if (tplBtnPressed(TPL_BTN_OK)) break;
        delay(100);
    }
    tft.setTextSize(1);
    tft.unloadFont();
}

// ============================================================
// tplList — 可滚动列表
// ============================================================
void tplList(AppConfig& cfg) {
    tft.loadFont(t18);
    uint16_t titleColor = cfg.getInt("title_color", 0x07FF);
    uint16_t selBg = cfg.getInt("sel_bg", 0x2945);
    uint16_t selBorder = cfg.getInt("sel_border", 0xFFE0);
    uint16_t itemColor = cfg.getInt("item_color", 0xFFFF);

    int count = (int)cfg.items.size();
    if (count == 0) {
        tft.fillScreen(TFT_BLACK);
        tft.setTextColor(0xFFFF, TFT_BLACK);
        tft.drawString("No items configured", 30, 100);
        delay(2000);
        tft.unloadFont();
        return;
    }

    int cursor = 0, scroll = 0;
    int itemH = 28;
    int maxVis = (240 - 50) / itemH;
    if (maxVis > count) maxVis = count;

    auto drawUI = [&]() {
        tft.fillScreen(TFT_BLACK);
        tft.setTextColor(titleColor, TFT_BLACK);
        tft.drawString(cfg.title, 10, 8);
        tft.drawLine(0, 28, 240, 28, 0x4208);

        for (int i = 0; i < maxVis; i++) {
            int idx = scroll + i;
            if (idx >= count) break;
            int y = 34 + i * itemH;

            bool sel = (idx == cursor);
            if (sel) {
                tft.fillRect(4, y, 232, itemH - 2, selBg);
                tft.drawRoundRect(4, y, 232, itemH - 2, 4, selBorder);
                tft.setTextColor(selBorder, selBg);
            } else {
                tft.setTextColor(itemColor, TFT_BLACK);
            }
            tft.drawString(cfg.items[idx], 14, y + 4);
        }

        // 选中提示
        if (count > 0) {
            tft.setTextColor(0x8410, TFT_BLACK);
            String info = cfg.items[cursor];
            if (info.length() > 20) info = info.substring(0, 20);
            tft.drawString("[" + info + "]", 10, 218);
        }

        tft.setTextColor(0x8410, TFT_BLACK);
        tft.drawString("UD:Scroll OK:Select L:Exit", 10, 230);
    };

    drawUI();

    while (true) {
        bool changed = false;
        if (tplBtnPressed(TPL_BTN_UP)) {
            cursor--; changed = true;
        }
        if (tplBtnPressed(TPL_BTN_DOWN)) {
            cursor++; changed = true;
        }
        if (tplBtnPressed(TPL_BTN_LEFT)) break;
        if (tplBtnPressed(TPL_BTN_OK)) {
            // 高亮选中项闪烁提示
            int y = 34 + (cursor - scroll) * itemH;
            tft.fillRect(4, y, 232, itemH - 2, 0xF800);
            tft.setTextColor(0xFFFF, 0xF800);
            tft.drawString(cfg.items[cursor], 14, y + 4);
            delay(300);
            changed = true;
        }

        if (changed) {
            if (cursor < 0) cursor = count - 1;
            if (cursor >= count) cursor = 0;
            if (cursor < scroll) scroll = cursor;
            if (cursor >= scroll + maxVis) scroll = cursor - maxVis + 1;
            drawUI();
        }
        delay(30);
    }
    tft.unloadFont();
}

// ============================================================
// tplCanvas — 自动绘图/动画
// ============================================================
void tplCanvas(AppConfig& cfg) {
    tft.loadFont(t18);
    String style = cfg.style;
    if (style.length() == 0) style = "stars";
    int speed = cfg.getInt("speed", 50);
    if (speed < 5) speed = 5;
    if (speed > 500) speed = 500;

    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(0xFFFF, TFT_BLACK);
    tft.setTextDatum(TC_DATUM);
    tft.drawString(cfg.title, 120, 5);
    tft.setTextDatum(TL_DATUM);

    if (style == "stars") {
        // 随机像素星空
        int count = 0;
        while (!tplBtnPressed(TPL_BTN_OK)) {
            for (int i = 0; i < 20; i++) {
                int x = random(240), y = 30 + random(200);
                uint16_t c = random(65535);
                tft.drawPixel(x, y, c);
            }
            count += 20;
            if (count > 8000) {
                tft.fillRect(0, 30, 240, 210, TFT_BLACK);
                count = 0;
            }
            delay(speed / 10);
        }
    }
    else if (style == "matrix") {
        // 彩色矩阵渐变
        tft.fillRect(0, 30, 240, 210, TFT_BLACK);
        for (int row = 0; row < 21; row++) {
            for (int col = 0; col < 24; col++) {
                uint16_t c = (uint16_t)(row * 1024 + col * 32);
                tft.fillRect(col * 10, 30 + row * 10, 10, 10, c);
            }
        }
        tft.setTextColor(0xFFFF, TFT_BLACK);
        tft.drawString("Press OK to exit", 40, 225);
        while (!tplBtnPressed(TPL_BTN_OK)) { delay(50); }
    }
    else if (style == "rainbow") {
        // 彩虹渐变条纹动画
        int offset = 0;
        while (!tplBtnPressed(TPL_BTN_OK)) {
            for (int y = 30; y < 240; y++) {
                int hue = ((y - 30) * 360 / 210 + offset) % 360;
                uint16_t c;
                if (hue < 60) c = ((uint16_t)(31 * hue / 60) << 11) | (31 << 6);
                else if (hue < 120) c = (31 << 11) | ((uint16_t)(31 * (120 - hue) / 60) << 6);
                else if (hue < 180) c = ((uint16_t)(31 * (hue - 120) / 60) << 6) | 31;
                else if (hue < 240) c = (31 << 11) | 31;
                else c = ((uint16_t)(31 * (300 - hue) / 60) << 11) | 31;
                tft.drawFastHLine(0, y, 240, c);
            }
            offset = (offset + 5) % 360;
            delay(speed / 5);
        }
    }
    else if (style == "wave") {
        // 正弦波动画
        float phase = 0;
        while (!tplBtnPressed(TPL_BTN_OK)) {
            tft.fillRect(0, 30, 240, 210, TFT_BLACK);
            for (int x = 0; x < 240; x += 2) {
                int y1 = 130 + (int)(sin((x * 0.03) + phase) * 50);
                int y2 = 130 + (int)(sin((x * 0.03) + phase + 1.5) * 40);
                uint16_t c1 = 0x07FF; // cyan
                uint16_t c2 = 0xF81F; // magenta
                tft.fillCircle(x, y1, 2, c1);
                tft.fillCircle(x, y2, 2, c2);
            }
            phase += 0.3;
            delay(speed / 5);
        }
    }
    else {
        // 默认: 随机色块
        while (!tplBtnPressed(TPL_BTN_OK)) {
            int x = random(220), y = 30 + random(200);
            int w = 5 + random(20), h = 5 + random(20);
            tft.fillRect(x, y, w, h, random(65535));
            delay(speed / 10);
        }
    }

    tft.unloadFont();
}

// ============================================================
// tplCounter — 计数器
// ============================================================
void tplCounter(AppConfig& cfg) {
    tft.loadFont(t18);
    int value = cfg.getInt("start", 0);
    int step = cfg.getInt("step", 1);
    uint16_t titleColor = cfg.getInt("title_color", 0x07FF);
    uint16_t numColor = cfg.getInt("num_color", 0xFFE0);

    auto drawUI = [&]() {
        tft.fillScreen(TFT_BLACK);
        tft.setTextColor(titleColor, TFT_BLACK);
        tft.setTextDatum(TC_DATUM);
        tft.drawString(cfg.title, 120, 20);

        tft.setTextColor(numColor, TFT_BLACK);
        tft.setTextSize(3);
        tft.drawString(String(value), 120, 100);
        tft.setTextSize(1);

        tft.setTextColor(0x8410, TFT_BLACK);
        tft.drawString("UP:+" + String(step) + " DOWN:-" + String(step), 50, 170);
        tft.drawString("OK:Reset L:Exit", 55, 195);
        tft.setTextDatum(TL_DATUM);
    };

    drawUI();

    while (true) {
        bool changed = false;
        if (tplBtnPressed(TPL_BTN_UP)) { value += step; changed = true; }
        if (tplBtnPressed(TPL_BTN_DOWN)) { value -= step; changed = true; }
        if (tplBtnPressed(TPL_BTN_OK)) { value = cfg.getInt("start", 0); changed = true; }
        if (tplBtnPressed(TPL_BTN_LEFT)) break;
        if (changed) drawUI();
        delay(30);
    }
    tft.setTextSize(1);
    tft.unloadFont();
}

// ============================================================
// tplSensor — 系统信息
// ============================================================
void tplSensor(AppConfig& cfg) {
    tft.loadFont(t18);
    uint16_t titleColor = cfg.getInt("title_color", 0x07FF);
    uint16_t labelColor = cfg.getInt("label_color", 0x8410);
    uint16_t valueColor = cfg.getInt("value_color", 0xFFFF);

    unsigned long lastDraw = 0;
    bool needRedraw = true;

    while (true) {
        if (millis() - lastDraw >= 2000 || needRedraw) {
            lastDraw = millis();
            needRedraw = false;

            tft.fillScreen(TFT_BLACK);
            tft.setTextColor(titleColor, TFT_BLACK);
            tft.setTextDatum(TC_DATUM);
            tft.drawString(cfg.title, 120, 8);
            tft.setTextDatum(TL_DATUM);
            tft.drawLine(0, 28, 240, 28, 0x4208);

            int y = 36;
            auto drawRow = [&](const char* label, const String& val) {
                tft.setTextColor(labelColor, TFT_BLACK);
                tft.drawString(label, 10, y);
                tft.setTextColor(valueColor, TFT_BLACK);
                tft.drawString(val, 130, y);
                y += 24;
            };

            drawRow("Free Heap:", String(ESP.getFreeHeap() / 1024) + " KB");
            drawRow("PSRAM Free:", String(ESP.getFreePsram() / 1024) + " KB");
            drawRow("CPU Freq:", String(ESP.getCpuFreqMHz()) + " MHz");

            // WiFi
            bool wifiOn = WiFi.isConnected();
            drawRow("WiFi:", wifiOn ? WiFi.SSID() : "Disconnected");
            if (wifiOn) {
                drawRow("RSSI:", String(WiFi.RSSI()) + " dBm");
                drawRow("IP:", WiFi.localIP().toString());
            }

            // Uptime
            unsigned long sec = millis() / 1000;
            int hr = sec / 3600, mn = (sec % 3600) / 60, sc = sec % 60;
            char ubuf[16];
            snprintf(ubuf, sizeof(ubuf), "%02d:%02d:%02d", hr, mn, sc);
            drawRow("Uptime:", String(ubuf));

            // 温度 (ESP32内置)
            #ifdef CONFIG_IDF_TARGET_ESP32S3
            drawRow("Chip:", "ESP32-S3");
            #else
            drawRow("Chip:", "ESP32");
            #endif

            tft.setTextColor(0x8410, TFT_BLACK);
            tft.drawString("Press OK to exit", 40, 222);
        }

        if (tplBtnPressed(TPL_BTN_OK)) break;
        delay(100);
    }
    tft.unloadFont();
}

// ============================================================
// 统一入口
// ============================================================
void runAppTemplate(AppConfig& cfg) {
    if (cfg.tmpl == "info") tplInfo(cfg);
    else if (cfg.tmpl == "clock") tplClock(cfg);
    else if (cfg.tmpl == "list") tplList(cfg);
    else if (cfg.tmpl == "canvas") tplCanvas(cfg);
    else if (cfg.tmpl == "counter") tplCounter(cfg);
    else if (cfg.tmpl == "sensor") tplSensor(cfg);
    else tplInfo(cfg);
}

#endif
