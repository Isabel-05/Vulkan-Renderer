#pragma once
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>


class Camera
{
private:

    glm::vec3 position;   
    glm::vec3 front;       
    glm::vec3 up ;        
    glm::vec3 right;      
    glm::vec3 worldUp;      

    float horizontalAngle;
	float verticalAngle;
    float radius;

    float mouseSensitivity; 
    float zoom;

public:

    Camera();

	~Camera() = default;

    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjectionMatrix(float aspectRatio, float nearPlane = 0.1f, float farPlane = 100.0f) const;

    void processMouseMovement(float xOffset, float yOffset, bool constrainPitch = true);
    void processMouseScroll(float yOffset);

    bool mousePressed = false;

    glm::vec3 getPosition() const { return position; }
    glm::vec3 getFront() const { return front; }
    float getZoom() const { return zoom; }
};

