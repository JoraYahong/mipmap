#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec2 TexCoords;
out vec3 surfaceNormal;
out vec3 toLightVector;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;


void main()
{ 
    vec4 worldPosition = model * vec4(aPos, 1.0);
	TexCoords = aTexCoords;    
    gl_Position = projection * view * worldPosition;

	surfaceNormal = (model * vec4(aNormal,0.0f)).xyz;

}