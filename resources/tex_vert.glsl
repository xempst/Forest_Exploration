#version  330 core
layout(location = 0) in vec3 vertPos;
layout(location = 1) in vec3 vertNor;
layout(location = 2) in vec2 vertTex;
uniform mat4 P;
uniform mat4 M;
uniform mat4 V;

out vec2 vTexCoord;
out vec3 fragNor;

out vec3 lightDir;
out vec3 EPos;

uniform vec3 lightPos;
uniform vec4 plane;

void main() {

  vec4 vPosition;

  /* First model transforms */
  gl_Position = P * V *M * vec4(vertPos.xyz, 1.0);
  gl_ClipDistance[0] = dot(M * vec4(vertPos.xyz, 1.0), plane);

  fragNor = (M * vec4(vertNor, 0.0)).xyz;

  lightDir = lightPos - (M*vec4(vertPos.xyz, 1.0)).xyz;
  EPos = (M*vec4(vertPos.xyz, 1.0)).xyz;

  /* pass through the texture coordinates to be interpolated */
  vTexCoord = vertTex;
}
