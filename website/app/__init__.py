# -*- coding: utf-8 -*-

from flask import Flask, request, g
from flask.ext.login import LoginManager
from flask.ext.babel import Babel

from .models import User


def create_app(config):
    app = Flask(__name__)
    app.config.from_object(config)

    login_manager = LoginManager()
    login_manager.init_app(app)
    login_manager.login_view = 'website.login'

    @login_manager.user_loader
    def load_user(user_id):
        return User.load(user_id)

    @app.before_request
    def on_request():
        g.db = database.connect()

    @app.teardown_request
    def on_teardown(exception):
        db = getattr(g, 'db', None)
        if db is not None:
            db.close()

    @app.after_request
    def after_request_calls(response):
        for callback in getattr(g, 'after_request_callbacks', ()):
            callback(response)
        return response


    babel = Babel(app)

    @babel.localeselector
    def select_locale():
        if 'lang' in request.args:
            lang = request.args.get('lang')
            @call_after_request
            def set_cookie(request):
                request.set_cookie('lang', lang, 60*60*24*31*12)
            return lang

        return request.cookies.get('lang') or \
               request.accept_languages.best_match(app.config.get('LANGUAGES'))


    from .website import website
    app.register_blueprint(website)

    from .api import api
    app.register_blueprint(api, url_prefix='/api')

    return app


def call_after_request(func):
    if not hasattr(g, 'after_request_callbacks'):
        g.after_request_callbacks = []
    g.after_request_callbacks.append(func)
    return func
