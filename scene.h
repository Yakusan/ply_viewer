#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLTexture>
#include <QMatrix4x4>
#include <QVector>
#include <QVector3D>

#include <camera.h>
#include <vector>


class Scene : public QOpenGLWidget, protected QOpenGLFunctions
{
  Q_OBJECT

public:
  //enum colorAxisMode {COLOR_BY_ROW, COLOR_BY_Z};

  Scene(const QString& plyFilePath, const QString& bundlePath, int hImg, QWidget* parent = 0);
  ~Scene();
  QVector<QMatrix4x4> _listView;
  int index;

public slots:
  void setPointSize(size_t size);

  /*
  void setColorAxisMode(colorAxisMode value);
  void attachCamera(QSharedPointer<Camera> camera);
  void setPickpointEnabled(bool enabled);
  void clearPickedpoints();


signals:
  void pickpointsChanged(const QVector<QVector3D> points);
  */

protected:
  void initializeGL() Q_DECL_OVERRIDE;
  void paintGL() Q_DECL_OVERRIDE;
  void resizeGL(int width, int height) Q_DECL_OVERRIDE;

  virtual void initTextures();

/*
  void mousePressEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
  void mouseMoveEvent(QMouseEvent *event) Q_DECL_OVERRIDE;

private slots:
  void _onCameraChanged(const CameraState& state);
*/

private:
  void _loadPLY(const QString& plyFilePath);
  void _loadBundle(const QString& bundleFilePath);
  void _cleanup();

  /*
  void _drawFrameAxis();
  QVector3D _unproject(int x, int y) const;
  QVector3D _pickPointFrom2D(const QPoint& pos) const;
  void _drawMarkerBox(const QVector3D& point, const QColor& color);
  */

  QMatrix4x4 createPerspectiveMatrix(float fov_v, float aspect, float near, float far);

  float _pointSize;

  /*
  colorAxisMode _colorMode;
  std::vector<std::pair<QVector3D, QColor> > _axesLines;

  QPoint _prevMousePosition;
  */

  QOpenGLShaderProgram * _shaders;
  QOpenGLVertexArrayObject _vao;
  QOpenGLBuffer _vertexBuffer;

  // ajouter photos
  QOpenGLShaderProgram * _shadersPhotos;
  QOpenGLVertexArrayObject _vaoPhotos;
  QOpenGLBuffer _vertexBufferPhotos;
  QOpenGLTexture * _photo;
  float _planVex[16];
  unsigned int _planIndex[6];
  QOpenGLBuffer  * _indexBufferPhotos;

  int                 _hImg;
  QVector<double>     _fov_v;
  QVector<QMatrix4x4> _listProjection;
  QMatrix4x4          _projectionMatrix;
  QMatrix4x4          _viewMatrix;
  QMatrix4x4          _worldMatrix;

  QVector<float> _pointsData;
  size_t _pointsCount;

  /*
  QVector3D _pointsBoundMin;
  QVector3D _pointsBoundMax;
  QVector3D _ray;

  QSharedPointer<Camera> _currentCamera;

  bool _pickpointEnabled;
  QVector<QVector3D> _pickedPoints;
  QVector3D _highlitedPoint;
  */
};
