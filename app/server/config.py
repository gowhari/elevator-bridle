import os
import logging


Obj = lambda: lambda: None


_abs = os.path.abspath
_join = os.path.join
path = Obj()
path.root = _abs(_join(os.path.dirname(__file__), os.pardir))
path.ui = _join(path.root, 'ui')
path.server = _join(path.root, 'server')

log = Obj()
log.filename = None
log.format = '%(asctime)s %(levelname)s %(filename)s:%(lineno)d: %(message)s'
log.level = logging.DEBUG


db = Obj()
db.url = 'sqlite:///%s/db.sqlite' % path.server

flask = Obj()
flask.secret_key = 'dev-secret-key'
flask.port = 5000
flask.debug = True
flask.static_url_path = '/static'
flask.static_folder = path.ui

try:
    import local_config
except ImportError:
    pass


if log.filename is None:
    log_handler = logging.StreamHandler()
else:
    log_handler = logging.handlers.WatchedFileHandler(log.filename)
log_handler.setFormatter(logging.Formatter(log.format))
root_logger = logging.getLogger()
root_logger.setLevel(log.level)
root_logger.addHandler(log_handler)
logging.getLogger('werkzeug').setLevel(logging.INFO)
