import os
import shutil
import subprocess


def make_word(data: bytes, offset: int, size: int) -> int:
	word = 0
	for i in range(0, size):
		word |= data[offset + i] << (8 * i)
	return word


def bytes_to_text(src_file: str, dst_file: str, wsize: int):
	with open(src_file, "rb") as fh:
		data = fh.read()

	over = len(data) % wsize
	end = len(data) - over
	words = list(make_word(data, i, wsize) for i in range(0, end, wsize))
	if over != 0:
		print(f'Size is not a multiple of {wsize}. Padding with zeros.')
		words.append(make_word(data, end, over))

	with open(dst_file, 'w') as fh:
		fh.write(',\n'.join(f'0x{w:X}' for w in words))
		fh.write('\n')


def compile_source(fxc: str, name: str, src_dir: str, dst_dir: str, model: str):
	src_file = os.path.join(src_dir, name)
	stem = os.path.splitext(name)[0]
	version = f'vs_{model}' if stem[-2:].lower() == 'vs' else f'ps_{model}'
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

			bytes_to_text(fxc_file, cpp_file, 4)
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
		compile_source(comp, it, srcd, dstd, '5_0')
