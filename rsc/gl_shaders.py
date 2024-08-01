from enum import Enum
import os
import sys


class VarInfo:
	name: str
	ori: str
	alt: str

	def __init__(self, macro: str, original: str, alternate: str):
		self.name = macro
		self.ori = original
		self.alt = alternate


class State(Enum):
	none = 0
	uniform = 1
	ina = 2
	out = 3


class Process:
	lf = 0xA
	space = 0x20
	pound = 0x23
	asterisk = 0x2A
	period = 0x2E
	slash = 0x2F
	dg_0 = 0x30
	dg_9 = 0x39
	up_a = 0x41
	up_z = 0x5A
	underscore = 0x5F
	lo_a = 0x61
	lo_z = 0x7A
	reserved = [
		b'__FILE__',
		b'__LINE__',
		b'__VERSION__',
		b'abs',
		b'acos',
		b'acosh',
		b'active',
		b'all',
		b'any',
		b'asin',
		b'asinh',
		b'asm',
		b'atan',
		b'atanh',
		b'attribute',
		b'bool',
		b'break',
		b'bvec2',
		b'bvec3',
		b'bvec4',
		b'case',
		b'cast',
		b'ceil',
		b'centroid',
		b'clamp',
		b'class',
		b'common',
		b'const',
		b'continue',
		b'cos',
		b'cosh',
		b'cross',
		b'dFdx',
		b'dFdy',
		b'default',
		b'degrees',
		b'discard',
		b'distance',
		b'do',
		b'dot',
		b'double',
		b'dvec2',
		b'dvec3',
		b'dvec4',
		b'else',
		b'enum',
		b'equal',
		b'exp',
		b'exp2',
		b'extern',
		b'external',
		b'faceforward',
		b'false',
		b'filter',
		b'fixed',
		b'flat',
		b'float',
		b'floor',
		b'for',
		b'fract',
		b'ftransform',
		b'fvec2',
		b'fvec3',
		b'fvec4',
		b'fwidth',
		b'goto',
		b'greaterThan',
		b'greaterThanEqual',
		b'half',
		b'highp',
		b'hvec2',
		b'hvec3',
		b'hvec4',
		b'if',
		b'iimage1D',
		b'iimage1DArray',
		b'iimage2D',
		b'iimage2DArray',
		b'iimage3D',
		b'iimageBuffer',
		b'iimageCube',
		b'image1D',
		b'image1DArray',
		b'image1DArrayShadow',
		b'image1DShadow',
		b'image2D',
		b'image2DArray',
		b'image2DArrayShadow',
		b'image2DShadow',
		b'image3D',
		b'imageBuffer',
		b'imageCube',
		b'in',
		b'inline',
		b'inout',
		b'input',
		b'int',
		b'interface',
		b'invariant',
		b'inversesqrt',
		b'isampler1D',
		b'isampler1DArray',
		b'isampler2D',
		b'isampler2DArray',
		b'isampler3D',
		b'isamplerCube',
		b'isinf',
		b'isnan',
		b'ivec2',
		b'ivec3',
		b'ivec4',
		b'length',
		b'lessThan',
		b'lessThanEqual',
		b'log',
		b'log2',
		b'long',
		b'lowp',
		b'main',
		b'mat2',
		b'mat2x2',
		b'mat2x3',
		b'mat2x4',
		b'mat3',
		b'mat3x2',
		b'mat3x3',
		b'mat3x4',
		b'mat4',
		b'mat4x2',
		b'mat4x3',
		b'mat4x4',
		b'matrixCompMult',
		b'max',
		b'mediump',
		b'min',
		b'mix',
		b'mod',
		b'modf',
		b'namespace',
		b'noinline',
		b'noise1',
		b'noise2',
		b'noise3',
		b'noise4',
		b'noperspective',
		b'normalize',
		b'not',
		b'notEqual',
		b'out',
		b'outerProduct',
		b'output',
		b'packed',
		b'partition',
		b'pow',
		b'precision',
		b'public',
		b'radians',
		b'reflect',
		b'refract',
		b'return',
		b'round',
		b'roundEven',
		b'row_major',
		b'sampler1D',
		b'sampler1DArray',
		b'sampler1DArrayShadow',
		b'sampler1DShadow',
		b'sampler2D',
		b'sampler2DArray',
		b'sampler2DArrayShadow',
		b'sampler2DRect',
		b'sampler2DRectShadow',
		b'sampler2DShadow',
		b'sampler3D',
		b'sampler3DRect',
		b'samplerBuffer',
		b'samplerCube',
		b'shadow1D',
		b'shadow1DLod',
		b'shadow1DProj',
		b'shadow1DProjLod',
		b'shadow2D',
		b'shadow2DLod',
		b'shadow2DProj',
		b'shadow2DProjLod',
		b'short',
		b'sign',
		b'sin',
		b'sinh',
		b'sizeof',
		b'smooth',
		b'smoothstep',
		b'sqrt',
		b'static',
		b'step',
		b'struct',
		b'superp',
		b'switch',
		b'tan',
		b'tanh',
		b'template',
		b'texelFetch',
		b'texelFetchOffset',
		b'texture',
		b'texture1D',
		b'texture1DLod',
		b'texture1DProj',
		b'texture1DProjLod',
		b'texture2D',
		b'texture2DLod',
		b'texture2DProj',
		b'texture2DProjLod',
		b'texture3D',
		b'texture3DLod',
		b'texture3DProj',
		b'texture3DProjLod',
		b'textureCube',
		b'textureCubeLod',
		b'textureGrad',
		b'textureGradOffset',
		b'textureLod',
		b'textureLodOffset',
		b'textureOffset',
		b'textureProj',
		b'textureProjGrad',
		b'textureProjGradOffset',
		b'textureProjLod',
		b'textureProjLodOffset',
		b'textureProjOffset',
		b'textureSize',
		b'this',
		b'transpose',
		b'true',
		b'trunc',
		b'typedef',
		b'uimage1D',
		b'uimage1DArray',
		b'uimage2D',
		b'uimage2DArray',
		b'uimage3D',
		b'uimageBuffer',
		b'uimageCube',
		b'uint',
		b'uniform',
		b'union',
		b'unsigned',
		b'usampler1D',
		b'usampler1DArray',
		b'usampler2D',
		b'usampler2DArray',
		b'usampler3D',
		b'usamplerCube',
		b'using',
		b'uvec2',
		b'uvec3',
		b'uvec4',
		b'varying',
		b'vec2',
		b'vec3',
		b'vec4',
		b'void',
		b'volatile',
		b'while'
	]
	src_dir: str
	dst_dir: str
	release: bool
	inputs: list[VarInfo] = []
	globs: dict[bytes, bytes]

	def __init__(self):
		self.src_dir = os.path.join(os.path.dirname(__file__), 'shaders')
		self.dst_dir = os.path.join(os.path.dirname(__file__), os.pardir, 'src', 'engine', 'shaders')
		self.release = len(sys.argv) <= 1

	def process(self, vert_file: str, frag_file: str):
		try:
			self.globs = {}
			for it in [vert_file, frag_file]:
				if self.release:
					self.reduce_source(it)
				else:
					self.copy_source(it)
		except Exception as e:
			print(e)

	def reduce_source(self, name: str):
		with open(os.path.join(self.src_dir, name), 'r') as fh:
			text = bytearray(fh.read(), 'ascii')

		state = State.none
		vertex = name[name.find('.') + 1:] == 'vert'
		pname = name[2:name.find('.')].upper()
		lrname = b''
		self.words = dict(self.globs)
		i = 0
		while i < len(text):
			if text[i] <= self.space:	# remove/squash spaces
				e = i + 1
				while e < len(text) and text[e] <= self.space:
					e += 1
				del text[i:e]
				if (i > 0 and self.check_text(text[i - 1])) and self.check_text(text[i]):
					text.insert(i, self.space)
				else:
					i -= 1
			elif text[i] == self.pound:	# skip preprocessor directive
				e = text.find(self.lf, i + 1)
				i = e if e != -1 else len(text) - 1
			elif text[i] == self.slash:	# skip comment
				if i + 1 < len(text):
					if text[i + 1] == self.slash:
						e = text.find(self.lf, i + 2)
						del text[i:e]
						i -= 1
					elif text[i + 1] == self.asterisk:
						e = text.find(b'*/', i)
						e = e + 2 if e != -1 else len(text)
						del text[i:e]
						i -= 1
			elif text[i] == self.period:	# skip word after period
				while i + 1 < len(text) and self.check_text(text[i + 1]):
					i += 1
			elif self.check_text(text[i]):	# skip/replace word
				e = i + 1
				while e < len(text) and self.check_text(text[e]):
					e += 1
				word = bytes(text[i:e])
				if self.dg_0 <= word[0] <= self.dg_9 or word[:3].upper() == b'GL_' or word in self.reserved:
					if word == b'uniform':
						state = State.uniform
					elif word == b'in':
						state = State.ina if vertex else State.none
					elif word == b'out':
						state = State.out
					i = e - 1
				else:
					if word in self.words:
						alias = self.words[word]
					else:
						lrname = self.next_name(lrname)
						self.words[word] = lrname
						alias = lrname
						if state != State.none:
							if state == State.uniform or state == State.ina:
								prefix = 'UNI' if state == State.uniform else 'ATTR'
								vname = str(word, 'ascii')
								self.inputs.append(VarInfo(f'{prefix}_{pname}_{vname.upper()}', vname, str(alias, 'ascii')))
							self.globs[word] = alias
							state = State.none
					text[i:e] = alias
					i += len(alias) - 1
			i += 1

		self.write_text(f'{os.path.join(self.dst_dir, name)}.rel.h', str(text, 'ascii'))

	def check_text(self, ch: int) -> bool:
		return self.dg_0 <= ch <= self.dg_9 or self.up_a <= ch <= self.up_z or self.lo_a <= ch <= self.lo_z or ch == self.underscore

	def next_name(self, name: bytes) -> bytes:
		nchars = b'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_'
		while True:
			if name[:3].upper() != b'GL9' or any(c != nchars[-1] for c in name[3:]):
				i = len(name) - 1
				while i >= 0 and name[i] == nchars[-1]:
					i -= 1

				if i >= 0:
					ni = nchars.find(name[i]) + 1 if i > 0 or name[i] != nchars[-12] else -1
					name = name[:i] + nchars[ni:ni + 1] + (nchars[:1] * (len(name) - i - 1))
				else:
					name = nchars[:1] * (len(name) + 1)
			else:
				ni = nchars.find(name[1]) + 1
				name = name[:1] + nchars[ni:ni + 1] + (nchars[:1] * (len(name) - 2))

			if not (name in self.reserved or name in self.globs.values() or name in self.words.values()):
				return name

	def write_macros(self):
		if self.release:
			self.inputs.sort(key=lambda v: v.name)
			with open(os.path.join(self.dst_dir, 'glDefs.h'), 'w') as fh:
				fh.write('#pragma once\n\n#ifdef NDEBUG\n')
				for it in self.inputs:
					fh.write(f'#define {it.name} "{it.alt}"\n')
				fh.write('#else\n')
				for it in self.inputs:
					fh.write(f'#define {it.name} "{it.ori}"\n')
				fh.write('#endif\n')

	def copy_source(self, name: str):
		with open(os.path.join(self.src_dir, name), 'r') as fh:
			self.write_text(f'{os.path.join(self.dst_dir, name)}.dbg.h', fh.read().strip())

	def write_text(self, file: str, text: str):
		with open(file, 'w') as fh:
			fh.write('R"r(')
			fh.write(text)
			fh.write(')r"\n')


if __name__ == '__main__':
	proc = Process()
	for (vt, ft) in [('glGui.vert', 'glGui.frag')]:
		proc.process(vt, ft)
	proc.write_macros()
