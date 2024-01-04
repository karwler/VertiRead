R"r(#version 130
uniform sampler2D f;uniform vec4 g;noperspective in vec2 e;out vec4 h;void main(){h=texture(f,e)*g;})r"
