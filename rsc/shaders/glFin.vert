#version 130

in vec2 vpos;
in vec2 vtuv;

noperspective out vec2 fragUV;

void main() {
	fragUV = vtuv;
	gl_Position = vec4(vpos, 0.0, 1.0);
}
