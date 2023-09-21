import os
import shutil
import subprocess


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

			with open(spv_file, "rb") as fh:
				data = fh.read()
			if len(data) % 4 != 0:
				print('size not divisible by 4')
			with open(cpp_file, 'w') as fh:
				fh.write(',\n'.join(f'0x{(data[i] | (data[i + 1] << 8) | (data[i + 2] << 16) | (data[i + 3] << 24)):X}' for i in range(0, len(data), 4)))
				fh.write('\n')
			os.remove(spv_file)
		except Exception as e:
			print(e)


if __name__ == '__main__':
	comp = shutil.which('glslc')
	if not comp:
		raise RuntimeError('Failed to find glslc')

	srcd = os.path.join(os.path.dirname(__file__), 'shaders')
	dstd = os.path.join(os.path.dirname(__file__), os.pardir, 'src', 'engine', 'shaders')
	for it in ['vkConv.comp', 'vkGui.vert', 'vkGui.frag', 'vkSel.vert', 'vkSel.frag']:
		compile_source(comp, it, srcd, dstd)
