import json
import gzip
import logging
import StringIO
from functools import wraps
from flask import Flask, request, session, redirect, g
import config
import database as db


app = Flask(__name__,
            static_url_path=config.flask.static_url_path,
            static_folder=config.flask.static_folder)
app.config['SEND_FILE_MAX_AGE_DEFAULT'] = 1
app.secret_key = config.flask.secret_key
logger = logging.getLogger()


class ErrorToClient(Exception):
    pass


@app.errorhandler(ErrorToClient)
def error_to_client(error):
    logger.debug('ErrorToClient: %s', error.args)
    return json.dumps({'code': error.args[0], 'args': error.args[1:]}), 400


@app.before_request
def before_request():
    g.db_session = db.Session()


@app.after_request
def after_request(response):
    return gzip_content(response)


@app.teardown_request
def teardown_request(exception):
    db_session = getattr(g, 'db_session', None)
    if db_session is not None:
        db_session.close()


@app.route('/')
def _index():
    return redirect('/static/index.html')


def gzip_content(response):
    response.direct_passthrough = False
    accept_encoding = request.headers.get('Accept-Encoding', '')
    if 'gzip' not in accept_encoding.lower():
        return response
    if (200 > response.status_code >= 300) or len(response.data) < 500 or 'Content-Encoding' in response.headers:
        return response
    gzip_buffer = StringIO.StringIO()
    gzip_file = gzip.GzipFile(mode='wb', compresslevel=6, fileobj=gzip_buffer)
    gzip_file.write(response.data)
    gzip_file.close()
    response.data = gzip_buffer.getvalue()
    response.headers['Content-Encoding'] = 'gzip'
    response.headers['Content-Length'] = len(response.data)
    return response


def return_json(f):
    @wraps(f)
    def inner(*a, **k):
        r = f(*a, **k)
        return json.dumps(r)
    return inner


import api
api.register_blueprints(app)
