R"r(#version 130
uniform vec4 a;uniform ivec4 b;uniform ivec4 c;in vec2 d;noperspective out vec2 e;void main(){vec4 f=vec4(0.0);if(b[2]>0&&b[3]>0&&c[2]>0&&c[3]>0){f.xy=vec2(max(b.xy,c.xy));f.zw=vec2(min(b.xy+b.zw,c.xy+c.zw))-f.xy;}if(f[2]>0.0&&f[3]>0.0){vec4 g=vec4(f.xy-vec2(b.xy),f.zw)/vec4(b.zwzw);e=d*g.zw+g.xy;vec2 h=d*f.zw+f.xy;gl_Position=vec4((h.x-a.x)/a[2]-1.0,-(h.y-a.y)/a[3]+1.0,0.0,1.0);}else{e=vec2(0.0);gl_Position=vec4(-2.0,-2.0,0.0,1.0);}})r"
