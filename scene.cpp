#include "scene.h"

#include <QMouseEvent>
#include <QOpenGLShaderProgram>
#include <QCoreApplication>
#include <QScopedPointer>

#include <cmath>
#include <cassert>
#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <sstream>
#include <limits>

const size_t POINT_STRIDE =  7; // x, y, z, index, r, g, b

Scene::Scene(const QString& plyFilePath, const QString& bundlePath, int hImg, int nbVox, QWidget* parent)
  : QOpenGLWidget(parent),
    _pointSize(1),
    _colorMode(COLOR_BY_Z)
{
  _pickpointEnabled = false;

  _hImg = hImg;
  if (nbVox < 1)
      _nbVox = 32;
  else
      _nbVox = nbVox;

  _voxSize = 0.02;

  _loadPLY(plyFilePath);
  _loadBundle(bundlePath);
  _voxStorage = new unsigned char[_nbVox*_nbVox*_nbVox];
  memset(_voxStorage, 1, _nbVox*_nbVox*_nbVox * sizeof(unsigned char));
  index = 0;
  _viewMatrix = _listView.at(0);
  _projectionMatrix = _listProjection.at(0);
  _indicesBufferVox = new QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
  _createVox();

  setMouseTracking(true);

  // make trivial axes cross
  _axesLines.push_back(std::make_pair(QVector3D(0.0, 0.0, 0.0), QColor(1.0, 0.0, 0.0)));
  _axesLines.push_back(std::make_pair(QVector3D(1.0, 0.0, 0.0), QColor(1.0, 0.0, 0.0)));
  _axesLines.push_back(std::make_pair(QVector3D(0.0, 0.0, 0.0), QColor(0.0, 1.0, 0.0)));
  _axesLines.push_back(std::make_pair(QVector3D(0.0, 1.0, 0.0), QColor(0.0, 1.0, 0.0)));
  _axesLines.push_back(std::make_pair(QVector3D(0.0, 0.0, 0.0), QColor(0.0, 0.0, 1.0)));
  _axesLines.push_back(std::make_pair(QVector3D(0.0, 0.0, 1.0), QColor(0.0, 0.0, 1.0)));

}


void Scene::_loadPLY(const QString& plyFilePath) {

  // open stream
  std::fstream is;
  is.open(plyFilePath.toStdString().c_str(), std::fstream::in);

  // ensure format with magic header
  std::string line;
  std::getline(is, line);
  if (line != "ply") {
    throw std::runtime_error("not a ply file");
  }

  // parse header looking only for 'element vertex' section size
  _pointsCount = 0;
  while (is.good()) {
    std::getline(is, line);
    if (line == "end_header") {
      break;
    } else {
      std::stringstream ss(line);
      std::string tag1, tag2, tag3;
      ss >> tag1 >> tag2 >> tag3;
      if (tag1 == "element" && tag2 == "vertex") {
        _pointsCount = std::atof(tag3.c_str());
      }
    }
  }

  // read and parse 'element vertex' section
  if (_pointsCount > 0) {
    _pointsData.resize(_pointsCount * POINT_STRIDE);

    std::stringstream ss;
    float *p = _pointsData.data();
    for (size_t i = 0; is.good() && i < _pointsCount; ++i) {
      float x, y, z, nx, ny, nz, r, g, b;
      std::getline(is, line);
      ss.str(line);

      ss >> x >> y >> z >> nx >> ny >> nz >> r >> g >> b;

      *p++ = x;
      *p++ = y;
      *p++ = z;
      *p++ = i;
      *p++ = r/255.;
      *p++ = g/255.;
      *p++ = b/255.;

      ss.clear();

      // update bounds
      _pointsBoundMax[0] = std::max(x, _pointsBoundMax[0]);
      _pointsBoundMax[1] = std::max(y, _pointsBoundMax[1]);
      _pointsBoundMax[2] = std::max(z, _pointsBoundMax[2]);
      _pointsBoundMin[0] = std::min(x, _pointsBoundMin[0]);
      _pointsBoundMin[1] = std::min(y, _pointsBoundMin[1]);
      _pointsBoundMin[2] = std::min(z, _pointsBoundMin[2]);

    }

    // check if we've got exact number of points mentioned in header
    if (p - _pointsData.data() < _pointsData.size()) {
      throw std::runtime_error("broken ply file");
    }
  }
}


void Scene::_loadBundle(const QString& bundleFilePath)
{
    const float w = width();
    const float h = height();

    int nbCam;
    // open stream
    std::fstream is;
    is.open(bundleFilePath.toStdString().c_str(), std::fstream::in);

    // ensure format with magic header
    std::string line;
    std::getline(is, line);

    if (line != "# Bundle file v0.3") {
        throw std::runtime_error("not a bundle file");
    }

    std::getline(is, line);
    std::stringstream ss(line);
    ss >> nbCam;

    float tmp[3];
    float rt[16];


    for (int i = 0; i < nbCam ; i++) {
        ss.clear();
        std::memset(rt, 0, sizeof(rt));

        std::getline(is, line);
        ss.str(line);

        ss >> tmp[0];

        // cf: http://paulbourke.net/miscellaneous/lens/
        _fov_v.append(2. * std::atan(_hImg * 0.5 / tmp[0]));

        ss.clear();
        for (int j = 0; j < 3; j++) {
            std::getline(is, line);
            ss.str(line);

            ss >> tmp[0] >> tmp[1] >> tmp[2];
            rt[j * 4    ] = tmp[0];
            rt[j * 4 + 1] = tmp[1];
            rt[j * 4 + 2] = tmp[2];

           ss.clear();
        }
        rt[15] = 1.;

        std::getline(is, line);
        ss.str(line);
        ss >> tmp[0] >> tmp[1] >> tmp[2];

        rt[3]  = tmp[0];
        rt[7]  = tmp[1];
        rt[11] = tmp[2];

        QMatrix4x4 RT(rt);

        _listView.append(RT);
        _listProjection.append(createPerspectiveMatrix(_fov_v.at(i), h / w, 0.01f, 100.0f));
    }
}

void Scene::_createVox() {
    _voxVertices = {
        0.0f,_voxSize,0.0f,   //Point A 0
        0.0f,_voxSize,_voxSize,    //Point B 1
        _voxSize,_voxSize,0.0f,    //Point C 2
        _voxSize,_voxSize,_voxSize,     //Point D 3

        0.0f,0.0f,0.0f,  //Point E 4
        0.0f,0.0f,_voxSize,   //Point F 5
        _voxSize,0.0f,0.0f,   //Point G 6
        _voxSize,0.0f,_voxSize,    //Point H 7
    };

    _voxIndices = {
        /*Above ABC,BCD*/
        0,1,2,
        1,3,2,
        /*Following EFG,FGH*/
        4,5,6,
        5,7,6,
        /*Left ABF,AEF*/
        4,0,6,
        0,2,6,
        /*Right side CDH,CGH*/
        5,7,1,
        7,3,1,
        /*ACG,AEG*/
        4,5,0,
        5,1,0,
        /*Behind BFH,BDH*/
        7,6,3,
        6,2,3
    };
}

// cf: http://www.songho.ca/opengl/gl_projectionmatrix.html
QMatrix4x4 Scene::createPerspectiveMatrix(float fov_v, float aspect, float near, float far)
{
    const float yScale = 1.0f / tan(fov_v / 2.0f);
    const float xScale = yScale * aspect;
    const float fmn    = (far - near);

    float k[16];

    memset(k, 0, sizeof(k));

    k[0]  = xScale;
    k[5]  = yScale;
    k[10] = -(far + near) / fmn;
    k[11] = -2.0f * far * near / fmn;
    k[14] = -1.0f;

    return QMatrix4x4(k);
}

Scene::~Scene()
{
  _cleanup();
}


void Scene::_cleanup()
{
  if(_shadersPoints.isNull())
    return;

  makeCurrent();
  _vertexBufferPoints.destroy();
  _shadersPoints.reset();
  delete _indicesBufferVox;
  delete [] _voxStorage;
  doneCurrent();
}


void Scene::initializeGL()
{
  connect(context(), &QOpenGLContext::aboutToBeDestroyed, this, &Scene::_cleanup);

  initializeOpenGLFunctions();
  glClearColor(0, 0, 0, 1.0);

  // the world is still for now
  _worldMatrix.setToIdentity();

  //
  // create points shaders and map attributes
  //
  _shadersPoints.reset(new QOpenGLShaderProgram());
  auto vsPointsLoaded = _shadersPoints->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/vertex_shader_points.glsl");
  auto fsPointsLoaded = _shadersPoints->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/fragment_shader_points.glsl");
  assert(vsPointsLoaded && fsPointsLoaded);
  // vector attributes
  _shadersPoints->bindAttributeLocation("vertex", 0);
  _shadersPoints->bindAttributeLocation("pointRowIndex", 1);
  _shadersPoints->bindAttributeLocation("color", 2);
  // constants
  _shadersPoints->bind();
  _shadersPoints->setUniformValue("lightPos", QVector3D(0, 0, 50));
  _shadersPoints->setUniformValue("pointsCount", static_cast<GLfloat>(_pointsCount));
  _shadersPoints->link();
  _shadersPoints->release();

  // create array container and load points into buffer
  _vaoPoints.create();
  QOpenGLVertexArrayObject::Binder vaoPointsBinder(&_vaoPoints);
  _vertexBufferPoints.create();
  _vertexBufferPoints.bind();
  _vertexBufferPoints.allocate(_pointsData.constData(), _pointsData.size() * sizeof(GLfloat));
  QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();
  f->glEnableVertexAttribArray(0);
  f->glEnableVertexAttribArray(1);
  f->glEnableVertexAttribArray(2);
  GLintptr vertex_offset = 0 * sizeof(float);
  GLintptr pointRowIndex_offset = 3 * sizeof(float);
  GLintptr color_offset = 4 * sizeof(float);
  f->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7*sizeof(GLfloat), (GLvoid*)vertex_offset);
  f->glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 7*sizeof(GLfloat), (GLvoid*)pointRowIndex_offset);
  f->glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 7*sizeof(GLfloat), (GLvoid*)color_offset);
  _vertexBufferPoints.release();

  //
  // create voxels shaders
  //
  _shadersVox.reset(new QOpenGLShaderProgram());
  auto vsVoxLoaded = _shadersVox->addCacheableShaderFromSourceFile(QOpenGLShader::Vertex, ":/vertex_shader_vox.glsl");
  auto fsVoxLoaded = _shadersVox->addCacheableShaderFromSourceFile(QOpenGLShader::Fragment, ":/fragment_shader_vox.glsl");
  assert(vsVoxLoaded && fsVoxLoaded);
  // vector attributes
  _shadersVox->bindAttributeLocation("vertex", 0);
  // constants
  _shadersVox->bind();
  _shadersVox->link();
  _shadersVox->release();

  // create array container and load voxels into buffer
  _vaoVox.create();
  _vaoVox.bind();
  _vertexBufferVox.create();
  _vertexBufferVox.bind();
  _vertexBufferVox.allocate(_voxVertices.constData(), _voxVertices.size() * sizeof(GLfloat));
  QOpenGLFunctions *g = QOpenGLContext::currentContext()->functions();
  g->glEnableVertexAttribArray(0);
  g->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);

  _indicesBufferVox->create();
  _indicesBufferVox->bind();
  _indicesBufferVox->allocate(_voxIndices.constData(), _voxIndices.size() * sizeof(GLuint));
  _vaoVox.release();
}

void Scene::paintGL()
{
  // ensure GL flags
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_VERTEX_PROGRAM_POINT_SIZE); //required for gl_PointSize

  //
  // set camera
  //
  const CameraState camera = _currentCamera->state();
  QMatrix4x4 tmp(_viewMatrix);
  tmp.rotate((GLfloat)_xRotation, 1, 0, 0);
  tmp.rotate((GLfloat)_yRotation, 0, 1, 0);
  tmp.rotate((GLfloat)_zRotation, 0, 0, 1);
  tmp.translate(QVector3D(_xTranslate, _yTranslate, _zTranslate));
  _projectionMatrix = _listProjection.at(index);


  // set clipping planes
  glEnable(GL_CLIP_PLANE1);
  glEnable(GL_CLIP_PLANE2);
  const double rearClippingPlane[] = {0., 0., -1., camera.rearClippingDistance};
  glClipPlane(GL_CLIP_PLANE1 , rearClippingPlane);
  const double frontClippingPlane[] = {0., 0., 1., camera.frontClippingDistance};
  glClipPlane(GL_CLIP_PLANE2 , frontClippingPlane);

  const auto viewMatrix = _projectionMatrix * tmp * _worldMatrix;

  //
  // draw points cloud
  //
  _vaoPoints.bind();
  _shadersPoints->bind();
  _shadersPoints->setUniformValue("pointsCount", static_cast<GLfloat>(_pointsCount));
  _shadersPoints->setUniformValue("mvpMatrix", viewMatrix);
  _shadersPoints->setUniformValue("pointSize", _pointSize);
  _shadersPoints->setUniformValue("colorAxisMode", static_cast<GLfloat>(_colorMode));
  _shadersPoints->setUniformValue("color", QVector3D(1.,0.,0.));
  _shadersPoints->setUniformValue("pointsBoundMin", _pointsBoundMin);
  _shadersPoints->setUniformValue("pointsBoundMax", _pointsBoundMax);
  glDrawArrays(GL_POINTS, 0, _pointsData.size());
  _shadersPoints->release();
  _vaoPoints.release();

  //
  // draw voxels
  //
  _vaoVox.bind();
  _shadersVox->bind();
  _shadersVox->setUniformValue("mvpMatrix", viewMatrix);
  for ( int i = 0; i < _nbVox; i++){
      for (int j = 0; j < _nbVox; j++) {
          for (int k = 0; k < _nbVox; k++) {
              if (_voxStorage[i*_nbVox*_nbVox + j*_nbVox + k]) {
                  _shadersVox->setUniformValue("xt", _xVoxT+i*_voxSize);
                  _shadersVox->setUniformValue("yt", _yVoxT+j*_voxSize);
                  _shadersVox->setUniformValue("zt", _zVoxT+k*_voxSize);
                  glDrawElements(GL_TRIANGLES, 12*3, GL_UNSIGNED_INT, (GLvoid*)0);
              }
          }
      }
  }
  _shadersVox->release();
  _vaoVox.release();

  //
  // draw picked points and line between
  //
  glBegin(GL_LINES);
  glColor3f(1., 1., 0.);
  {
    QMatrix4x4 mvMatrix = tmp * _worldMatrix;
    for (auto vertex : _pickedPoints) {
      const auto translated = _projectionMatrix * mvMatrix * vertex;
      glVertex3f(translated.x(), translated.y(), translated.z());
    }
  }
  glEnd();
  for (auto vertex : _pickedPoints) {
    _drawMarkerBox(vertex, QColor(1., 1., 0.));
  }

  //
  // draw mouse-highlited point
  //
  if (_highlitedPoint != QVector3D()) {
    _drawMarkerBox(_highlitedPoint, QColor(0., 1., 1.));
  }

}


void Scene::_drawMarkerBox(const QVector3D& point, const QColor& color) {
  glBegin(GL_LINE_LOOP);
  glColor3f(color.red(), color.green(), color.blue());
  {
    const float dx = 0.01;
    const auto aspect = (float)width() / height();
    const auto dy = dx*aspect;
    const auto p = _projectionMatrix * _viewMatrix * _worldMatrix * point;
    glVertex3f(p.x()-dx, p.y()-dy, p.z());
    glVertex3f(p.x()+dx, p.y()-dy, p.z());
    glVertex3f(p.x()+dx, p.y()+dy, p.z());
    glVertex3f(p.x()-dx, p.y()+dy, p.z());
  }
  glEnd();
}


void Scene::resizeGL(int w, int h)
{
    for(int i = 0 ; i < _listProjection.length() ; ++i)
        _listProjection[i] = createPerspectiveMatrix(_fov_v.at(i), float(h) / float(w), 0.01f, 100.0f);
}


QVector3D Scene::_pickPointFrom2D(const QPoint& pos) const {

  // do slow linear search through small sample
  // must have is BSP/quadtree for O(logN) searching on large samples
  float maxDistance = 1e-1;
  auto ray = _unproject(pos.x(), pos.y());
  QVector3D closest;
  for (size_t i = 0; i < _pointsCount; i++) {
    const GLfloat *p = &_pointsData[i*4];
    QVector3D point(p[0], p[1], p[2]);

    float distance = (point - ray).length();
    if (distance < maxDistance) {
      closest = point;
      maxDistance = distance;
    }
  }
  return closest;
}


void Scene::mousePressEvent(QMouseEvent *event)
{
  _prevMousePosition = event->pos();

  if (event->button() == Qt::LeftButton && _pickpointEnabled)
  {
    const QVector3D closest = _pickPointFrom2D(event->pos());
    if (closest != QVector3D()) {
      if (_pickedPoints.size() == 2) {
        // clear previous pair
        _pickedPoints.clear();
      }

      _pickedPoints << closest;
      emit pickpointsChanged(_pickedPoints);
      update();
    }
  }
}


void Scene::mouseMoveEvent(QMouseEvent *event)
{
  const int dx = event->x() - _prevMousePosition.x();
  const int dy = event->y() - _prevMousePosition.y();
  const bool panningMode = (event->modifiers() & Qt::ShiftModifier);
  _prevMousePosition = event->pos();

  if (event->buttons() & Qt::LeftButton) {

    if (panningMode) {
      if (dx > 0) {
        _currentCamera->right();
      }
      if (dx < 0) {
        _currentCamera->left();
      }
      if (dy > 0) {
        _currentCamera->down();
      }
      if (dy < 0) {
        _currentCamera->up();
      }
    } else {
      Scene::rotate(dy*0.5, dx*0.5, 0);
    }
  }

  if (_pickpointEnabled) {
    _highlitedPoint = _pickPointFrom2D(event->pos());
  }
  update();
}


void Scene::setPointSize(size_t size) {
  assert(size > 0);
  _pointSize = size;
  update();
}


void Scene::setColorAxisMode(colorAxisMode value) {
  _colorMode = value;
  update();
}


void Scene::attachCamera(QSharedPointer<Camera> camera) {
  if (_currentCamera) {
    disconnect(_currentCamera.data(), &Camera::changed, this, &Scene::_onCameraChanged);
  }
  _currentCamera = camera;
  connect(camera.data(), &Camera::changed, this, &Scene::_onCameraChanged);
}


void Scene::_onCameraChanged(const CameraState&) {
  update();
}


void Scene::setPickpointEnabled(bool enabled) {
  _pickpointEnabled = enabled;
  if (!enabled) {
    _highlitedPoint = QVector3D();
    update();
  }
}


void Scene::clearPickedpoints() {
  _pickedPoints.clear();
  emit pickpointsChanged(_pickedPoints);
  update();
}


QVector3D Scene::_unproject(int x, int y) const {
  // with Qt5.5 we can make use of new QVector3D::unproject()

  const QMatrix4x4 mvMatrix = _projectionMatrix * _viewMatrix * _worldMatrix;
  const QMatrix4x4 inverted = mvMatrix.inverted();

  // normalized device coordinates
  double ndcX = 2*(double)x / width() - 1;
  double ndcY = 2*(double)(height() - y) / height() - 1;
  QVector4D nearPoint4 = inverted * QVector4D(ndcX, ndcY, 1, 1);
  QVector4D farPoint4 = inverted * QVector4D(ndcX, ndcY, -1, 1);
  if (nearPoint4.w() == 0.0) {
    return QVector3D();
  }

  double w = 1.0/nearPoint4.w();
  QVector3D nearPoint = QVector3D(nearPoint4);
  nearPoint *= w;

  w = 1.0/farPoint4.w();
  QVector3D farPoint = QVector3D(farPoint4);
  farPoint *= w;

  QVector3D direction = farPoint - nearPoint;
  if (direction.z() == 0.0) {
    return QVector3D();
  }

  double t = -nearPoint.z() / direction.z();
  return nearPoint + direction * t;
}

void Scene::setXRotation(int angle)
{
  angle = angle % 360;
  if (angle != _xRotation) {
    _xRotation = angle;
  }
}


void Scene::setYRotation(int angle)
{
  angle = angle % 360;
  if (angle != _yRotation) {
    _yRotation = angle;
  }
}


void Scene::setZRotation(int angle)
{
  angle = angle % 360;
  if (angle != _zRotation) {
    _zRotation = angle;
  }
}


void Scene::rotate(int dx, int dy, int dz) {
  setXRotation(_xRotation - dx);
  setYRotation(_yRotation + dy);
  setZRotation(_zRotation - dz);
}

void Scene::setxVoxT(int x) {
    float fx = (float)(x) / 100.;
    if (fx != _xVoxT) {
        _xVoxT = fx;
    }
    update();
}

void Scene::setyVoxT(int y) {
    float fy = (float)(y) / 100.;
    if (fy != _yVoxT) {
        _yVoxT = fy;
    }
    update();
}

void Scene::setzVoxT(int z) {
    float fz = (float)(z) / 100.;
    if (fz != _zVoxT) {
        _zVoxT = fz;
    }
    update();
}

void Scene::setVoxSize(int s) {
    float fs = (float)(s) / 1000.;
    if (fs != _voxSize) {
        _voxSize = fs;
    }
    _createVox();
    update();
}

void Scene::carve() {
#pragma omp parallel for collapse(3)
    for (int i = 0; i < _nbVox; ++i) {
        for (int j = 0; j < _nbVox; ++j) {
            for (int k = 0; k < _nbVox; ++k) {
                _voxStorage[i*_nbVox*_nbVox + j*_nbVox + k] = 0;
                float x = _xVoxT+i*_voxSize;
                float y = _yVoxT+j*_voxSize;
                float z = _zVoxT+k*_voxSize;
                const float *p = _pointsData.constData();
                for (size_t l = 0; l < _pointsCount * 7; l += 7) {
                    if (p[l] > x && p[l] < x + _voxSize && p[l+1] > y && p[l+1] < y + _voxSize && p[l+2] > z && p[l+2] < z + _voxSize) {
                        _voxStorage[i*_nbVox*_nbVox + j*_nbVox + k] = 1;
                        break;
                    }
                }
            }
        }
    }
    update();
}
