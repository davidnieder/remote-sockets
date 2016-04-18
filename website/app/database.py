# -*- coding: utf-8 -*-

import os
import sqlite3

from flask import g


PATH = os.path.dirname(os.path.abspath(__file__))
DATABASE = PATH + '/socket.db'

def connect():
    if hasattr(g, 'db'):
        return g.db

    con = sqlite3.connect(DATABASE)
    con.row_factory = sqlite3.Row
    #con.execute('PRAGMA foreign_keys=on;')
    return con

def create():
    con = connect()
    con.executescript(schemata['users'])
    con.executescript(schemata['sockets'])
    con.executescript(schemata['tasks'])
    con.executescript(schemata['logs'])
    con.commit()
    con.close()


schemata = {
    'users':
        ''' CREATE TABLE users  (
            name TEXT PRIMARY KEY NOT NULL UNIQUE,
            password TEXT NOT NULL,
            admin BOOLEAN NOT NULL,
            client_secret TEXT NOT NULL,
            server_secret TEXT NOT NULL,
            remote_addr TEXT NOT NULL);
        ''',
    'sockets':
        ''' CREATE TABLE sockets (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            nr INTEGER NOT NULL,
            description TEXT NOT NULL,
            user_id INTEGER NOT NULL,
            FOREIGN KEY(user_id) REFERENCES users(name));
        ''',
    'tasks':
        ''' CREATE TABLE tasks  (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            time TEXT NOT NULL,
            active INTEGER NOT NULL,
            turn_on TEXT NOT NULL,
            turn_off TEXT NOT NULL,
            user_id TEXT NOT NULL,
            FOREIGN KEY(user_id) REFERENCES users(name));
        ''',
    'logs':
        ''' CREATE TABLE logs   (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            task_id INTEGER NOT NULL,
            user_id TEXT NOT NULL,
            timestamp REAL NOT NULL,
            success BOOLEAN NOT NULL,
            response TEXT NOT NULL,
            FOREIGN KEY(task_id) REFERENCES tasks(id),
            FOREIGN KEY(user_id) REFERENCES users(name));
        '''
}
