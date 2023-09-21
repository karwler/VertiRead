import os


def reduce_source(name: str, src_dir: str, dst_dir: str):
	lf = 0xA
	space = 0x20
	pound = 0x23
	asterisk = 0x2A
	slash = 0x2F
	dg_0 = 0x30
	dg_9 = 0x39
	up_a = 0x41
	up_z = 0x5A
	underscore = 0x5F
	lo_a = 0x61
	lo_z = 0x7A

	def check_text(ch: int):
		return dg_0 <= ch <= dg_9 or up_a <= ch <= up_z or lo_a <= ch <= lo_z or ch == underscore

	with open(os.path.join(src_dir, name), "r") as fh:
		text = bytearray(fh.read().strip(), 'ascii')

	i = 0
	while i < len(text):
		if text[i] <= space:
			e = i + 1
			while e < len(text) and text[e] <= space:
				e += 1
			del text[i:e]
			if check_text(text[i - 1]) and check_text(text[i]):
				text.insert(i, space)
			else:
				i -= 1
		elif text[i] == pound:
			e = text.find(lf, i + 1)
			i = e if e != -1 else len(text) - 1
		elif text[i] == slash:
			if text[i + 1] == slash:
				e = text.find(lf, i + 2)
				del text[i:e]
				i -= 1
			elif text[i + 1] == asterisk:
				e = text.find(b'*/', i)
				e = e + 2 if e != -1 else len(text)
				del text[i:e]
				i -= 1
		i += 1

	with open(f'{os.path.join(dst_dir, name)}.rel.h', 'w') as fh:
		fh.write('R"r(')
		fh.write(str(text, 'ascii'))
		fh.write(')r"\n')


if __name__ == '__main__':
	srcd = os.path.join(os.path.dirname(__file__), 'shaders')
	dstd = os.path.join(os.path.dirname(__file__), os.pardir, 'src', 'engine', 'shaders')
	for it in ['glGui.vert', 'glGui.frag', 'glSel.vert', 'glSel.frag']:
		try:
			reduce_source(it, srcd, dstd)
		except Exception as e:
			print(e)
