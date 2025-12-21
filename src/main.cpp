#include "MainWindow.h"

#include <QApplication>
#include <QIcon>

extern "C" {
#include <libavformat/avformat.h>
}

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);

    avformat_network_init();
    a.setWindowIcon(QIcon(":/AudioFFT_logo.ico")); 
    MainWindow w;
    w.show();
    
    int return_code = a.exec();

    return return_code;
}