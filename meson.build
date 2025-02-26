project(
	'VertiRead',
	'cpp',
	default_options : ['cpp_std=c++20', 'warning_level=3'],
	meson_version : '>=1.4.0',
	version : '1.0.0'
)
compiler = meson.get_compiler('cpp')
dir_data = target_machine.kernel() == 'nt' ? meson.project_name() : 'share' / meson.project_name().to_lower()

args = []
if ['gcc','clang', 'intel', 'intel-llvm'].contains(compiler.get_id())
	args += [
		'-Wformat=2', '-Wmissing-declarations', '-Wold-style-cast', '-Wshadow', '-Wunreachable-code',
		'-Wno-implicit-fallthrough', '-Wno-missing-field-initializers', '-Wno-switch'
	]
	if compiler.get_id() == 'gcc'
		args += ['-Wcast-align=strict', '-Wduplicated-cond', '-Wlogical-op', '-Wuseless-cast']
	else
		args += ['-Wcast-align', '-Wnull-dereference', '-Wshadow-all', '-Wno-final-dtor-non-final-class']
	endif
	if target_machine.kernel() == 'nt'
		args += ['-Wno-cast-function-type', '-Wno-unused-parameter', '-Wno-unused-variable']
	endif
endif

src_files = [
	'src/engine/drawSys.cpp',
	'src/engine/drawSys.h',
	'src/engine/fileSys.cpp',
	'src/engine/fileSys.h',
	'src/engine/inputSys.cpp',
	'src/engine/inputSys.h',
	'src/engine/renderer.cpp',
	'src/engine/renderer.h',
	'src/engine/scene.cpp',
	'src/engine/scene.h',
	'src/engine/windowSys.cpp',
	'src/engine/windowSys.h',
	'src/engine/world.cpp',
	'src/engine/world.h',
	'src/engine/optional/internal.h',
	'src/prog/browser.cpp',
	'src/prog/browser.h',
	'src/prog/fileOps.cpp',
	'src/prog/fileOps.h',
	'src/prog/program.cpp',
	'src/prog/program.h',
	'src/prog/progs.cpp',
	'src/prog/progs.h',
	'src/prog/types.cpp',
	'src/prog/types.h',
	'src/utils/compare.cpp',
	'src/utils/compare.h',
	'src/utils/layouts.cpp',
	'src/utils/layouts.h',
	'src/utils/sdlCompat.h',
	'src/utils/settings.cpp',
	'src/utils/settings.h',
	'src/utils/sthandle.h',
	'src/utils/stvector.h',
	'src/utils/utils.cpp',
	'src/utils/utils.h',
	'src/utils/widgets.cpp',
	'src/utils/widgets.h'
]
if get_option('direct3d') and target_machine.kernel() == 'nt' and compiler.has_header('d3d11.h')
	src_files += [
		'src/engine/rendererDx.cpp',
		'src/engine/rendererDx.h',
		'src/engine/optional/d3d.cpp',
		'src/engine/optional/d3d.h'
	]
	args += '-DWITH_DIRECT3D'

	dx_shaders = ['dxGuiVs', 'dxGuiPs', 'dxFinVs', 'dxFinPs', 'dxRgbCs', 'dxBgrCs', 'dxRedCs', 'dxIdxCs']
	cmd_fxc = find_program('fxc', required : false)
	if not get_option('force_inline_shaders') and cmd_fxc.found()
		foreach dfile : dx_shaders
			configure_file(
				command : [cmd_fxc, '/Ges', '/Gis', '/T', dfile.substring(-2).to_lower() + '_5_0'] + (get_option('debug') ? ['/Od', '/Zi'] : ['/O1']) + ['/Fo', '@OUTPUT@', '@INPUT@'],
				input : 'rsc/shaders' / dfile + '.hlsl',
				install : true,
				install_dir : dir_data / 'shaders',
				output : dfile + '.fxc'
			)
		endforeach
		args += '-DEXT_DIRECT3D_SHADERS'
	else
		foreach dfile : dx_shaders
			src_files += 'src/engine/shaders' / dfile + (get_option('debug') ? '.dbg.h' : '.rel.h')
		endforeach
	endif
endif
if get_option('opengl')
	src_files += [
		'src/engine/rendererGl.cpp',
		'src/engine/rendererGl.h',
		'src/engine/shaders/glDefs.h'
	]
	args += '-DWITH_OPENGL'

	gl_shaders = ['glGui.vert', 'glGui.frag', 'glFin.vert', 'glFin.frag']
	if get_option('debug')
		cmd_bash = find_program('bash')
		cmd_sed = find_program('sed')
		nl_ch = target_machine.kernel() == 'nt' ? '\\\\\\r\\\\\\n' : '\\\\\\n'
		foreach gfile : gl_shaders
			gfile_out = 'src/engine/shaders' / gfile + '.dbg.h'
			run_command(cmd_bash, '-c', '\'' + cmd_sed.full_path() + '\' \'s/^/"/\' \'' + files('rsc/shaders' / gfile).get(0).full_path() + '\' | \'' + cmd_sed.full_path() + '\' \'s/$/' + nl_ch + '"/\' > \'' + (meson.global_source_root() / gfile_out) + '\'', check : true)
			src_files += gfile_out
		endforeach
	else
		foreach gfile : gl_shaders
			src_files += 'src/engine/shaders' / gfile + '.rel.h'
		endforeach
	endif
endif
if get_option('vulkan') and compiler.has_header('vulkan/vulkan.h')
	src_files += [
		'src/engine/rendererVk.cpp',
		'src/engine/rendererVk.h'
	]
	args += '-DWITH_VULKAN'

	vk_shaders = ['vkGui.vert', 'vkGui.frag', 'vkFin.vert', 'vkFin.frag', 'vkRgb.comp', 'vkIdx.comp']
	cmd_glslc = find_program('glslc', required : false)
	if not get_option('force_inline_shaders') and cmd_glslc.found()
		foreach vfile : vk_shaders
			configure_file(
				command : [cmd_glslc, '--target-env=vulkan1.0', '--target-spv=spv1.0', get_option('debug') ? '-g' : '-O', '-o', '@OUTPUT@', '@INPUT@'],
				input : 'rsc/shaders' / vfile,
				install : true,
				install_dir : dir_data / 'shaders',
				output : vfile + '.spv'
			)
		endforeach
		args += '-DEXT_VULKAN_SHADERS'
	else
		foreach vfile : vk_shaders
			src_files += 'src/engine/shaders' / vfile + (get_option('debug') ? '.dbg.h' : '.rel.h')
		endforeach
	endif
endif
if get_option('ftp')
	src_files += [
		'src/engine/network.cpp',
		'src/engine/network.h',
		'src/prog/fileOpsFtp.cpp',
		'src/prog/fileOpsFtp.h'
	]
	args += '-DWITH_FTP'
endif
if get_option('fontconfig') and compiler.has_header('fontconfig/fontconfig.h')
	src_files += [
		'src/engine/optional/fontconfig.cpp',
		'src/engine/optional/fontconfig.h'
	]
	args += '-DCAN_FONTCFG'
endif
if get_option('mupdf') and target_machine.kernel() != 'nt' and compiler.has_header('mupdf/fitz.h')
	src_files += [
		'src/engine/optional/mupdf.cpp',
		'src/engine/optional/mupdf.h'
	]
	args += '-DCAN_MUPDF'
endif

pkgconf = find_program('pkgconf', required : false)
if pkgconf.found()
	may_secret = false
	need_glib = false

	if get_option('ftp')
		if get_option('gnutls')
			rs = run_command(pkgconf, '--cflags', 'gnutls', check : false)
			if rs.returncode() == 0
				src_files += [
					'src/engine/optional/gnutls.cpp',
					'src/engine/optional/gnutls.h'
				]
				args += rs.stdout().strip().split() + '-DCAN_GNUTLS'
			else
				message(rs.stderr())
			endif
		endif
		if get_option('openssl')
			rs = run_command(pkgconf, '--cflags', 'openssl', check : false)
			if rs.returncode() == 0
				src_files += [
					'src/engine/optional/openssl.cpp',
					'src/engine/optional/openssl.h'
				]
				args += rs.stdout().strip().split() + '-DCAN_OPENSSL'
			else
				message(rs.stderr())
			endif
		endif
	endif
	if target_machine.kernel() != 'nt'
		if get_option('smbclient')
			rs = run_command(pkgconf, '--cflags', 'smbclient', check : false)
			if rs.returncode() == 0
				src_files += [
					'src/engine/optional/smbclient.cpp',
					'src/engine/optional/smbclient.h',
					'src/prog/fileOpsSmb.cpp',
					'src/prog/fileOpsSmb.h'
				]
				args += rs.stdout().strip().split() + '-DCAN_SMB'
				may_secret = true
			else
				message(rs.stderr())
			endif
		endif
		if get_option('ssh2')
			rs = run_command(pkgconf, '--cflags', 'libssh2', check : false)
			if rs.returncode() == 0
				src_files += [
					'src/engine/optional/ssh2.cpp',
					'src/engine/optional/ssh2.h',
					'src/prog/fileOpsSftp.cpp',
					'src/prog/fileOpsSftp.h'
				]
				args += rs.stdout().strip().split() + '-DCAN_SFTP'
				may_secret = true
			else
				message(rs.stderr())
			endif
		endif
		if get_option('secret') and (get_option('ftp') or may_secret)
			rs = run_command(pkgconf, '--cflags', 'libsecret-1', check : false)
			if rs.returncode() == 0
				secret_args = rs.stdout().strip().split()
				need_glib = true
			else
				message(rs.stderr())
			endif
		endif
	endif
	if get_option('poppler')
		rs = run_command(pkgconf, '--cflags', 'poppler-glib', check : false)
		if rs.returncode() == 0
			poppler_args = rs.stdout().strip().split()
			need_glib = true
		else
			message(rs.stderr())
		endif
	endif
	if need_glib
		src_files += [
			'src/engine/optional/glib.cpp',
			'src/engine/optional/glib.h'
		]
	endif
endif
if get_option('sdl3')
	args += rs.stdout().strip().split() + '-DWITH_SDL3'
endif

if target_machine.kernel() == 'nt'
	windows = import('windows', required : false)
	if windows.found()
		src_files += windows.compile_resources('rsc/resource.rc')
	endif
	args += ['-DUNICODE', '-D_UNICODE', '-D_CRT_SECURE_NO_WARNINGS', '-DNOMINMAX']
endif

deps = [
	dependency('freetype2', include_type : 'system'),
	dependency('glm', include_type : 'system'),
	dependency(get_option('sdl3') ? 'sdl3' : 'sdl2', include_type : 'system'),
	dependency(get_option('sdl3') ? 'sdl3_image' : 'sdl2_image', include_type : 'system')
]
if not compiler.has_header('format')
	args += '-DWITH_LIBFMT'
	deps += dependency('fmt', include_type : 'system')
endif
if get_option('archive')
	dep_archive = dependency('libarchive', include_type : 'system', required : false)
	if dep_archive.found()
		args += '-DWITH_ARCHIVE'
		deps += dep_archive
	endif
endif
if get_option('icu')
	dep_icuuc = dependency('icu-uc', include_type : 'system', required : false)
	dep_icuin = dependency('icu-i18n', include_type : 'system', required : false)
	if dep_icuuc.found() and dep_icuin.found()
		args += '-DWITH_ICU'
		deps += [dep_icuuc, dep_icuin]
	endif
endif

links = []
if target_machine.kernel() == 'linux'
	links += ['-lpthread', '-ldl']
endif
if get_option('ftp') and target_machine.kernel() == 'nt'
	links += '-lws2_32'
endif

exec = executable(
	target_machine.kernel() == 'nt' ? meson.project_name() : meson.project_name().to_lower(),
	src_files,
	cpp_args : args,
	dependencies : deps,
	include_directories : 'src',
	install : true,
	install_dir : target_machine.kernel() == 'nt' ? dir_data : 'bin',
	link_args : links,
	native : get_option('native'),
	win_subsystem : 'windows'
)

install_subdir('rsc/icons', install_dir : dir_data)
install_data('rsc' / meson.project_name().to_lower() + '.svg', install_dir : dir_data / 'icons')
install_data('rsc/BrisaSans.otf', 'rsc/themes.ini', install_dir : dir_data)
install_data('rsc/licenses/Copying.BrisaSans.txt', install_dir : dir_data / 'licenses')

if target_machine.kernel() == 'nt' and ['gcc','clang', 'intel', 'intel-llvm'].contains(compiler.get_id())
	cmd_bash = find_program('bash', required : false)
	cmd_ldd = find_program('ldd', required : false)
	cmd_grep = find_program('grep', required : false)
	cmd_xargs = find_program('xargs', required : false)
	cmd_cp = find_program('cp', required : false)
	if cmd_bash.found() and cmd_ldd.found() and cmd_grep.found() and cmd_xargs.found() and cmd_cp.found()
		inst_dir = get_option('prefix') / dir_data
		meson.add_install_script(cmd_bash, '-c', '\'' + cmd_ldd.full_path() + '\' \'' + (inst_dir / exec.name()) + '.exe' + '\' | \'' + cmd_grep.full_path() + '\' -E \'/(clang|mingw|ucrt)[0-9]+/.*\\.dll\' -o | \'' + cmd_xargs.full_path() + '\' -I{} \'' + cmd_cp.full_path() + '\' \'{}\' \'' + inst_dir + '\'')
	endif
endif
