#ifndef CAMERA_H
#define CAMERA_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vector>

// Defines several possible options for camera movement. Used as abstraction to stay away from window-system specific input methods
enum Camera_Movement {
	FORWARD,
	BACKWARD,
	LEFT,
	RIGHT
};

// Default camera values
const float YAW = -90.0f;
const float PITCH = 0.0f;
const float SPEED = 1.0f;
const float SENSITIVITY = 0.04f;
const float ZOOM = 45.0f;


// An abstract camera class that processes input and calculates the corresponding Euler Angles, Vectors and Matrices for use in OpenGL
class Camera
{
public:
	// camera Attributes
	glm::vec3 Position;
	glm::vec3 Front;
	glm::vec3 Up;
	glm::vec3 Right;
	glm::vec3 WorldUp;
	// euler Angles
	float Yaw;
	float Pitch;//how high the camera is
	// camera options
	float MovementSpeed=0.01f;
	float MouseSensitivity;
	float Zoom;
	float distance_from_user = 0.00003f;
	float angle_around_user = 0.0f;
	float player_x;
	float player_y;
	float player_z;
	float player_y_rotate;
	bool is_player = true;
	// constructor with vectors
	Camera(bool is_player,float rotate_step, float x_position, float y_position, float z_position, glm::vec3 position , glm::vec3 up = glm::vec3(0.0f, 3.0f, 0.0f), float yaw = YAW, float pitch = PITCH) : Front(glm::vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY), Zoom(ZOOM)
	{
		Position = position;
		WorldUp = up;
		Yaw = yaw;
		Pitch = pitch;
		player_x = x_position;
		player_y = y_position+1.5f;
		if (is_player) {
			player_z = z_position + 2.0f;
		}
		else {
			player_z = z_position - 5.0f;
		}
		player_y_rotate = rotate_step;
		this->is_player = is_player;
		updateCameraVectors();
	}
	// constructor with scalar values
	Camera(float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch) : Front(glm::vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY), Zoom(ZOOM)
	{
		Position = glm::vec3(posX, posY, posZ);
		WorldUp = glm::vec3(upX, upY, upZ);
		Yaw = yaw;
		Pitch = pitch;
		updateCameraVectors();
	}
	// returns the view matrix calculated using Euler Angles and the LookAt Matrix
	glm::mat4 GetViewMatrix()
	{
		calculateCameraPosition();
		return glm::lookAt(Position, Position + Front, Up);
	}


	//calculateHorizontalDistance
	float calculateHorizontalDistance() {
		return (float)(distance_from_user*glm::cos(glm::radians(Pitch)));
	}
	//calculateVerticalDistance
	float calculateVerticalDistance() {
		return (float)(distance_from_user*glm::sin(glm::radians(Pitch)));
	}
	void changePosition(float rotate_step, float x_position, float y_position, float z_position) {
		player_x = x_position;
		player_y = y_position+1.5f;
		player_z = z_position+2.0f;
		player_y_rotate = rotate_step;
	}
	// processes input received from any keyboard-like input system. Accepts input parameter in the form of camera defined ENUM (to abstract it from windowing systems)
	

	// processes input received from a mouse input system. Expects the offset value in both the x and y direction.
	void ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch = true)
	{
		xoffset *= MouseSensitivity;
		yoffset *= MouseSensitivity;

		Yaw += xoffset;
		Pitch += yoffset;

		// make sure that when pitch is out of bounds, screen doesn't get flipped
		if (constrainPitch)
		{
			if (Pitch > 89.0f)
				Pitch = 89.0f;
			if (Pitch < -89.0f)
				Pitch = -89.0f;
		}

		// update Front, Right and Up Vectors using the updated Euler angles
		updateCameraVectors();
	}

	// processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis
	void ProcessMouseScroll(float yoffset)
	{
		Zoom -= (float)yoffset;
		if (Zoom < 1.0f)
			Zoom = 1.0f;
		if (Zoom > 45.0f)
			Zoom = 45.0f;
	}

private:
	// calculates the front vector from the Camera's (updated) Euler Angles
	void updateCameraVectors()
	{
		// calculate the new Front vector
		glm::vec3 front;
		front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
		front.y = sin(glm::radians(Pitch));
		front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
		Front = glm::normalize(front);
		// also re-calculate the Right and Up vector
		Right = glm::normalize(glm::cross(Front, WorldUp));  // normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
		Up = glm::normalize(glm::cross(Right, Front));
	}
	void calculateCameraPosition() {

		float horizontalDistance = calculateHorizontalDistance();
		float verticalDistance = calculateVerticalDistance();
		float theta = player_y_rotate + angle_around_user;
		float offsetX =(float) (horizontalDistance * glm::sin(glm::radians(theta)));
		float offsetZ = (float)(horizontalDistance * glm::cos(glm::radians(theta)));
		Position.x = player_x - offsetX;
		Position.y = player_y + verticalDistance;
		if (this->is_player) {
			Position.z = player_z - offsetZ;
		}
		else
		{
			Position.z = player_z - 2.5f;
		}
	}
};
#endif