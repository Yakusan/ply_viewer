#include <QApplication>
#include "mainwindow.h"
#include <omp.h>

int main(int argc, char *argv[])
{
  omp_set_num_threads(16);
  QApplication app(argc, argv);
  MainWindow mainWindow;
  mainWindow.show();
  return app.exec();
}
