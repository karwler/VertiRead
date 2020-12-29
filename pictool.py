import os
import sys
import wand.api
import wand.color
import wand.image

def convert_image(dst: str, filepath: str, save_ico: bool = False) -> None:
	try:
		with wand.image.Image() as img:
			with wand.color.Color('transparent') as bgcolor:
				wand.api.library.MagickSetBackgroundColor(img.wand, bgcolor.resource)
			img.read(filename=filepath)

			filebase = os.path.join(dst, os.path.splitext(os.path.basename(filepath))[0])
			with open(filebase + '.png', 'wb') as out:
				out.write(img.make_blob('png8'))
			if save_ico:
				with open(filebase + '.ico', 'wb') as out:
					out.write(img.make_blob('ico'))
	except Exception as ex:
		print(f'"{filepath}": {ex}')

if __name__ == '__main__':
	if len(sys.argv) == 2:
		convert_image(os.path.dirname(sys.argv[1]), sys.argv[1], True)
	elif len(sys.argv) > 2:
		for file in sys.argv[2:]:
			convert_image(sys.argv[1], file)
	else:
		print('USAGE:\n', __file__, '<file>\n', __file__, '<destination> <files>')
