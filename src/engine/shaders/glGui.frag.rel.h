R"r(#version 130
uniform sampler2D f;uniform vec4 g[8];uniform uint h;noperspective in vec2 e;out vec4 i;void main(){i=texture(f,e)*g[h];})r"
