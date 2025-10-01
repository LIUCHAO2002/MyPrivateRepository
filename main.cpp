#include "main_window.h"
#include <QApplication>
#include <QFont>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    
    // 设置全局字体，确保中文正常显示
    QFont font("SimHei", 9);
    a.setFont(font);
    
    MainWindow w;
    w.show();
    
    return a.exec();
}
    