#pragma once

#include <QWidget>
#include <QVector3D>
#include <QSharedPointer>
#include <QLabel>

#include "camera.h"

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


private:
  Scene* _scene;
  QLabel* _lblColorBy;
  QLabel* _lblDistanceInfo;
  QLabel* _lblCamera;
};
