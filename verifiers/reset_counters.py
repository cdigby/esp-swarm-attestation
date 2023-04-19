from pathlib import Path
import shutil

dirpath = Path('counters')
if dirpath.exists() and dirpath.is_dir():
    shutil.rmtree(dirpath)
    print('Counters reset')

else:
    print('Counters do not exist')