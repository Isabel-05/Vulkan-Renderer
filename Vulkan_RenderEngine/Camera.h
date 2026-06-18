#pragma once
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>


enum class CameraMovement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT,
    UP,
    DOWN
}; 

class Camera
{
private:

    // Spatial positioning and orientation vectors
    // These form the camera's local coordinate system in world space
    glm::vec3 position;     // Camera's location in world coordinates
    glm::vec3 front;        // Forward direction (where camera is looking)
    glm::vec3 up;           // Camera's local up direction (for roll control)
    glm::vec3 right;        // Camera's local right direction (perpendicular to front and up)
    glm::vec3 worldUp;      // Global up vector reference (typically Y-axis)

    float horizontalAngle;
	float verticalAngle;
    float radius;

    float movementSpeed; 
    float mouseSensitivity; 
    float zoom;



public:

    Camera();

	~Camera() = default;

    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjectionMatrix(float aspectRatio, float nearPlane = 0.1f, float farPlane = 100.0f) const;

    void processMouseMovement(float xOffset, float yOffset, bool constrainPitch = true);  // Mouse-based rotation
    void processMouseScroll(float yOffset);                              // Scroll-based zoom control

    bool mousePressed = false;

    glm::vec3 getPosition() const { return position; }
    glm::vec3 getFront() const { return front; }
    float getZoom() const { return zoom; }
};

