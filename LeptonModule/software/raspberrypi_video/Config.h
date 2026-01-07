#pragma once
#include <QString>

struct LayerCfg {
    int offset_x = 0;
    int offset_y = 0;
    int rotate_deg = 0;
    double scale = 1.0;
    double opacity = 1.0;
    bool flip_h = false;
    bool flip_v = false;
};

struct UsbCamCfg {
    bool enabled = true;
    QString device = "/dev/video0";
    int width = 640;
    int height = 480;
    int fps = 15;
    bool emboss = false;
    LayerCfg xform;
};

struct ThermalCfg {
    bool enabled = true;
    int smooth = 0; // 0=off, higher=stronger
    LayerCfg xform;
};

struct AppCfg {
    QString background = "black"; // "black" or "grey"
    UsbCamCfg usb;
    ThermalCfg thermal;
};

class ConfigIO {
public:
    static bool load(const QString& path, AppCfg& out);
    static bool save(const QString& path, const AppCfg& in);
};
