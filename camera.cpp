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

// Vue FPS
void Camera::updateView()
{
    _currentView = _currentView.inverted();

    _currentView.rotate(_xRotation, QVector3D(1, 0, 0));
    _currentView.rotate(_yRotation, QVector3D(0, 1, 0));
    _currentView.translate(QVector3D(_xTranslation, _yTranslation, _zTranslation));
    _currentView = _currentView.inverted();

    _xRotation = 0.f;
    _yRotation = 0.f;

    _xTranslation = 0.f;
    _yTranslation = 0.f;
    _zTranslation = 0.f;
}
