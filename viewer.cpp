#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QPushButton>
#include <QDesktopWidget>
#include <QApplication>
#include <QMessageBox>
#include <QComboBox>
#include <QTimer>
#include <QGroupBox>
#include <QCheckBox>
#include <QSlider>
#include <QMatrix3x3>
#include <QVector3D>

#include "camera.h"
#include "scene.h"
#include "viewer.h"

#include <cassert>
#include <fstream>
#include <string>
#include <sstream>

Viewer::Viewer(const QString& configPath)
{
  // accept keyboard input
  setFocusPolicy(Qt::StrongFocus);
  setFocus();
  QFile config;
  config.setFileName(configPath);
  config.open(QIODevice::ReadOnly);
  QString buff = config.readAll();
  QStringList list = buff.split('\n');
  QString plyPath = list[0];
  QString bundlePath = list[1];
  int hImg = list[2].toInt();
  int nbVox = list[3].toInt();

  //
  // make and connect scene widget
  //
  _scene = new Scene(plyPath, bundlePath, hImg, nbVox);

  //
  // make 'point size' contoller
  //
  auto pointSizeSlider = new QSlider(Qt::Horizontal);
  pointSizeSlider->setRange(1, 20);
  pointSizeSlider->setTickPosition(QSlider::TicksRight);
  pointSizeSlider->setTickInterval(1);
  pointSizeSlider->setPageStep(1);
  connect(pointSizeSlider, &QSlider::valueChanged, this, &Viewer::_updatePointSize);


  _lblCamera = new QLabel();
  auto cbCamera = new QComboBox();
  for (int i = 0; i < _scene->_listView.length(); i++) {
      cbCamera->addItem(QString("Camera " + QString::number(i)));
  }
  connect(cbCamera, static_cast<void(QComboBox::*)(int) >(&QComboBox::currentIndexChanged), [=](const int newValue) {
     _scene->index = newValue;
     _scene->_viewMatrix = _scene->_listView.at(_scene->index);
     _scene->_xRotation = 0.;
     _scene->_yRotation = 0.;
     _scene->_zRotation = 0.;
     _scene->_xTranslate = 0.;
     _scene->_yTranslate = 0.;
     _scene->_zTranslate = 0.;
     _scene->update();
  });

  //
  //make carve button
  //
  auto btnCarve = new QPushButton(tr("Intersect"));
  btnCarve->setMaximumWidth(100);
  connect(btnCarve, &QPushButton::pressed, [=]() {
      _scene->carve();
  });


  //
  // compose control panel
  //
  QWidget* cpWidget = new QWidget();
  QVBoxLayout* controlPanel = new QVBoxLayout();
  cpWidget->setMaximumWidth(300);
  cpWidget->setLayout(controlPanel);
  controlPanel->addWidget(_lblCamera);
  controlPanel->addWidget(pointSizeSlider);
  controlPanel->addSpacing(20);
  controlPanel->addSpacing(20);
  controlPanel->addWidget(cbCamera);
  controlPanel->addSpacing(40);
  controlPanel->addWidget(btnCarve);
  controlPanel->addStretch(2);

  //
  // compose main layout
  //
  QHBoxLayout *mainLayout = new QHBoxLayout;
  mainLayout->addWidget(_scene);
  mainLayout->addWidget(cpWidget);
  setLayout(mainLayout);

  _updatePointSize(1);
}

Viewer::~Viewer()
{
    if(_scene)
    {
        delete _scene;
        _scene = nullptr;
    }
}

void Viewer::keyPressEvent(QKeyEvent* keyEvent) {
  switch ( keyEvent->key() )
  {
    case Qt::Key_Escape:
      QApplication::instance()->quit();  // DEV MODE
      break;

    case Qt::Key_Left:
    case Qt::Key_Q:
      _scene->_xTranslate += 0.01;
      break;

    case Qt::Key_Right:
    case Qt::Key_D:
      _scene->_xTranslate -= 0.01;
      break;

    case Qt::Key_Up:
    case Qt::Key_Z:
      _scene->_yTranslate += 0.01;
      break;

    case Qt::Key_Down:
    case Qt::Key_S:
      _scene->_yTranslate -= 0.01;
      break;

    case Qt::Key_Space:
    case Qt::Key_A:
      _scene->_zTranslate += 0.01;
      break;

    case Qt::Key_C:
    case Qt::Key_E:
      _scene->_zTranslate -= 0.01;
      break;

    default:
      QWidget::keyPressEvent(keyEvent);
  }
  _scene->update();
}


void Viewer::_updatePointSize(int value) {
  _scene->setPointSize(value);
}
