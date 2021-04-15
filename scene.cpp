#include "scene.h"

#include <QMouseEvent>
#include <cmath>
#include <iostream>
#include <fstream>
#include <cstring>
#include <sstream>
#include <cassert>
#include <omp.h>

const size_t POINT_STRIDE =  7; // x, y, z, index, r, g, b

unsigned char *tmp;

Scene::Scene(const QString& plyFilePath, const QString& bundlePath, QString& maskPath, int hImg, QWidget* parent)
  : QOpenGLWidget(parent),
    _pointSize(1),
    _fov_v()
{
  _hImg = hImg;
  _maskPath = maskPath;
  _loadPLY(plyFilePath);
  _loadBundle(bundlePath);
  _voxStorage = new unsigned char[_nbVox*_nbVox*_nbVox];
  memset(_voxStorage, 1, _nbVox*_nbVox*_nbVox * sizeof(unsigned char));
  index = 0;
  _currentCamera.setViewMatrix(_listView.at(0));
  _projectionMatrix = _listProjection.at(0);
  _indicesBufferVox = new QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
  _spaceSize = qMax(qMax(_pointsBoundMax[0] - _pointsBoundMin[0],_pointsBoundMax[1] - _pointsBoundMin[1]), _pointsBoundMax[2] - _pointsBoundMin[2]);
  _createVox();


  tmp = new unsigned char[_nbVox*_nbVox*_nbVox];

  setMouseTracking(true);
}


void Scene::_loadPLY(const QString& plyFilePath) {

  _pointsBoundMax[0] = std::numeric_limits<float>::min();
  _pointsBoundMax[1] = std::numeric_limits<float>::min();
  _pointsBoundMax[2] = std::numeric_limits<float>::min();
  _pointsBoundMin[0] = std::numeric_limits<float>::max();
  _pointsBoundMin[1] = std::numeric_limits<float>::max();
  _pointsBoundMin[2] = std::numeric_limits<float>::max();

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

    _fov_v.resize(nbCam);
    for (int i = 0; i < nbCam ; i++) {
        ss.clear();
        std::memset(rt, 0, sizeof(rt));

        std::getline(is, line);
        ss.str(line);

        ss >> tmp[0];

        // cf: http://paulbourke.net/miscellaneous/lens/
        _fov_v[i] = 2. * std::atan(_hImg * 0.5 / tmp[0]);

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
    float voxSize = _spaceSize/_nbVox;
    _voxVertices = {
        0.0f,voxSize,0.0f,   //Point A 0
        0.0f,voxSize,voxSize,    //Point B 1
        voxSize,voxSize,0.0f,    //Point C 2
        voxSize,voxSize,voxSize,     //Point D 3

        0.0f,0.0f,0.0f,  //Point E 4
        0.0f,0.0f,voxSize,   //Point F 5
        voxSize,0.0f,0.0f,   //Point G 6
        voxSize,0.0f,voxSize,    //Point H 7
    };

    _spaceVertices = {
        0.0f,_spaceSize,0.0f,   //Point A 0
        0.0f,_spaceSize,_spaceSize,    //Point B 1
        _spaceSize,_spaceSize,0.0f,    //Point C 2
        _spaceSize,_spaceSize,_spaceSize,     //Point D 3

        0.0f,0.0f,0.0f,  //Point E 4
        0.0f,0.0f,_spaceSize,   //Point F 5
        _spaceSize,0.0f,0.0f,   //Point G 6
        _spaceSize,0.0f,_spaceSize,    //Point H 7
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
  delete [] tmp;
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
  _vaoPoints.bind();
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
  _vaoPoints.release();

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

  //
  // create array container and load voxels into buffer
  //
  _vaoSpace.create();
  _vaoSpace.bind();
  _vertexBufferSpace.create();
  _vertexBufferSpace.bind();
  _vertexBufferSpace.allocate(_spaceVertices.constData(), _spaceVertices.size() * sizeof(GLfloat));
  QOpenGLFunctions *h = QOpenGLContext::currentContext()->functions();
  h->glEnableVertexAttribArray(0);
  h->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);

  _indicesBufferVox->bind();
  _indicesBufferVox->allocate(_voxIndices.constData(), _voxIndices.size() * sizeof(GLuint));
  _vaoSpace.release();
}

void Scene::paintGL()
{
  // ensure GL flags
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);
  glEnable(GL_VERTEX_PROGRAM_POINT_SIZE); //required for gl_PointSize

  //
  // set camera
  //
  const auto viewMatrix = _projectionMatrix *  _currentCamera.viewMatrix() * _worldMatrix;

  //
  // draw points cloud
  //
  if (_drawPoints){
      _vaoPoints.bind();
      _shadersPoints->bind();
      _shadersPoints->setUniformValue("pointsCount", static_cast<GLfloat>(_pointsCount));
      _shadersPoints->setUniformValue("mvpMatrix", viewMatrix);
      _shadersPoints->setUniformValue("pointSize", _pointSize);
      glDrawArrays(GL_POINTS, 0, _pointsData.size());
      _shadersPoints->release();
      _vaoPoints.release();
  }

    //
    // draw voxels
    //
  if(_drawVoxels) {


      _vaoVox.bind();
      _shadersVox->bind();
      _shadersVox->setUniformValue("mvpMatrix", viewMatrix);
      float voxSize = _spaceSize/_nbVox;
      for ( int i = 0; i < _nbVox; i++){
          for (int j = 0; j < _nbVox; j++) {
              for (int k = 0; k < _nbVox; k++) {
                  if (_voxStorage[i*_nbVox*_nbVox + j*_nbVox + k]) {
                      _shadersVox->setUniformValue("xt", _pointsBoundMin[0]+i*voxSize);
                      _shadersVox->setUniformValue("yt", _pointsBoundMin[1]+j*voxSize);
                      _shadersVox->setUniformValue("zt", _pointsBoundMin[2]+k*voxSize);
                      _shadersVox->setUniformValue("flag", 1);
                      glDrawElements(GL_TRIANGLES, 12*3, GL_UNSIGNED_INT, (GLvoid*)0);
                  }
              }
          }
      }
      _shadersVox->release();
      _vaoVox.release();

      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
      _vaoVox.bind();
      _shadersVox->bind();
      _shadersVox->setUniformValue("mvpMatrix", viewMatrix);
      for ( int i = 0; i < _nbVox; i++){
          for (int j = 0; j < _nbVox; j++) {
              for (int k = 0; k < _nbVox; k++) {
                  if (_voxStorage[i*_nbVox*_nbVox + j*_nbVox + k]) {
                      _shadersVox->setUniformValue("xt", _pointsBoundMin[0]+i*voxSize);
                      _shadersVox->setUniformValue("yt", _pointsBoundMin[1]+j*voxSize);
                      _shadersVox->setUniformValue("zt", _pointsBoundMin[2]+k*voxSize);
                      _shadersVox->setUniformValue("flag", 0);
                      glDrawElements(GL_TRIANGLES, 12*3, GL_UNSIGNED_INT, (GLvoid*)0);
                  }
              }
          }
      }
      _shadersVox->release();
      _vaoVox.release();
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

  }

  //
  // draw voxels space
  //

  if(_drawSpace){
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

      _vaoSpace.bind();
      _shadersVox->bind();
      _shadersVox->setUniformValue("mvpMatrix", viewMatrix);
      _shadersVox->setUniformValue("xt", _pointsBoundMin[0]);
      _shadersVox->setUniformValue("yt", _pointsBoundMin[1]);
      _shadersVox->setUniformValue("zt", _pointsBoundMin[2]);
      _shadersVox->setUniformValue("flag", 2);
      glDrawElements(GL_TRIANGLES, 12*3, GL_UNSIGNED_INT, (GLvoid*)0);
      _shadersVox->release();
      _vaoSpace.release();

      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  }

}

void Scene::resizeGL(int w, int h)
{
    for(int i = 0 ; i < _listProjection.length() ; ++i)
        _listProjection[i] = createPerspectiveMatrix(_fov_v.at(i), float(h) / float(w), 0.01f, 100.0f);
}

void Scene::mouseMoveEvent(QMouseEvent *event)
{
  const int dx = event->x() - _prevMousePosition.x();
  const int dy = event->y() - _prevMousePosition.y();
  _prevMousePosition = event->pos();


  if (event->buttons() & Qt::LeftButton)
  {
      if (dx != 0)
        _currentCamera.setYRotation(dx);

      if (dy != 0 )
        _currentCamera.setXRotation(dy);

      _currentCamera.updateView();
    }
  update();
}

void Scene::setPointSize(size_t size) {
  assert(size > 0);
  _pointSize = size;
  update();
}

void Scene::setVoxelSize(int nb) {
  _nbVox = nb;
  delete [] _voxStorage;
  delete [] tmp;
  _voxStorage = new unsigned char[_nbVox*_nbVox*_nbVox];
  tmp =  new unsigned char[_nbVox*_nbVox*_nbVox];
  memset(_voxStorage, 1, _nbVox*_nbVox*_nbVox * sizeof(unsigned char));
  memset(tmp, 1, _nbVox*_nbVox*_nbVox*sizeof(unsigned char));
  _createVox(); QOpenGLVertexArrayObject::Binder vaoPointsBinder(&_vaoPoints);

  _vertexBufferVox.bind();
  _vertexBufferVox.allocate(_voxVertices.constData(), _voxVertices.size() * sizeof(GLfloat));
  QOpenGLFunctions *g = QOpenGLContext::currentContext()->functions();
  g->glEnableVertexAttribArray(0);
  g->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
  _vertexBufferVox.release();
  update();
}

void Scene::intersect() {
    float voxSize = _spaceSize/_nbVox;
    const float *p = _pointsData.constData();
    memset(_voxStorage, 0, _nbVox*_nbVox*_nbVox*sizeof(unsigned char));
#pragma omp parallel for shared(voxSize, _voxStorage, _pointsBoundMin, p)
    for (size_t i = 0; i < _pointsCount * 7; i += 7) {
        int x = (p[i]+ - _pointsBoundMin[0]) / voxSize;
        int y = (p[i+1] - _pointsBoundMin[1]) / voxSize;
        int z = (p[i+2] - _pointsBoundMin[2]) / voxSize;
        _voxStorage[x*_nbVox*_nbVox + y*_nbVox + z] = 1;
    }
    update();
}

void Scene::carve() {
    memset(tmp, 1, _nbVox*_nbVox*_nbVox*sizeof(unsigned char));
    for (int i = 0; i < _listProjection.length() ; i++) {
        carveView(i);
    }
    update();
}

void Scene::carveView(int v) {
    float voxSize = _spaceSize/_nbVox;
    float halSize = voxSize * .5;
    float t[3];
    t[0] = _pointsBoundMin[0] + halSize;
    t[1] = _pointsBoundMin[1] + halSize;
    t[2] = _pointsBoundMin[2] + halSize;
    QVector4D X, X2;
    QImage mask(_maskPath+"/mask_"+QString::number(v)+".jpg");
    int h = mask.height(), w = mask.width();
    const auto viewMatrix = _listProjection.at(v) * _listView.at(v);
#pragma omp parallel for private(X, X2) shared(voxSize, _voxStorage, _pointsBoundMin, _nbVox, tmp, t) collapse(3)
    for (int i = 0; i < _nbVox ; i++) {
        for (int j = 0; j < _nbVox; j++) {
            for (int k = 0; k < _nbVox; k++){
                X2.setX(i * voxSize + t[0]);
                X2.setY(j * voxSize + t[1]);
                X2.setZ(k * voxSize + t[2]);
                X2.setW(1.0);

                X = viewMatrix * X2;

                float x = X.x() / X.z();
                float y = X.y() / -X.z();

                if (abs(x) >= 1. || abs(y) >= 1.) {
                    tmp[i*_nbVox*_nbVox + j*_nbVox + k] = 0;
                    continue;
                }

                float xNDC = (x + 2. / 2.) / 2.;
                float yNDC = (y + 2. / 2.) / 2.;

                float xRaster = std::floor(xNDC * w);
                float yRaster = std::floor(yNDC * h);

                if (mask.pixelColor(xRaster,yRaster).red() != 255){
                    tmp[i*_nbVox*_nbVox + j*_nbVox + k] = 0;
                }
            }
        }
    }
    memcpy(_voxStorage, tmp, _nbVox*_nbVox*_nbVox*sizeof(unsigned char));
}
