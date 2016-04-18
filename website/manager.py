#!venv/bin/python

from __future__ import unicode_literals, print_function

from os import urandom
from getpass import getpass
from binascii import hexlify, unhexlify

from werkzeug.security import generate_password_hash
from flask.ext.script import Manager, Command

from app import create_app, database
from app.models import User, Socket

from config import Config


manager = Manager(create_app(Config))

@manager.command
def createdb():
    database.create()

@manager.command
def adduser(admin=False):
    """ Adds a user to the database """
    name = raw_input('Name: ')
    if User.load(name) is not None:
        print('User "{}" already exists.'.format(name))
        return 1

    pw = getpass('Password: ')
    pw_confirm = getpass('Confirm: ')
    if pw != pw_confirm:
        print('Passwords do not match')
        return 1

    pw = generate_password_hash(pw)
    remote_addr = raw_input('Remote device (host:port): ')
    client_secret = raw_input('Client secret (empty to generate): ')
    server_secret = raw_input('Server secret (empty to generate): ')
    client_secret = client_secret or hexlify(urandom(32))
    server_secret = server_secret or hexlify(urandom(32))

    user = User(name, pw, admin, client_secret, server_secret, remote_addr)
    user.save()
    Socket.create_sockets(user.name)

@manager.command
def listusers():
    users = User.load_all()
    for nr,user in zip(range(1, len(users)+1), users):
        print('{}. {} ({})'.format(nr, user.name, user.remote_addr))

@manager.command
def showsecrets(username):
    """ Shows shared secrets for a user """
    user = User.load(username)
    if user is None:
        print('No such user: "{}"'.format(username))
        return 1
    else:
        print('Client secret: ' + hexlify(user.client_secret))
        print('Server secret: ' + hexlify(user.server_secret))

@manager.command
def setsecrets(username):
    """ Sets new shared secrets of a user """
    user = User.load(username)
    if user is None:
        print('No such user: "{}"'.format(username))
        return 1

    client_secret = raw_input('client secret: ')
    server_secret = raw_input('server secret: ')
    user.update(client_secret=client_secret, server_secret=server_secret)

@manager.command
def setaddress(username):
    """ Sets the esp8266/raspberry-pi address of a user """
    user = User.load(username)
    if user is None:
        print('No such user: "{}"'.format(username))
        return 1

    addr = raw_input('address (addr:port): ')
    user.update(remote_addr=addr)


if __name__ == '__main__':
    manager.run()
