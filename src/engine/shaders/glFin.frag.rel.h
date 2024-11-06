R"r(#version 130
uniform sampler2D d;uniform float e;noperspective in vec2 c;out vec4 f;void main(){f=vec4(pow(texture(d,c).rgb,vec3(e)),1.0);})r"
