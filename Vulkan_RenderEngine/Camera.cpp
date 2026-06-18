#include "Camera.h"

Camera::Camera()
{
    position = glm::vec3(0.0f, 0.0f, 0.0f);
    up = glm::vec3(0.0f, 1.0f, 0.0f);        // Y-axis as world up
	horizontalAngle = 0.0f;
	verticalAngle = 90.0f;
	radius = 8.0f;
    zoom = 45.0f;
    movementSpeed = 0.0001f;
    mouseSensitivity = 0.2f;
}

glm::mat4 Camera::getViewMatrix() const
{
    return glm::lookAt(position, glm::vec3(0.0f, 0.0f, 0.0f), up);
}

glm::mat4 Camera::getProjectionMatrix(float aspectRatio, float nearPlane, float farPlane) const
{
    return glm::perspective(glm::radians(zoom), aspectRatio, nearPlane, farPlane);
}

void Camera::processMouseMovement(float xOffset, float yOffset, bool constrainPitch)
{
	if (!mousePressed)
		return;

    xOffset *= mouseSensitivity;
    yOffset *= mouseSensitivity;

	horizontalAngle += xOffset;
	verticalAngle += yOffset;

    if (horizontalAngle < 0)
        horizontalAngle += 360.0f;
    else if (horizontalAngle > 360.0f)
        horizontalAngle -= 360.0f;
    if (verticalAngle < 0) 
    {
        verticalAngle += 360.0f;
    } 
    else if (verticalAngle > 360.0f)
    {
        verticalAngle -= 360.0f;
    }

    if(verticalAngle > 180.0f)
    {
		up = glm::vec3(0.0f, -1.0f, 0.0f);
	}
	else
	{  
		up = glm::vec3(0.0f, 1.0f, 0.0f);
	}
        

	position.x = radius * cos(glm::radians(horizontalAngle)) * sin(glm::radians(verticalAngle));
	position.z = radius * sin(glm::radians(horizontalAngle)) * sin(glm::radians(verticalAngle));
	position.y = radius * cos(glm::radians(verticalAngle));

}
