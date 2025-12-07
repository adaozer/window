#pragma once
#include "maths.h"
#include <cmath>

class Camera {
public:
    Vec3 position = Vec3(0, 0, 5);   

    float yaw = 0.0f;               
    float pitch = 0.0f;             

    float moveSpeed = 5.0f;   
    float sensitivity = 0.002f; 

    Vec3 forward() const {
        Vec3 f(
            std::cos(pitch) * std::cos(yaw),
            std::sin(pitch),
            std::cos(pitch) * std::sin(yaw)
        );
        return f.normalize();     
    }

    Vec3 right() const {
        Vec3 fwd = forward();
        Vec3 worldUp(0.0f, 1.0f, 0.0f);

        // right = Cross(up, dir)
        Vec3 r = Vec3::Cross(worldUp, fwd).normalize();
        return r;
    }

    Matrix viewMatrix() const {
        Vec3 f = forward();
        Vec3 target = position + f;
        Vec3 up(0.0f, 1.0f, 0.0f);

        // matches your Matrix::lookAt(to, from, up)
        return Matrix::lookAt(target, position, up);
    }
};
