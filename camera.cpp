#include "camera.h"

void Camera::translate(float tx, float ty, float tz)
{
    setXTranslation(tx);
    setYTranslation(ty);
    setZTranslation(tz);
}

void Camera::rotate(float dx, float dy) {
  setXRotation(dx);
  setYRotation(dy);
}


void Camera::updateView()
{
    float * tx = _currentView.data() + 12;
    float * ty = _currentView.data() + 13;
    float * tz = _currentView.data() + 14;

    float tmpX = *tx;
    float tmpY = *ty;
    float tmpZ = *tz;

    *tx = 0.f;
    *ty = 0.f;
    *tz = 0.f;

    _currentView.rotate(-_yRotation, QVector3D(0, 1, 0));
    _currentView.rotate(_xRotation, QVector3D(1, 0, 0));

    *tx = tmpX + _xTranslation;
    *ty = tmpY + _yTranslation;
    *tz = tmpZ + _zTranslation;

    _xRotation = 0.f;
    _yRotation = 0.f;

    _xTranslation = 0.f;
    _yTranslation = 0.f;
    _zTranslation = 0.f;
}
