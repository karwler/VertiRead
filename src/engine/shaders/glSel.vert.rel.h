R"r(#version 130
uniform vec4 a;uniform ivec4 b;uniform ivec4 c;in vec2 d;void main(){vec4 e=vec4(0.0);if(b[2]>0&&b[3]>0&&c[2]>0&&c[3]>0){e.xy=vec2(max(b.xy,c.xy));e.zw=vec2(min(b.xy+b.zw,c.xy+c.zw))-e.xy;}if(e[2]>0.0&&e[3]>0.0){vec2 f=d*e.zw+e.xy;gl_Position=vec4((f.x-a.x)/a[2]-1.0,-(f.y-a.y)/a[3]+1.0,0.0,1.0);}else gl_Position=vec4(-2.0,-2.0,0.0,1.0);})r"
