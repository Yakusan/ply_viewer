#pragma once

#include <QMainWindow>

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  MainWindow();

protected slots:
  void _openFileDialog();
  void _openView(const QString& configPath);
  void _closeView();
};
