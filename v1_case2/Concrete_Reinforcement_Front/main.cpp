#include "stdafx.h"
#include "Concrete_Reinforcement_Front.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    // QApplication is a class that manages the GUI application's control flow and main settings.
    QApplication a(argc, argv);
    // Concrete_Reinforcement_Front is the main window of the application.
    Concrete_Reinforcement_Front w;
    // Show the main window.
    w.show();
    // Start the application's event loop.
    return a.exec();
}
