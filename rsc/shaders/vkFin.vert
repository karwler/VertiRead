#version 460

layout(location = 0) in vec2 vpos;

void main() {
	gl_Position = vec4(vpos, 0.0, 1.0);
}
