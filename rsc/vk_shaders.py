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


def compile_source(glslc: str, name: str, src_dir: str, dst_dir: str):
	src_file = os.path.join(src_dir, name)
	for dbg in [True, False]:
		try:
			opt = '-g' if dbg else '-O'
			ext = 'dbg' if dbg else 'rel'
			spv_file = os.path.join(dst_dir, f'{name}.{ext}.spv')
			cpp_file = os.path.join(dst_dir, f'{name}.{ext}.h')

			ret = subprocess.run([glslc, '--target-env=vulkan1.0', '--target-spv=spv1.0', opt, '-o', spv_file, src_file])
			if ret.stdout:
				print(f'stdout: {ret.stdout}')
			if ret.stderr:
				print(f'stderr: {ret.stderr}')
			if ret.returncode != 0:
				print(f'returned: {ret.returncode}')

			bytes_to_text(spv_file, cpp_file, 4)
			os.remove(spv_file)
		except Exception as e:
			print(e)


if __name__ == '__main__':
	comp = shutil.which('glslc')
	if not comp:
		raise RuntimeError('Failed to find glslc')

	srcd = os.path.join(os.path.dirname(__file__), 'shaders')
	dstd = os.path.join(os.path.dirname(__file__), os.pardir, 'src', 'engine', 'shaders')
	for it in ['vkGui.vert', 'vkGui.frag', 'vkSel.vert', 'vkSel.frag', 'vkIdx.comp', 'vkRgb.comp']:
		compile_source(comp, it, srcd, dstd)
