#version 130

uniform sampler2D colorMap;
uniform vec4 colors[8];
uniform uint colorId;

noperspective in vec2 fragUV;

out vec4 outColor;

void main() {
	outColor = texture(colorMap, fragUV) * colors[colorId];
}
