#version  330 core
layout(location = 0) in vec4 vertPos;
layout(location = 1) in vec3 vertNor;
layout(location = 2) in vec2 vertTex;
uniform float d_time;
uniform mat4 P;
uniform mat4 V;
uniform mat4 M;
uniform vec3 cameraPos;
uniform vec3 lightPos;

uniform vec4 plane;

const float tiling = 6.0;

out vec2 texCoords;
out vec4 clipSpace;
out vec3 toCameraVector;
out vec3 fromLightVector;

void main() {
	vec4 worldPosition = M * vertPos;
	texCoords = vertTex * tiling;
	gl_Position = P * V * worldPosition;
	clipSpace = gl_Position;
	toCameraVector = cameraPos - worldPosition.xyz;
	fromLightVector = worldPosition.xyz - lightPos;
}