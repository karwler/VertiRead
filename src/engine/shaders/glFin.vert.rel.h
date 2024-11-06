R"r(#version 130
in vec2 a;in vec2 b;noperspective out vec2 c;void main(){c=b;gl_Position=vec4(a,0.0,1.0);})r"
