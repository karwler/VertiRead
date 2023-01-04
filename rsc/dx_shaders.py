import os
import shutil
import subprocess

vsGui = '''
struct VertOut {
	float4 pos : SV_POSITION;
	float2 tuv : TEXCOORD0;
};

cbuffer Pview : register(b0) {
	float4 pview;
};

cbuffer Instance : register(b1) {
	int4 rect;
	int4 frame;
};

static const float2 vposs[] = {
	float2(0.f, 0.f),
	float2(1.f, 0.f),
	float2(0.f, 1.f),
	float2(1.f, 1.f)
};

VertOut main(uint vid : SV_VertexId) {
	float4 dst = float4(0.f, 0.f, 0.f, 0.f);
	if (rect[2] > 0 && rect[3] > 0 && frame[2] > 0 && frame[3] > 0) {
		dst.xy = max(rect.xy, frame.xy);
		dst.zw = min(rect.xy + rect.zw, frame.xy + frame.zw) - dst.xy;
	}

	VertOut vout;
	if (dst[2] > 0.f && dst[3] > 0.f) {
		float4 uvrc = float4(dst.xy - rect.xy, dst.zw) / float4(rect.zwzw);
		vout.tuv = vposs[vid] * uvrc.zw + uvrc.xy;
		float2 loc = vposs[vid] * dst.zw + dst.xy;
		vout.pos = float4((loc.x - pview.x) / pview[2] - 1.f, -(loc.y - pview.y) / pview[3] + 1.0, 0.f, 1.f);
	} else {
		vout.tuv = float2(0.f, 0.f);
		vout.pos = float4(-2.f, -2.f, 0.f, 1.f);
	}
	return vout;
}'''

psGui = '''
Texture2D textureView : register(t0);
SamplerState sampleState : register(s0);

cbuffer InstanceColor : register(b0) {
	float4 color;
};

float4 main(float4 pos : SV_POSITION, float2 tuv : TEXCOORD0) : SV_TARGET {
	return textureView.Sample(sampleState, tuv) * color;
}'''

vsSel = '''
cbuffer Pview : register(b0) {
	float4 pview;
};

cbuffer Instance : register(b1) {
	int4 rect;
	int4 frame;
};

static const float2 vposs[] = {
	float2(0.f, 0.f),
	float2(1.f, 0.f),
	float2(0.f, 1.f),
	float2(1.f, 1.f)
};

float4 main(uint vid : SV_VertexId) : SV_POSITION {
	float4 dst = float4(0.f, 0.f, 0.f, 0.f);
	if (rect[2] > 0 && rect[3] > 0 && frame[2] > 0 && frame[3] > 0) {
		dst.xy = max(rect.xy, frame.xy);
		dst.zw = min(rect.xy + rect.zw, frame.xy + frame.zw) - dst.xy;
	}

	if (dst[2] > 0.f && dst[3] > 0.f) {
		float2 loc = vposs[vid] * dst.zw + dst.xy;
		return float4((loc.x - pview.x) / pview[2] - 1.0, -(loc.y - pview.y) / pview[3] + 1.0, 0.f, 1.f);
	}
	return float4(-2.f, -2.f, 0.f, 1.f);
}'''

psSel = '''
cbuffer InstanceAddr : register(b1) {
	uint2 addr;
};

uint2 main(float4 pos : SV_POSITION) : SV_TARGET {
	if (addr.x != 0 || addr.y != 0)
		return addr;
	discard;
	return uint2(0, 0);
}'''

def compile_source(fxc: str, code: str, name: str, ver: str):
	with open(name, 'w') as fh:
		fh.write(code)

	for dbg in [ True, False ]:
		try:
			opt = [ '/Od', '/Zi' ] if dbg else [ '/O1' ]
			ext = 'dbg' if dbg else 'rel'
			fxcFile = f'{name}.{ext}.fxc'
			cppFile = f'{name}.{ext}.h'

			ret = subprocess.run([ fxc, '/Ges', '/Gis', '/T', ver, '/Fo', fxcFile, name ] + opt)
			if ret.stdout:
				print(f'stdout: {ret.stdout}')
			if ret.stderr:
				print(f'stderr: {ret.stderr}')
			if ret.returncode != 0:
				print(f'returned: {ret.returncode}')

			with open(fxcFile, "rb") as fh:
				data = fh.read()
			with open(cppFile, 'w') as fh:
				fh.write(',\n'.join(f'0x{c:X}' for c in data))
				fh.write('\n')
			os.remove(fxcFile)
		except Exception as e:
			print(e)
	os.remove(name)

if __name__ == '__main__':
	fxc = shutil.which('fxc')
	os.chdir(os.path.join(os.path.dirname(__file__), os.pardir, 'src', 'engine', 'shaders'))
	for it in [ (vsGui, 'dx.gui.vert', 'vs_5_0'), (psGui, 'dx.gui.pixl', 'ps_5_0'), (vsSel, 'dx.sel.vert', 'vs_5_0'), (psSel, 'dx.sel.pixl', 'ps_5_0') ]:
		compile_source(fxc, it[0], it[1], it[2])
