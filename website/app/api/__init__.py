# -*- coding: utf-8 -*-

from flask import Blueprint, abort
from flask.ext.login import current_user


api = Blueprint('api', __name__)

@api.before_request
def login_required():
    if not current_user.is_authenticated:
        abort(403)


from . import endpoints
