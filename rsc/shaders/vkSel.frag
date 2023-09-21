#version 450

layout(push_constant) uniform PushData {
	ivec4 rect;
	ivec4 frame;
	uvec2 addr;
} pc;

layout(location = 0) out uvec2 outAddr;

void main() {
	if (pc.addr.x != 0u || pc.addr.y != 0u)
		outAddr = pc.addr;
	else
		discard;
}
