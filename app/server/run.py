import config
from serverbase import app


if __name__ == '__main__':
    app.run('0.0.0.0', config.flask.port, debug=config.flask.debug)
