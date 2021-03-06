#pragma once

#include <QWidget>
#include <QLabel>

// declare but not include to hide scene interface
class Scene;

class Viewer : public QWidget
{
  Q_OBJECT

public:

  Viewer(const QString& configPath);
  ~Viewer();


protected:
  void keyPressEvent(QKeyEvent *);


private slots:
  void _updatePointSize(int);
  void _updateVoxelSize(int);


private:
  Scene* _scene;
};
