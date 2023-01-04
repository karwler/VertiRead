import os
import shutil
import subprocess

vsGui = '''#version 450

const vec2 vposs[4] = vec2[](
	vec2(0.0, 0.0),
	vec2(1.0, 0.0),
	vec2(0.0, 1.0),
	vec2(1.0, 1.0)
);

layout(push_constant) uniform PushData {
	ivec4 rect;
	ivec4 frame;
	ivec4 txloc;
	vec4 color;
	uvec2 tid;
} pc;

layout(set = 0, binding = 0) uniform UniformData0 {
	vec2 tbounds[3];
} u0;

layout(set = 1, binding = 0) uniform UniformData1 {
	vec4 pview;
} u1;

layout(location = 0) noperspective out vec2 fragUV;

void main() {
	vec2 dpos, dsiz = vec2(0.0);
	if (pc.rect[2] > 0 && pc.rect[3] > 0 && pc.frame[2] > 0 && pc.frame[3] > 0) {
		dpos = vec2(max(pc.rect.xy, pc.frame.xy));
		dsiz = vec2(min(pc.rect.xy + pc.rect.zw, pc.frame.xy + pc.frame.zw)) - dpos;
	}

	if (dsiz.x > 0.0 && dsiz.y > 0.0) {
		vec2 uvpos = vec2(pc.txloc.xy) + (dpos - vec2(pc.rect.xy)) / vec2(pc.rect.zw) * vec2(pc.txloc.zw);
		vec2 uvsiz = dsiz / vec2(pc.rect.zw) * vec2(pc.txloc.zw);
		fragUV = (vposs[gl_VertexIndex] * uvsiz + uvpos) / u0.tbounds[pc.tid.x];
		vec2 loc = vposs[gl_VertexIndex] * dsiz + dpos;
		gl_Position = vec4((loc - u1.pview.xy) / u1.pview.zw - 1.0, 0.0, 1.0);
	} else {
		fragUV = vec2(0.0);
		gl_Position = vec4(-2.0, -2.0, 0.0, 1.0);
	}
}'''

fsGui = '''#version 450

layout(push_constant) uniform PushData {
	ivec4 rect;
	ivec4 frame;
	ivec4 txloc;
	vec4 color;
	uvec2 tid;
} pc;

layout(set = 0, binding = 1) uniform sampler2DArray icons;
layout(set = 0, binding = 2) uniform sampler2DArray texts;
layout(set = 0, binding = 3) uniform sampler2DArray rpics;

layout(location = 0) noperspective in vec2 fragUV;

layout(location = 0) out vec4 outColor;

void main() {
	switch (pc.tid.x) {
	case 0:
		outColor = texture(icons, vec3(fragUV, pc.tid.y)) * pc.color;
		break;
	case 1:
		outColor = texture(texts, vec3(fragUV, pc.tid.y));
		break;
	case 2:
		outColor = texture(rpics, vec3(fragUV, pc.tid.y));
	}
}'''

vsSel = '''#version 450

const vec2 vposs[4] = vec2[](
	vec2(0.0, 0.0),
	vec2(1.0, 0.0),
	vec2(0.0, 1.0),
	vec2(1.0, 1.0)
);

layout(push_constant) uniform PushData {
	ivec4 rect;
	ivec4 frame;
	uvec2 addr;
} pc;

layout(binding = 0) uniform UniformData {
	vec4 pview;
} ud;

void main() {
	vec2 dpos, dsiz = vec2(0.0);
	if (pc.rect[2] > 0 && pc.rect[3] > 0 && pc.frame[2] > 0 && pc.frame[3] > 0) {
		dpos = vec2(max(pc.rect.xy, pc.frame.xy));
		dsiz = vec2(min(pc.rect.xy + pc.rect.zw, pc.frame.xy + pc.frame.zw)) - dpos;
	}

	if (dsiz.x > 0.0 && dsiz.y > 0.0) {
		vec2 loc = vposs[gl_VertexIndex] * dsiz + dpos;
		gl_Position = vec4((loc - ud.pview.xy) / ud.pview.zw - 1.0, 0.0, 1.0);
   } else
		gl_Position = vec4(-2.0, -2.0, 0.0, 1.0);
}'''

fsSel = '''#version 450

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
}'''

def compile_source(glslc: str, code: str, name: str):
	with open(name, 'w') as fh:
		fh.write(code)

	for dbg in [ True, False ]:
		try:
			opt = '-g' if dbg else '-O'
			ext = 'dbg' if dbg else 'rel'
			spvFile = f'{name}.{ext}.spv'
			cppFile = f'{name}.{ext}.h'

			ret = subprocess.run([ glslc, '--target-env=vulkan1.0', '--target-spv=spv1.0', opt, '-o', spvFile, name ])
			if ret.stdout:
				print(f'stdout: {ret.stdout}')
			if ret.stderr:
				print(f'stderr: {ret.stderr}')
			if ret.returncode != 0:
				print(f'returned: {ret.returncode}')

			with open(spvFile, "rb") as fh:
				data = fh.read()
			if len(data) % 4 != 0:
				print('size not divisible by 4')
			with open(cppFile, 'w') as fh:
				fh.write(',\n'.join(f'0x{(data[i] | (data[i + 1] << 8) | (data[i + 2] << 16) | (data[i + 3] << 24)):X}' for i in range(0, len(data), 4)))
				fh.write('\n')
			os.remove(spvFile)
		except Exception as e:
			print(e)
	os.remove(name)

if __name__ == '__main__':
	glslc = shutil.which('glslc')
	os.chdir(os.path.join(os.path.dirname(__file__), os.pardir, 'src', 'engine', 'shaders'))
	for it in [ (vsGui, 'vk.gui.vert'), (fsGui, 'vk.gui.frag'), (vsSel, 'vk.sel.vert'), (fsSel, 'vk.sel.frag') ]:
		compile_source(glslc, it[0], it[1])
