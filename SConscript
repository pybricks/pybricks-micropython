from glob import glob

Import('env')

# TODO: This should be fixed to play nicer with scons, I think.
font = env.Command('_font.h', ['_font.h.base', 'font.8x5.png'],
                   './scripts/generate_fonts.py base/font.8x5.png '
                   'base/_font.h.base base/_font.h')

for source in glob('*.[cS]')+glob('drivers/*.[cS]'):
    obj = env.Object(source.split('.')[0], source)
    env.Append(NXOS_BASEPLATE=obj)
    if source == 'display.c':
        env.Depends(obj, font)

# env.Doxygen('Doxyfile')
