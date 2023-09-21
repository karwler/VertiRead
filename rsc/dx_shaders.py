import os
import shutil
import subprocess


def compile_source(fxc: str, name: str, src_dir: str, dst_dir: str):
	src_file = os.path.join(src_dir, name)
	stem = os.path.splitext(name)[0]
	version = 'vs_5_0' if stem[-2:].lower() == 'vs' else 'ps_5_0'
	for dbg in [True, False]:
		try:
			opt = ['/Od', '/Zi'] if dbg else ['/O1']
			ext = 'dbg' if dbg else 'rel'
			fxc_file = os.path.join(dst_dir, f'{stem}.{ext}.cso')
			cpp_file = os.path.join(dst_dir, f'{stem}.{ext}.h')

			ret = subprocess.run([fxc, '/Ges', '/Gis', '/T', version, '/Fo', fxc_file, src_file] + opt)
			if ret.stdout:
				print(f'stdout: {ret.stdout}')
			if ret.stderr:
				print(f'stderr: {ret.stderr}')
			if ret.returncode != 0:
				print(f'returned: {ret.returncode}')

			with open(fxc_file, "rb") as fh:
				data = fh.read()
			with open(cpp_file, 'w') as fh:
				fh.write(',\n'.join(f'0x{c:X}' for c in data))
				fh.write('\n')
			os.remove(fxc_file)
		except Exception as e:
			print(e)


if __name__ == '__main__':
	comp = shutil.which('fxc')
	if not comp:
		raise RuntimeError('Failed to find fxc')

	srcd = os.path.join(os.path.dirname(__file__), 'shaders')
	dstd = os.path.join(os.path.dirname(__file__), os.pardir, 'src', 'engine', 'shaders')
	for it in ['dxGuiVs.hlsl', 'dxGuiPs.hlsl', 'dxSelVs.hlsl', 'dxSelPs.hlsl']:
		compile_source(comp, it, srcd, dstd)
