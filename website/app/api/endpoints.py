# -*- coding: utf-8 -*-

from flask import request, jsonify

from flask.ext.login import login_required, current_user

from ..models import User, Socket, Task
from ..logger import get_task_log
from ..remote import Remote
from . import api



@api.route('/switch', methods=['POST'])
def switch_socket():
    action = request.get_json().get('action')
    socket = request.get_json().get('socket')

    if action in ['on', 'off'] and socket in current_user.socket_numbers:
        remote = Remote(current_user.remote_addr, current_user.client_secret,
                     current_user.server_secret)
        status, response = remote.send_command([socket], action)

        if status:
            return jsonify(status='ok', response=response)
        return jsonify(status='failure', response=response)

    return jsonify(status='failure', response='argument error')


@api.route('/ping-device')
def ping_device():
    remote = Remote(current_user.remote_addr, current_user.client_secret,
                 current_user.server_secret)
    status, response = remote.send_ping()

    if status:
        return jsonify(status='ok', respone=response)
    return jsonify(status='failure', response=response)


@api.route('/change-description', methods=['POST'])
def change_description():
    socket_id = request.get_json().get('socket')
    description = request.get_json().get('description')

    socket = Socket.load(socket_id)
    if socket and socket.user_id == current_user.name:
        socket.update(description=description)
        return jsonify(status='ok', socket=socket_id, description=socket.description)

    return jsonify(status='failure', error='argument error')


@api.route('/save-task', methods=['POST'])
def save_task():
    json = request.get_json()
    task = Task.from_json(json, current_user.name)
    if task.validate():
        task.save()
        return jsonify(status='ok', taskId=task.id)
    return jsonify(status='failure')


@api.route('/delete-task', methods=['POST'])
def delete_task():
    task = Task.load(request.get_json().get('taskId'))
    if task and task.user_id == current_user.name:
        Task.delete(task.id)
        return jsonify(status='ok', taskId=task.id)
    return jsonify(status='failure', taskId=task.id)

@api.route('/get-task-log')
def get_log():
    return jsonify(task_list=get_task_log(current_user.name))

