#version 130

uniform sampler2D colorMap;
uniform vec4 color;

noperspective in vec2 fragUV;

out vec4 outColor;

void main() {
	outColor = texture(colorMap, fragUV) * color;
}
