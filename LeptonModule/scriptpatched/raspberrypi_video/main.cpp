#include <QApplication>
#include <QVBoxLayout>
#include <QWidget>

#include "LeptonThread.h"
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

        QWidget *w = new QWidget;
        QVBoxLayout *layout = new QVBoxLayout(w);
        layout->setContentsMargins(0,0,0,0);
        layout->setSpacing(0);

        MyLabel *label = new MyLabel(w);
        layout->addWidget(label);

        LeptonThread *thread = new LeptonThread();
        thread->setLogLevel(loglevel);
        thread->useColormap(typeColormap);
        thread->useLepton(typeLepton);
        thread->useSpiSpeedMhz(spiSpeed);
        thread->setAutomaticScalingRange();
        if (0 <= rangeMin) thread->useRangeMinValue(rangeMin);
        if (0 <= rangeMax) thread->useRangeMaxValue(rangeMax);

        QObject::connect(thread, SIGNAL(updateImage(QImage)), label, SLOT(setImage(QImage)));
        thread->start();

        w->showFullScreen();
        return a.exec();
}
