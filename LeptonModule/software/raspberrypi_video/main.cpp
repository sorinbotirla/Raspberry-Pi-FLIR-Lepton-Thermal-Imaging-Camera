#include <QApplication>
#include <QVBoxLayout>
#include <QWidget>
#include <QCoreApplication>
#include <QFileInfo>
#include "Config.h"
#include "CmdServer.h"

#include "LeptonThread.h"
#include "UsbCamThread.h"
#include "MyLabel.h"

int main(int argc, char **argv)
{
        int typeColormap = 3;
        int typeLepton = 2;
        int spiSpeed = 20;
        int rangeMin = -1;
        int rangeMax = -1;
        int loglevel = 0;

        for(int i=1; i < argc; i++) {
                if ((strcmp(argv[i], "-cm") == 0) && (i + 1 != argc)) {
                        int val = std::atoi(argv[i + 1]);
                        if ((val == 1) || (val == 2)) { typeColormap = val; i++; }
                } else if ((strcmp(argv[i], "-tl") == 0) && (i + 1 != argc)) {
                        int val = std::atoi(argv[i + 1]);
                        if (val == 3) { typeLepton = val; i++; }
                } else if ((strcmp(argv[i], "-ss") == 0) && (i + 1 != argc)) {
                        int val = std::atoi(argv[i + 1]);
                        if ((10 <= val) && (val <= 30)) { spiSpeed = val; i++; }
                } else if ((strcmp(argv[i], "-min") == 0) && (i + 1 != argc)) {
                        int val = std::atoi(argv[i + 1]);
                        if ((0 <= val) && (val <= 65535)) { rangeMin = val; i++; }
                } else if ((strcmp(argv[i], "-max") == 0) && (i + 1 != argc)) {
                        int val = std::atoi(argv[i + 1]);
                        if ((0 <= val) && (val <= 65535)) { rangeMax = val; i++; }
                } else if ((strcmp(argv[i], "-d") == 0) && (i + 1 != argc)) {
                        int val = std::atoi(argv[i + 1]);
                        if (0 <= val) { loglevel = val & 0xFF; i++; }
                }
        }

        QApplication a(argc, argv);

        AppCfg cfg;

        QString cfgPath = QCoreApplication::applicationDirPath() + "/config.json";
        if (argc >= 2 && QString(argv[1]).endsWith(".json")) {
            cfgPath = argv[1];
        }

        bool ok = ConfigIO::load(cfgPath, cfg);
        qDebug() << "CONFIG LOAD" << cfgPath << ok
                 << "bg=" << cfg.background
                 << "usb_dev=" << cfg.usb.device;
        qDebug() << "CFG thermal"
                 << cfg.thermal.xform.offset_x
                 << cfg.thermal.xform.offset_y
                 << cfg.thermal.xform.scale
                 << cfg.thermal.xform.rotate_deg;

        qDebug() << "CFG usb"
                 << cfg.usb.xform.offset_x
                 << cfg.usb.xform.offset_y
                 << cfg.usb.xform.scale
                 << cfg.usb.xform.rotate_deg;

        QWidget *w = new QWidget;
        QVBoxLayout *layout = new QVBoxLayout(w);
        layout->setContentsMargins(0,0,0,0);
        layout->setSpacing(0);

        MyLabel *myLabel = new MyLabel(w);
        myLabel->setLogo(
            "flir_logo.png",
            70,  // height
            60   // margin from edges in pixels
        );
        layout->addWidget(myLabel);
        myLabel->setConfig(cfg);
        CmdServer *cmd = new CmdServer(
            "/tmp/lepton_cmd",
            cfgPath,
            &cfg,
            w
        );

        LeptonThread *thread = new LeptonThread();
        thread->setBackgroundMode(cfg.background);
        thread->setLogLevel(loglevel);
        thread->useColormap(typeColormap);
        thread->useLepton(typeLepton);
        thread->useSpiSpeedMhz(spiSpeed);
        thread->setAutomaticScalingRange();

        QObject::connect(cmd, &CmdServer::configChanged, [&cfg, myLabel, thread]() {
            myLabel->setConfig(cfg);
            thread->setBackgroundMode(cfg.background);
        });


        if (0 <= rangeMin) thread->useRangeMinValue(rangeMin);
        if (0 <= rangeMax) thread->useRangeMaxValue(rangeMax);

        QObject::connect(thread, SIGNAL(updateImage(QImage)), myLabel, SLOT(setImage(QImage)));
        thread->start();

       UsbCamThread *cam = new UsbCamThread(cfg.usb.device);
       cam->setSize(cfg.usb.width, cfg.usb.height);
       cam->setFps(cfg.usb.fps);
        QObject::connect(cam, SIGNAL(updateCamera(QImage)), myLabel, SLOT(setCameraImage(QImage)));
        cam->start();

        w->showFullScreen();
        return a.exec();
}
