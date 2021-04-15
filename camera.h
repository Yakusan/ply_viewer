#ifndef __CAMERA_H__
#define __CAMERA_H__

#include <QMatrix4x4>

class Camera : public QObject
{
  Q_OBJECT

private:
    QMatrix4x4 _currentView;

    float      _xTranslation;
    float      _yTranslation;
    float      _zTranslation;

    float      _xRotation;
    float      _yRotation;

public:
    Camera() :
      _currentView(),
      _xTranslation(0.f), _yTranslation(0.f), _zTranslation(0.f),
      _xRotation(0.f), _yRotation(0.f) {}

    inline void setViewMatrix(const QMatrix4x4 & currentView) { _currentView = currentView; }
    inline const QMatrix4x4 & viewMatrix() { return _currentView; }

    inline void setXTranslation(float tx) { _xTranslation = tx; }
    inline void setYTranslation(float ty) { _yTranslation = ty; }
    inline void setZTranslation(float tz) { _zTranslation = tz; }

    void translate(float tx, float ty, float tz);

    inline void setXRotation(float angle) { _xRotation = angle; }
    inline void setYRotation(float angle) { _yRotation = angle; }

    void rotate(float dx, float dy);

    void updateView();
};

#endif // __CAMERA_H__

