# -*- coding: utf-8 -*-

import site
site.addsitedir('/var/www/iot/remote-sockets')
site.addsitedir('/var/www/iot/remote-sockets/venv/lib/python2.7/site-packages')

from app import create_app
from config import ConfigProduction

application = create_app(ConfigProduction)
