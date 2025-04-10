#include "pumpcontroller.h"

#include <QApplication>
#include <QPalette>
#include <QStyleFactory>


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QApplication::setStyle(QStyleFactory::create("Fusion"));

    QFont appFont;

#ifdef Q_OS_MAC
    appFont = QFont("Helvetica Neue", 12);  // macOS default system font
#elif defined(Q_OS_WIN)
    appFont = QFont("Segoe UI", 9);  // Windows system font
#else
    appFont = QFont("Sans Serif", 9);  // Linux or fallback
#endif

    QApplication::setFont(appFont);


    // Set a light color palette manually
    QPalette lightPalette;
    lightPalette.setColor(QPalette::Window, QColor(240, 240, 240));
    lightPalette.setColor(QPalette::WindowText, Qt::black);
    lightPalette.setColor(QPalette::Base, Qt::white);
    lightPalette.setColor(QPalette::AlternateBase, QColor(225, 225, 225));
    lightPalette.setColor(QPalette::ToolTipBase, Qt::white);
    lightPalette.setColor(QPalette::ToolTipText, Qt::black);
    lightPalette.setColor(QPalette::Text, Qt::black);
    lightPalette.setColor(QPalette::Button, QColor(240, 240, 240));
    lightPalette.setColor(QPalette::ButtonText, Qt::black);
    lightPalette.setColor(QPalette::BrightText, Qt::red);
    lightPalette.setColor(QPalette::Link, QColor(0, 120, 215));
    lightPalette.setColor(QPalette::Highlight, QColor(0, 120, 215));
    lightPalette.setColor(QPalette::HighlightedText, Qt::white);

    //a.setPalette(lightPalette);
    PumpController w;
    w.show();
    return a.exec();
}
