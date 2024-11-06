import argparse
import enum
import os
import shutil
import sys
import subprocess

PROGRESS_FILENAME = 'progress.txt'
NUM_OPTIONS = 8
CNT_OPTIONS = 2 ** NUM_OPTIONS

class Option(enum.IntFlag):
	SDL3	= 0x01,
	MISC	= 0x02,	# ICU, fontconfig, libarchive and renderers
	NET		= 0x04,	# FTP, SFTP and SMB
	GNUTLS	= 0x08,	# only with NET and makefile
	OPENSSL	= 0x10,	# only with NET and makefile
	SECRET	= 0x20,	# only with NET and Linux
	POPPLER	= 0x40,	# only with makefile
	MUPDF	= 0x80	# only for linux


class State(enum.IntEnum):
	OFF = 0,
	ON = 1,
	ANY = 2


class RunResult:
	iden: Option
	msg: str

	def __init__(self, opts: Option, args: list[str], ex: Exception):
		self.iden = opts
		self.msg = f'\n{opts}\n{args}\n{ex}'


def init_progress(opt_mask: list[State], failed_opts: list[Option], opt_set: list[Option], args, base_dir: str):
	with open(os.path.join(base_dir, PROGRESS_FILENAME), 'w') as fh:
		fh.write(f'Generator: {args.generator}\nBuild type: {args.build_type}\nJobs: {args.jobs}\nMask: ')
		fh.write(', '.join(f'{Option(1 << i).name}={opt_mask[i].name}' for i in range(len(opt_mask))))
		fh.write('\n')
		if failed_opts:
			fh.write('\nPrevious fails:\n')
			for fit in failed_opts:
				fh.write(f'{fit:02X}\n')
		if opt_set:
			fh.write('\nOptions:\n')
			for i in range(len(opt_set)):
				fh.write(f'{i}: {opt_set[i]:02X} {opt_set[i]:08b}\n')
		fh.write(f'\nProgress:\n0/{len(opt_set)}\n')


def update_progress(progress: int, total: int, base_dir: str):
	with open(os.path.join(base_dir, PROGRESS_FILENAME), 'a') as fh:
		fh.write(f'{progress}/{total}\n')


def write_fails(idens: list[Option], base_dir: str):
	try:
		with open(os.path.join(base_dir, PROGRESS_FILENAME), 'w') as fh:
			fh.write('\nFailed:\n')
			for iit in idens:
				fh.write(f'{iit:02X}\n')
	except Exception as ex:
		print(ex)


def read_fails(base_dir: str) -> list[Option]:
	try:
		with open(os.path.join(base_dir, PROGRESS_FILENAME), 'r') as fh:
			for line in fh:
				if line.strip() == 'Failed:':
					return [Option(int(line.strip(), 16)) for line in fh]
	except Exception:
		pass
	return []


def get_mask(text: str) -> list[State]:
	try:
		return [State(int(text[i])) for i in range(min(len(text), NUM_OPTIONS))]
	except Exception:
		return []


def get_generator_info(name: str) -> tuple[bool, bool]:
	with_mingw = name and (name.startswith('MinGW') or name.startswith('MSYS'))
	return (os.name == 'nt' and not with_mingw, with_mingw)


def gen_options(opt_mask: list[State], failed_opts: list[Option], generator: str) -> list[Option]:
	def is_valid(opt: Option) -> bool:
		if with_mingw and opt & Option.SDL3:
			return False
		if not opt & Option.NET and opt & (Option.GNUTLS | Option.OPENSSL | Option.SECRET):
			return False
		if with_msvc and opt & (Option.GNUTLS | Option.OPENSSL | Option.POPPLER):
			return False
		if os.name == 'nt' and opt & (Option.SECRET | Option.MUPDF):
			return False
		if any(opt_mask[c] != State.ANY and (opt_mask[c] == State.ON) != bool(opt & (1 << c)) for c in range(min(len(opt_mask), NUM_OPTIONS))):
			return False
		return not failed_opts or opt in failed_opts

	(with_msvc, with_mingw) = get_generator_info(generator)
	return [i for i in range(CNT_OPTIONS) if is_valid(i)]


def get_cmake_opt(name: str, on: bool) -> str:
	return f'-D{name}={"ON" if on else "OFF"}'


def run_cmake(cmd_cmake: str, opts: Option, base_dir: str, args) -> RunResult:
	with_msvc = get_generator_info(args.generator)[0]
	build_name = (('sdl3_' if opts & Option.SDL3 else 'sdl2_')
		+ ('misc_' if opts & Option.MISC else 'mini_')
		+ ('net_' if opts & Option.NET else 'loc_')
		+ ('g' if opts & Option.GNUTLS else 'x')
		+ ('o' if opts & Option.OPENSSL else 'x')
		+ ('s' if opts & Option.SECRET else 'x')
		+ ('p' if opts & Option.POPPLER else 'x')
		+ ('m' if opts & Option.MUPDF else 'x'))

	cmake_opts = [cmd_cmake, os.path.join(os.pardir, os.pardir)]
	if args.compiler:
		cmake_opts.append(f'-DCMAKE_CXX_COMPILER={args.compiler}')
	if args.generator:
		cmake_opts += ['-G', args.generator]
	if args.build_type:
		cmake_opts.append(f'-DCMAKE_BUILD_TYPE={args.build_type}')
	cmake_opts += [
		get_cmake_opt('SDL3', opts & Option.SDL3),
		get_cmake_opt('USE_ICU', opts & Option.MISC),
		get_cmake_opt('USE_ARCHIVE', opts & Option.MISC),
		get_cmake_opt('OPENGL', opts & Option.MISC),
		get_cmake_opt('VULKAN', opts & Option.MISC),
		get_cmake_opt('FORCE_INLINE_SHADERS', opts & Option.MISC),
		get_cmake_opt('FTP', opts & Option.NET)
	]
	if not with_msvc:
		cmake_opts += [
			get_cmake_opt('USE_FONTCONFIG', opts & Option.MISC),
			get_cmake_opt('USE_GNUTLS', opts & Option.GNUTLS),
			get_cmake_opt('USE_OPENSSL', opts & Option.OPENSSL),
			get_cmake_opt('USE_POPPLER', opts & Option.POPPLER)
		]
	if os.name == 'nt':
		cmake_opts.append(get_cmake_opt('DIRECT3D', opts & Option.MISC))
	else:
		cmake_opts += [
			get_cmake_opt('USE_SMBCLIENT', opts & Option.NET),
			get_cmake_opt('USE_SSH2', opts & Option.NET),
			get_cmake_opt('USE_SECRET', opts & Option.SECRET),
			get_cmake_opt('USE_MUPDF', opts & Option.MUPDF)
		]

	build_opts = [cmd_cmake, '--build', os.curdir]
	if with_msvc:
		if not args.build_type:
			build_opts += ['--config', 'Release']
	elif args.jobs and args.jobs > 1:
		build_opts += ['-j', str(args.jobs)]

	try:
		build_dir = os.path.join(base_dir, build_name)
		if os.path.isdir(build_dir):
			shutil.rmtree(build_dir)
		os.mkdir(build_dir)
		os.chdir(build_dir)
		subprocess.run(cmake_opts).check_returncode()
		subprocess.run(build_opts).check_returncode()
		return None
	except Exception as ex:
		return RunResult(opts, cmake_opts, ex)


if __name__ == '__main__':
	cmake_command = shutil.which('cmake')
	if not cmake_command:
		raise RuntimeError('Failed to find cmake')
	bdir = os.path.join(os.path.dirname(__file__), os.pardir, 'build_tests')
	if not os.path.isdir(bdir):
		os.mkdir(bdir)

	parser = argparse.ArgumentParser()
	parser.add_argument('-c', '--compiler')
	parser.add_argument('-g', '--generator')
	parser.add_argument('-j', '--jobs', type=int)
	parser.add_argument('-m', '--mask')
	parser.add_argument('-t', '--build_type')
	args = parser.parse_args()
	if not args.generator:
		if os.name == 'nt':
			if shutil.which('nmake'):
				args.generator = 'NMake Makefiles'
			elif shutil.which('mingw32-make'):
				args.generator = 'MinGW Makefiles'
		else:
			if shutil.which('ninja'):
				args.generator = 'Ninja'
			elif shutil.which('make'):
				args.generator = 'Unix Makefiles'
	omask = get_mask(args.mask)
	pfails = read_fails(bdir)
	option_combinations = gen_options(omask, pfails, args.generator)

	errors = []
	init_progress(omask, pfails, option_combinations, args, bdir)
	for i in range(len(option_combinations)):
		if cr := run_cmake(cmake_command, option_combinations[i], bdir, args):
			errors.append(cr)
		update_progress(i + 1, len(option_combinations), bdir)

	if errors:
		write_fails([it.iden for it in errors], bdir)
		print(f'\nErrors:\n{os.linesep.join(it.msg for it in errors)}')
	else:
		print('\nAll good')
