# -*- coding: utf-8 -*-

from __future__ import unicode_literals

from datetime import datetime
from flask.ext.login import UserMixin

from . import database as db
from .remote import Remote


class User(UserMixin):
    def __init__(self, name, password, admin, client_secret, server_secret,
                 remote_addr, sockets=[], tasks=[]):
        self.name = name
        self.password = password
        self.admin = admin
        self.client_secret = client_secret
        self.server_secret = server_secret
        self.remote_addr = remote_addr

        for socket in sockets:
            pass
        for task in tasks:
            pass

        self.sockets_cached = None
        self.tasks_cached = None

    def __repr__(self):
        return '<User "{}">'.format(self.name)

    def get_id(self):
        return unicode(self.name)

    @property
    def sockets(self):
        if self.sockets_cached:
            return self.sockets_cached
        self.sockets_cached = Socket.user_sockets(self.name)
        return self.sockets_cached

    @property
    def socket_numbers(self):
        return [s.number for s in self.sockets]

    @property
    def schedule(self):
        if self.tasks_cached:
            return self.tasks_cached
        self.tasks_cached = Task.user_tasks(self.name)
        return self.tasks_cached

    def update(self, name=None, password=None, admin=None, client_secret=None,
               server_secret=None, remote_addr=None):
        name = name or self.name
        password = password or self.password
        admin = admin or self.admin
        client_secret = client_secret or self.client_secret
        server_secret = server_secret or self.server_secret
        remote_addr = remote_addr or self.remote_addr

        con = db.connect()
        query = '''
                UPDATE users
                SET    name=(?), password=(?), admin=(?), client_secret=(?),
                       server_secret=(?), remote_addr=(?)
                WHERE  name=(?);
                '''
        con.execute(query, (name, password, admin, client_secret, server_secret,
                            remote_addr, self.name))
        con.commit()
        #TODO set only if operation successful
        self.name = name
        self.password = password
        self.admin = admin
        self.client_secret = client_secret
        self.server_secret = server_secret
        self.remote_addr = remote_addr

    def save(self):
        con = db.connect()
        query = '''
                INSERT
                INTO   users(name, password, admin, client_secret,
                             server_secret, remote_addr)
                       values(?,?,?,?,?,?);
                '''
        con.execute(query, (self.name, self.password, self.admin,
                            self.client_secret, self.server_secret,
                            self.remote_addr))
        con.commit()

    @staticmethod
    def load(name):
        con = db.connect()
        query = '''
                SELECT password,admin,client_secret,server_secret,remote_addr
                FROM   users
                WHERE  name=(?);
                '''
        row = con.execute(query, (name,)).fetchone()
        if row:
            return User(name, row[0], row[1], row[2], row[3], row[4])
        return None

    @staticmethod
    def load_all():
        con = db.connect()
        query = '''
                SELECT name,password,admin,client_secret,server_secret,
                       remote_addr
                FROM   users;
                '''
        rows = con.execute(query).fetchall()
        users = []
        for row in rows:
            users.append(User(row[0], row[1], row[2], row[3], row[4], row[5]))
        return users


class Socket(object):
    def __init__(self, number, description, user_id, id=0):
        self.number = number
        self.description = description
        self.user_id = user_id
        self.id = id

    def __repr__(self):
        return '<Socket #{}, user: "{}">'.format(self.id, self.user_id)

    def update(self, number=None, description=None, user_id=None):
        number = number or self.number
        description = description or self.description
        user_id = user_id or self.user_id

        con = db.connect()
        query = '''
                UPDATE  sockets
                SET     nr=(?), description=(?), user_id=(?)
                WHERE   id=(?);
                '''
        con.execute(query, (number, description, user_id, self.id))
        con.commit()
        #TODO only set if execute was successful
        self.number = number
        self.description = description
        self.user_id = user_id

    def save(self):
        con = db.connect()
        cur = con.cursor()
        query = '''
                INSERT
                INTO   sockets(nr, description, user_id)
                       values(?,?,?);
                '''
        cur.execute(query, (self.number, self.description, self.user_id))
        con.commit()
        self.id = cur.lastrowid

    @staticmethod
    def load(socket_id):
        con = db.connect()
        query = '''
                SELECT nr, description, user_id
                FROM   sockets
                WHERE  id=(?);
                '''
        row = con.execute(query, (socket_id,)).fetchone()
        if row:
            return Socket(row[0], row[1], row[2], socket_id)
        return None

    @staticmethod
    def user_sockets(user_id):
        con = db.connect()
        query = '''
                SELECT   nr, description, id
                FROM     sockets
                WHERE    user_id=(?)
                ORDER BY nr ASC;
                '''
        rows = con.execute(query, (user_id,)).fetchall()
        sockets = []
        for row in rows:
            sockets.append(Socket(row[0], row[1], user_id, row[2]))
        return sockets

    @staticmethod
    def create_sockets(user_id, amount=3):
        for socket_nr in range(1, amount+1):
            Socket(socket_nr, 'Socket #{}'.format(socket_nr), user_id).save()


class Task(object):
    def __init__(self, time, active, user_id, turn_on=[], turn_off=[], id=0):
        self.time = time
        self.active = active
        self.user_id = user_id
        self.turn_on = turn_on
        self.turn_off = turn_off
        self.id = id

    def __repr__(self):
        return '<Task #{}, user: "{}">'.format(self.id, self.user_id)

    def update(self):
        con = db.connect()
        query = '''
                UPDATE tasks
                SET    time=(?), active=(?), turn_on=(?), turn_off=(?)
                WHERE  id=(?) AND user_id=(?);
                '''
        con.execute(query, (self.time, self.active, list2str(self.turn_on),
                            list2str(self.turn_off), self.id, self.user_id))
        con.commit()

    def save(self):
        if self.id:
            self.update()
            return

        con = db.connect()
        cur = con.cursor()
        query = '''
                INSERT
                INTO   tasks(time, active, turn_on, turn_off, user_id)
                       values(?,?,?,?,?);
                '''
        turn_on = list2str(self.turn_on)
        turn_off = list2str(self.turn_off)
        cur.execute(query, (self.time, self.active, turn_on, turn_off,
                            self.user_id))
        con.commit()
        self.id = cur.lastrowid

    @property
    def is_due(self):
        hour, minute = self.time.split(':')
        now = datetime.utcnow().replace(second=0, microsecond=0)
        task_time = now.replace(hour=int(hour), minute=int(minute))
        return now == task_time

    def execute(self):
        user = User.load(self.user_id)
        remote = Remote.from_user(user)
        if self.turn_on:
            return remote.send_command(self.turn_on, 'on')
        if self.turn_off:
            return remote.send_command(self.turn_off, 'off')

    def validate(self):
        # validate 'time'
        try:
            datetime.strptime(self.time, '%H:%M')
        except:
            return False
        # validate 'active'
        if self.active not in [False, True]:
            return False
        # TODO validate 'turn_on'/'turn_off', user_id(?)

        return True

    @staticmethod
    def load(task_id):
        con = db.connect()
        query = '''
                SELECT time, active, turn_on, turn_off, user_id
                FROM   tasks
                WHERE  id=(?);
                '''
        row = con.execute(query, (task_id,)).fetchone()
        if row:
            turn_on = str2list(row[2])
            turn_off = str2list(row[3])
            return Task(row[0], bool(row[1]), row[4], turn_on, turn_off,
                        task_id)
        return None

    @staticmethod
    def load_all():
        con = db.connect()
        query = '''
                SELECT time, active, user_id, turn_on, turn_off, id
                FROM   tasks;
                '''
        rows = con.execute(query).fetchall()
        tasks = []
        for row in rows:
            tasks.append(Task(row[0], row[1], row[2], str2list(row[3]),
                              str2list(row[4]), row[5]))
        return tasks

    @staticmethod
    def delete(task_id):
        con = db.connect()
        query = '''
                DELETE
                FROM   tasks
                WHERE  id=(?);
                '''
        con.execute(query, (task_id,))
        con.commit()

    @staticmethod
    def user_tasks(user_id):
        con = db.connect()
        query = '''
                SELECT time, active, turn_on, turn_off, id
                FROM   tasks
                WHERE  user_id=(?);
                '''
        rows = con.execute(query, (user_id,)).fetchall()
        tasks = []
        for row in rows:
            turn_on = str2list(row[2])
            turn_off = str2list(row[3])
            tasks.append(Task(row[0], bool(row[1]), user_id,  turn_on, turn_off,
                              row[4]))
        return tasks

    @staticmethod
    def from_json(json, user_id):
        return Task(json.get('time'), json.get('active'), user_id,
                json.get('turnOn'), json.get('turnOff'), json.get('taskId'))


# type conversion for sqlite
def list2str(l):
    return ','.join(str(e) for e in l)

def str2list(s):
    return map(int, s.split(',')) if s else []
