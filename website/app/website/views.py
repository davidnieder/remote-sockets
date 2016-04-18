# -*- coding: utf-8 -*-

from flask import render_template, request, redirect, url_for
from flask.ext.login import current_user, login_required
from flask.ext.login import login_user, logout_user
from werkzeug.security import check_password_hash

from ..models import User
from ..logger import get_task_log
from . import website


@website.route('/')
@login_required
def index():
    task_log = get_task_log(current_user.name)
    return render_template('index.html', task_log=task_log)

@website.route('/login', methods=['GET', 'POST'])
def login():
    if request.method == 'GET':
        return render_template('login.html')

    username = request.form.get('username')
    password = request.form.get('password')
    remember = request.form.get('remember') == 'true'
    user = User.load(username)

    if user and check_password_hash(user.password, password):
        login_user(user, remember=remember)
        return redirect(url_for('.index'))
    return render_template('login.html', error=True)

@website.route('/logout')
@login_required
def logout():
    logout_user()
    return redirect(url_for('.login'))

