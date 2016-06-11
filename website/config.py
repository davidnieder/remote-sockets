# -*- coding: utf-8 -*-


class Config(object):
    DEBUG = True
    SECRET_KEY = 'not so secret'
    LANGUAGES = 'en', 'de'


class ConfigProduction(Config):
    DEBUG = False
    SECRET_KEY = 'little more random and secret'
