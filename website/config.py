# -*- coding: utf-8 -*-


class Config(object):
    DEBUG = True
    SECRET_KEY = 'not so secret'


class ConfigProduction(Config):
    DEBUG = False
    SECRET_KEY = 'little more random and secret'
