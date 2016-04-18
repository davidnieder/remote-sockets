# -*- coding: utf-8 -*-

from time import time

from . import database as db


query_insert = '''
        INSERT
        INTO    logs(task_id, user_id, timestamp, success, response)
                values(?,?,?,?,?);
        '''

query_get = '''
        SELECT   task_id, user_id, timestamp, success, response
        FROM     logs
        WHERE    user_id=(?)
        ORDER BY id DESC
        LIMIT    (?)
        OFFSET   (?)
        '''

def log_task(task, success, response):
    con = db.connect()
    cur = con.cursor()

    timestamp = time()
    cur.execute(query_insert, (task.id, task.user_id, timestamp, success, response))
    con.commit()

def get_task_log(user_id, amount=20, offset=0):
    con = db.connect()
    cur = con.cursor()

    rows = cur.execute(query_get, (user_id, amount, offset)).fetchall()
    return [dict(row) for row in rows]

