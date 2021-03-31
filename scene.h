#pragma once

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QMatrix4x4>
#include <QMatrix3x3>
#include <QVector3D>
#include <QSharedPointer>
#include <Qt3DExtras/Qt3DExtras>
#include <Qt3DCore/Qt3DCore>

#include <camera.h>
#include <vector>


class Scene : public QOpenGLWidget, protected QOpenGLFunctions
{
  Q_OBJECT

public:
  enum colorAxisMode {COLOR_BY_ROW, COLOR_BY_Z};

  Scene(const QString& plyFilePath, const QString& bundlePath, int hImg, int nbVox, QWidget* parent = 0);
  ~Scene();
  QVector<QMatrix4x4> _listView;
  QMatrix4x4          _viewMatrix;
  int index;
  float _xTranslate = 0.;
  float _yTranslate = 0.;
  float _zTranslate = 0.;
  int _xRotation = 0;
  int _yRotation = 0;
  int _zRotation = 0;

public slots:
  void setPointSize(size_t size);
  void setColorAxisMode(colorAxisMode value);
  void attachCamera(QSharedPointer<Camera> camera);
  void setPickpointEnabled(bool enabled);
  void clearPickedpoints();
  void carve();
  void setxVoxT(int x);
  void setyVoxT(int y);
  void setzVoxT(int z);
  void setVoxSize(int s);

signals:
  void pickpointsChanged(const QVector<QVector3D> points);


protected:
  void initializeGL() Q_DECL_OVERRIDE;
  void paintGL() Q_DECL_OVERRIDE;
  void resizeGL(int width, int height) Q_DECL_OVERRIDE;

  void mousePressEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
  void mouseMoveEvent(QMouseEvent *event) Q_DECL_OVERRIDE;


private slots:
  void _onCameraChanged(const CameraState& state);

private:
  void _loadPLY(const QString& plyFilePath);
  void _loadBundle(const QString& bundleFilePath);
  void _createVox();
  void _cleanup();
  QVector3D _unproject(int x, int y) const;
  QVector3D _pickPointFrom2D(const QPoint& pos) const;
  void _drawMarkerBox(const QVector3D& point, const QColor& color);
  QMatrix4x4 createPerspectiveMatrix(float fov_v, float aspect, float near, float far);

  void rotate(int dx, int dy, int dz);
  void setXRotation(int angle);
  void setYRotation(int angle);
  void setZRotation(int angle);

  float _pointSize;
  colorAxisMode _colorMode;
  std::vector<std::pair<QVector3D, QColor> > _axesLines;

  QPoint _prevMousePosition;
  QOpenGLVertexArrayObject _vaoPoints;
  QOpenGLBuffer _vertexBufferPoints;
  QScopedPointer<QOpenGLShaderProgram> _shadersPoints;

  QOpenGLVertexArrayObject _vaoVox;
  QOpenGLBuffer _vertexBufferVox;
  QOpenGLBuffer *_indicesBufferVox;
  QScopedPointer<QOpenGLShaderProgram> _shadersVox;

  int                 _nbVox;
  float               _voxSize;
  int                 _hImg;
  unsigned char       *_voxStorage;
  QVector<double>     _fov_v;
  QVector<QMatrix4x4> _listProjection;
  QMatrix4x4          _projectionMatrix;
  QMatrix4x4          _worldMatrix;

  QVector<float> _pointsData;
  size_t         _pointsCount;
  QVector3D      _pointsBoundMin;
  QVector3D      _pointsBoundMax;
  QVector3D      _ray;

  QVector<float>          _voxVertices;
  QVector<unsigned int>   _voxIndices;

  QSharedPointer<Camera> _currentCamera;

  bool _pickpointEnabled;
  QVector<QVector3D> _pickedPoints;
  QVector3D _highlitedPoint;

  float _xVoxT = 0.;
  float _yVoxT = 0.;
  float _zVoxT = 0.;
};
