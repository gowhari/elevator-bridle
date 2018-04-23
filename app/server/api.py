import flask_restless
import server
import database as db


def register_blueprints(app):
    app.register_blueprint(server.bp, url_prefix='')
    db_session = db.Session()
    rest = flask_restless.APIManager(app, session=db_session)
    all_methods = ['GET', 'POST', 'DELETE', 'PUT', 'PATCH']
    rest.create_api(db.Unit, url_prefix='', methods=all_methods, results_per_page=None)
    rest.create_api(db.Cell, url_prefix='', methods=all_methods, results_per_page=None)
