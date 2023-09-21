R"r(#version 130
uniform uvec2 addr;out uvec2 outAddr;void main(){if(addr.x!=0u||addr.y!=0u)outAddr=addr;else discard;})r"
