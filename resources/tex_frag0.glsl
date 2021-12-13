#version 330 core
uniform sampler2D Texture0;
uniform vec3 MatAmb;
uniform vec3 MatDif;
uniform vec3 MatSpec;

uniform float MatShine;
uniform int colorMode;

in vec2 vTexCoord;
in float dCo;
//interpolated normal and light vector in camera space
in vec3 fragNor;
in vec3 lightDir;
//position of the vertex in camera space
in vec3 EPos;


layout(location = 0) out vec4 Outcolor;



void main() {
    vec3 normal = normalize(fragNor);
	vec3 light = normalize(lightDir);

	float dC = max(0, dot(normal, light));
	vec3 H = normalize(lightDir + EPos);
	float sC = max(0, dot(normal, H));

	vec4 texColor0 = texture(Texture0, vTexCoord);
  	Outcolor = texColor0 * dC * 0.5 + texColor0 * sC * 0.5;

	if (colorMode == 0)
		Outcolor = vec4(MatDif*dC, 1.0) + vec4(MatSpec*pow(sC, MatShine), 1.0);
}

