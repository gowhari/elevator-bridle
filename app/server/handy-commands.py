import hw as hw_module


key_ff = [0xff, 0xff, 0xff, 0xff, 0xff, 0xff]
key_123 = [1, 2, 3, 4, 5, 6]
key_main = [90, 206, 26, 227, 18, 6]


hw = hw_module.Hw()
while not hw.ping():
    print 'no ping'


def find_valid_key():
    for k in [key_ff, key_main, key_123]:
        hw.use_auth_key(k)
        try:
            hw.read_card_block(1)
            return k
        except hw_module.Error, err:
            pass
    raise hw_module.Error('key not found')


# read block 1
def read_block_one():
    hw.use_auth_key(key_main)
    s = hw.read_card_block(1)
    s = map(ord, s)
    import crypto
    res = crypto.decrypt(s[:15])
    print res
    print 'type = %d   no = %d' % (res[0], res[1] * 256 + res[2])


def mk_unit_card():
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
    hw.write_unit_card(5)
    print 'create unit card: done'


def mk_beep_freq_cards(change):
    assert change in ('-', '+')
    print 'card read...'
    card_key = find_valid_key()
    print 'card key:', card_key
    if card_key != key_main:
        print 'change key...'
        print 'card read...'
        hw.set_card_auth_keys(key_main)
        print 'card key has been set to main'
        print 'use key main:'
        hw.use_auth_key(key_main)
        print 'ok'
    print 'card read...'
    command = {'+': 5, '-': 6}
    l = hw.mk_first_block(2, command[change])
    hw.write_card_block(1, l)
    print 'done'


def mk_update_card(command_type):
    '''
    1: update
    2: erase and update
    7: toggle
    '''
    assert command_type in (1, 2, 7)
    hw.use_auth_key(key_main)
    # print hw.dump_card_data()
    # exit()
    print 'card read...'
    card_key = find_valid_key()
    print 'card key:', card_key
    if card_key != key_main:
        print 'change key...'
        print 'card read...'
        hw.set_card_auth_keys(key_main)
        print 'card key has been set to main'
        print 'use key main:'
        hw.use_auth_key(key_main)
        print 'ok'
    print 'card read...'
    l = hw.mk_first_block(2, command_type)
    hw.write_card_block(1, l)
    print 'done'


def set_update_card_data():
    # hw.use_auth_key(key_main)
    import writer
    blocks = writer.get_blocks()
    print blocks
    # exit()
    block_no = 4
    for block_data in blocks:
        print 'writing to block %d: %s' % (block_no, block_data)
        hw.write_card_block(block_no, block_data)
        print 'done'
        block_no += 1
        if block_no % 4 == 3:
            block_no += 1


def set_toggle_card_data(unit_name):
    import writer
    import database as db
    se = db.Session()
    unit = se.query(db.Unit).filter(db.Unit.name.like(unit_name + '-%')).one()
    cells = se.query(db.Cell).filter_by(unit_id=unit.id).all()
    cell_nums = [i.no for i in cells]
    assert all([int(i / 8) == unit.id for i in cell_nums])
    val = sum([2 ** (i % 8) for i in cell_nums])
    data = [(unit.id, val)]
    blocks = writer.get_blocks(data)
    block_no = 4
    for block_data in blocks:
        print 'writing to block %d: %s' % (block_no, block_data)
        hw.write_card_block(block_no, block_data)
        print 'done'
        block_no += 1
        if block_no % 4 == 3:
            block_no += 1


# read_block_one()
# mk_update_card(1)
# mk_beep_freq_cards('-')
# set_toggle_card_data('unit-83')
# set_update_card_data()
