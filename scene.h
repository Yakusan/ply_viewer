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

#include <unistd.h>
#include <vector>

#include "camera.h"

class Scene : public QOpenGLWidget, protected QOpenGLFunctions
{
  Q_OBJECT

public:
  Scene(const QString& plyFilePath, const QString& bundlePath, QString& maskPath, int hImg, QWidget* parent = 0);
  ~Scene();
  QVector<QMatrix4x4> _listView;
  Camera              _currentCamera; // Peut bouger
  int index;
  bool _drawPoints = true;
  bool _drawSpace = false;
  bool _drawVoxels = false;

public slots:
  void setPointSize(size_t size);
  void setVoxelSize(int nb);
  void intersect();
  void carve();

signals:
  void pickpointsChanged(const QVector<QVector3D> points);


protected:
  void initializeGL() Q_DECL_OVERRIDE;
  void paintGL() Q_DECL_OVERRIDE;
  void resizeGL(int width, int height) Q_DECL_OVERRIDE;

  void mouseMoveEvent(QMouseEvent *event) Q_DECL_OVERRIDE;


private slots:

private:
  void _loadPLY(const QString& plyFilePath);
  void _loadBundle(const QString& bundleFilePath);
  void _createVox();
  void _cleanup();
  QMatrix4x4 createPerspectiveMatrix(float fov_v, float aspect, float near, float far);

  void rotate(int dx, int dy, int dz);
  void setXRotation(int angle);
  void setYRotation(int angle);
  void setZRotation(int angle);
  void carveView(int v);

  QVector2D project(QVector4D v);

  float _pointSize;

  QPoint _prevMousePosition;
  QOpenGLVertexArrayObject _vaoPoints;
  QOpenGLBuffer _vertexBufferPoints;
  QScopedPointer<QOpenGLShaderProgram> _shadersPoints;

  QOpenGLVertexArrayObject _vaoVox;
  QOpenGLBuffer _vertexBufferVox;
  QOpenGLBuffer *_indicesBufferVox;
  QScopedPointer<QOpenGLShaderProgram> _shadersVox;

  QOpenGLVertexArrayObject _vaoSpace;
  QOpenGLBuffer _vertexBufferSpace;

  int                 _nbVox = 32;
  float               _spaceSize;
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
  QVector<float>          _spaceVertices;
  QVector<unsigned int>   _voxIndices;

  QString _maskPath;
};
