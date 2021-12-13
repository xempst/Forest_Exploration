#version 330 core 

out vec4 color;

uniform vec3 MatAmb;
uniform vec3 MatDif;
uniform vec3 MatSpec;

uniform float MatShine;

//interpolated normal and light vector in camera space
in vec3 fragNor;
in vec3 lightDir;
//position of the vertex in camera space
in vec3 EPos;

void main()
{
	//you will need to work with these for lighting
	vec3 normal = normalize(fragNor);
	vec3 light = normalize(lightDir);

	float dC = max(0, dot(normal, light));
	vec3 H = normalize(lightDir + EPos);
	float sC = max(0, dot(normal, H));

	color = vec4(MatDif*dC, 1.0) + vec4(MatSpec*pow(sC, MatShine), 1.0);

	//color = vec4(MatAmb*10.0, 1.0);
}
