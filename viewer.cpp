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

#include "scene.h"
#include "viewer.h"


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
  QString maskPath = list[2];
  int hImg = list[3].toInt();

  //
  // make and connect scene widget
  //
  _scene = new Scene(plyPath, bundlePath, maskPath, hImg);

  //
  // make 'point size' contoller
  //
  auto pointSizeSlider = new QSlider(Qt::Horizontal);
  pointSizeSlider->setRange(1, 20);
  pointSizeSlider->setTickPosition(QSlider::TicksRight);
  pointSizeSlider->setTickInterval(1);
  pointSizeSlider->setPageStep(1);
  connect(pointSizeSlider, &QSlider::valueChanged, this, &Viewer::_updatePointSize);

  QLabel *lblPointSize = new QLabel();
  lblPointSize->setText("Points size");

  QWidget* pspWidget = new QWidget();
  QVBoxLayout* pointSizePanel = new QVBoxLayout();
  pspWidget->setMaximumWidth(300);
  pspWidget->setLayout(pointSizePanel);
  pointSizePanel->addWidget(lblPointSize);
  pointSizePanel->addWidget(pointSizeSlider);

  auto voxelSizeSlider = new QSlider(Qt::Horizontal);
  voxelSizeSlider->setRange(1, 128);
  voxelSizeSlider->setSingleStep(1);
  voxelSizeSlider->setValue(32);
  connect(voxelSizeSlider, &QSlider::valueChanged, this, &Viewer::_updateVoxelSize);

  QLabel *lblVoxelSize = new QLabel();
  lblVoxelSize->setText("Voxels size");

  QWidget* vspWidget = new QWidget();
  QVBoxLayout* voxelSizePanel = new QVBoxLayout();
  vspWidget->setMaximumWidth(300);
  vspWidget->setLayout(voxelSizePanel);
  voxelSizePanel->addWidget(lblVoxelSize);
  voxelSizePanel->addWidget(voxelSizeSlider);

  auto cbCamera = new QComboBox();
  for (int i = 0; i < _scene->_listView.length(); i++) {
      cbCamera->addItem(QString("Camera " + QString::number(i)));
  }
  connect(cbCamera, static_cast<void(QComboBox::*)(int) >(&QComboBox::currentIndexChanged), [=](const int newValue) {
     _scene->index = newValue;
     _scene->_currentCamera.setViewMatrix(_scene->_listView.at(_scene->index));
     _scene->update();
  });

  //
  //make carve button
  //
  auto btnIntersect = new QPushButton(tr("Intersect"));
  btnIntersect->setMaximumWidth(100);
  connect(btnIntersect, &QPushButton::pressed, [=]() {
      _scene->intersect();
  });

  auto btnCarve = new QPushButton(tr("Carve"));
  btnCarve->setMaximumWidth(100);
  connect(btnCarve, &QPushButton::pressed, [=]() {
      _scene->carve();
  });

  auto cbDrawPoints = new QCheckBox(tr("Draw point cloud"));
  cbDrawPoints->setMaximumWidth(200);
  cbDrawPoints->setCheckState(Qt::CheckState::Checked);
  connect(cbDrawPoints, &QCheckBox::stateChanged, [=](const int state) {
      _scene->_drawPoints = state;
      _scene->update();
  });

  auto cbDrawSpace = new QCheckBox(tr("Draw Space"));
  cbDrawSpace->setMaximumWidth(200);
  connect(cbDrawSpace, &QCheckBox::stateChanged, [=](const int state) {
      _scene->_drawSpace = state;
      _scene->update();
  });

  auto cbDrawVoxels = new QCheckBox(tr("Draw Voxels"));
  cbDrawVoxels->setMaximumWidth(200);
  connect(cbDrawVoxels, &QCheckBox::stateChanged, [=](const int state) {
      _scene->_drawVoxels = state;
      _scene->update();
  });


  //
  // compose control panel
  //
  QWidget* cpWidget = new QWidget();
  QVBoxLayout* controlPanel = new QVBoxLayout();
  cpWidget->setMaximumWidth(300);
  cpWidget->setLayout(controlPanel);
  controlPanel->addWidget(pspWidget);
  controlPanel->addSpacing(20);
  controlPanel->addWidget(cbCamera);
  controlPanel->addSpacing(30);
  controlPanel->addWidget(cbDrawPoints);
  controlPanel->addSpacing(10);
  controlPanel->addWidget(cbDrawSpace);
  controlPanel->addSpacing(10);
  controlPanel->addWidget(cbDrawVoxels);
  controlPanel->addSpacing(30);
  controlPanel->addWidget(vspWidget);
  controlPanel->addSpacing(30);
  controlPanel->addWidget(btnIntersect);
  controlPanel->addSpacing(10);
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
      _scene->_currentCamera.setXTranslation(-0.01);
      break;

    case Qt::Key_Right:
    case Qt::Key_D:
      _scene->_currentCamera.setXTranslation(0.01);
      break;

    case Qt::Key_Up:
    case Qt::Key_Z:
      _scene->_currentCamera.setZTranslation(-0.01);
      break;

    case Qt::Key_Down:
    case Qt::Key_S:;
      _scene->_currentCamera.setZTranslation(0.01);
      break;

    case Qt::Key_Space:
    case Qt::Key_A:
      _scene->_currentCamera.setYTranslation(0.01);
      break;

    case Qt::Key_C:
    case Qt::Key_E:
      _scene->_currentCamera.setYTranslation(-0.01);
      break;

    default:
      QWidget::keyPressEvent(keyEvent);
  }
  _scene->_currentCamera.updateView();
  _scene->update();
}


void Viewer::_updatePointSize(int value) {
  _scene->setPointSize(value);
}

void Viewer::_updateVoxelSize(int value) {
  _scene->setVoxelSize(value);
}
