import base64
# Based on https://www.safaribooksonline.com/library/view/python-cookbook/0596001673/ch09s06.html

def base64_image(filename):
    with open(filename, 'rb') as in_file:
        guts = in_file.read()
    encoded = base64.encodebytes(guts)
    return encoded.decode("utf-8")


def do_image(filename, var_name, file):
    print('\n# Base64 encoding of {}'.format(filename), file=out_file)
    text = base64_image(filename)
    print('{} = """\n{}"""\n'.format(var_name, text), file=out_file)


with open('../man_tool/coweeta_icons.py', 'w') as out_file:
    print('# Base64 encoding of icon files', file=out_file)
    print('# Built by {}'.format(__file__), file=out_file)

    do_image('little-us-forest-service-logo.gif', 'usfs', file=out_file)
    do_image('salamander-logo.gif', 'salamandar', file=out_file)


