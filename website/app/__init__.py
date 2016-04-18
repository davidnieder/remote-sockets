# -*- coding: utf-8 -*-

from flask import Flask, g
from flask.ext.login import LoginManager

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


    from .website import website
    app.register_blueprint(website)

    from .api import api
    app.register_blueprint(api, url_prefix='/api')

    return app
