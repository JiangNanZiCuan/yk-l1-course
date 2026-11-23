"""Add the independent coursework modules to the pinned Padavan source tree."""
from pathlib import Path
import shutil
import sys

payload = Path(__file__).resolve().parent
trunk = Path(sys.argv[1]).resolve()
kernel = trunk / 'linux-3.4.x'
for name in ('guess_game.c', 'guess_logic.h'):
    shutil.copyfile(payload / name, kernel / 'drivers/misc' / name)
p = kernel / 'drivers/misc/Makefile'
p.write_text(p.read_text() + '\n# Coursework loadable module\nobj-m += guess_game.o\n')
app = trunk / 'user/guess_game'
app.mkdir()
for name in ('guess_client.c', 'Makefile', 'guess_game_init.sh'):
    shutil.copyfile(payload / name, app / name)
(app / 'guess_game_init.sh').chmod(0o755)
p = trunk / 'user/Makefile'
text = p.read_text()
assert text.count('\nall:\n') == 1
p.write_text(text.replace('\nall:\n', '\ndir_y += guess_game\n\nall:\n'))
# Keep the module and application installed, but require manual loading while
# hardware startup is being diagnosed. Enable autostart only after validation.
if '--autostart' in sys.argv[2:]:
    p = trunk / 'user/rc/rc.c'
    text = p.read_text()
    anchor = '\tsystem("/etc/storage/started_script.sh &");'
    assert text.count(anchor) == 1
    p.write_text(text.replace(anchor, '\tsystem("/sbin/guess_game_init.sh &");\n' + anchor))
    print('Integrated game files with firmware startup hook.')
else:
    print('Integrated game files; manual launch: /sbin/guess_game_init.sh')
