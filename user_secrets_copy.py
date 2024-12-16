import os
from shutil import copyfile

if not os.path.exists('USER_SECRETS.ini'):
    copyfile('USER_SECRETS_TEMPLATE.ini', 'USER_SECRETS.ini')