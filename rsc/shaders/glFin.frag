#version 130

uniform sampler2D sceneMap;
uniform float gamma;

noperspective in vec2 fragUV;

out vec4 outColor;

void main() {
	outColor = vec4(pow(texture(sceneMap, fragUV).rgb, vec3(gamma)), 1.0);
}
