# encoding: utf8

import time
import json
import logging
from flask import Blueprint, request, make_response, g
import hw as hw_module
import config
import database as db
from serverbase import return_json, ErrorToClient


bp = Blueprint('server', __name__)
logger = logging.getLogger()
hw = hw_module.Hw()


key_ff = [0xff, 0xff, 0xff, 0xff, 0xff, 0xff]
key_123 = [1, 2, 3, 4, 5, 6]
key_main = [90, 206, 26, 227, 18, 6]


def find_valid_key():
    for k in [key_ff, key_main, key_123]:
        hw.use_auth_key(k)
        try:
            hw.read_card_block(1)
            return k
        except hw_module.Error, err:
            pass
    raise hw_module.Error('key not found')


def fill_card(cell_id):
    while not hw.ping():
        print 'no ping'
    print 'card read...'
    card_uid = hw.get_card_uid()
    print 'card uid: %s' % card_uid
    print 'card read...'
    card_key = find_valid_key()
    print 'card key:', card_key
    if card_key != key_main:
        print 'card read...'
        hw.set_card_auth_keys(key_main)
        print 'card key has been set to main'
        print 'use key main:'
        hw.use_auth_key(key_main)
        print 'ok'
    print 'card read...'
    hw.fill_card_random()
    print 'card filled random'
    print 'card read...'
    hw.write_unit_card(cell_id)
    print 'create unit card: done'
    return card_uid


@bp.route('/assign-cell-to-unit/<int:unit_id>/<int:cell_id>', methods=['POST'])
def assign_cell_to_unit(unit_id, cell_id):
    print 'unit=%d cell=%d' % (unit_id, cell_id)
    cell = g.db_session.query(db.Cell).filter_by(id=cell_id).one()
    unit = g.db_session.query(db.Unit).filter_by(id=unit_id).one()
    card_uid = fill_card(cell.no)
    cell.unit = unit
    cell.state = 'off'
    cell.card = card_uid
    g.db_session.commit()
    return 'ok'


@bp.route('/apply-new-states', methods=['POST'])
def _apply_new_states():
    cells = g.db_session.query(db.Cell).filter(db.Cell.new_state != None).all()
    units = g.db_session.query(db.Unit).filter(db.Unit.new_permission != None).all()
    for i in cells:
        i.state = i.new_state
        i.new_state = None
    for i in units:
        i.permission = i.new_permission
        i.new_permission = None
    g.db_session.commit()
    return 'ok'
