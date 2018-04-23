from __future__ import division
import database as db


def get_changes():
    '''
    return: [(index, value)]
            where index is index of cards array and value is the value to be set
    '''
    se = db.Session()
    changed_cells = se.query(db.Cell).filter(db.Cell.new_state != None).all()
    # changed_cells = se.query(db.Cell).filter(db.Cell.state != 'free').all()
    if len(changed_cells) == 0:
        return []
    nums_to_load = []
    byte_indexes = []
    for i in changed_cells:
        start = i.no - i.no % 8
        byte_indexes.append(start // 8)
        nums_to_load.extend(range(start, start + 8))
    nums_to_load = sorted(list(set(nums_to_load)))
    byte_indexes = sorted(list(set(byte_indexes)))
    needed_cells = se.query(db.Cell).filter(db.Cell.no.in_(nums_to_load)).all()
    by_no = {i.no: i for i in needed_cells}
    result = []
    for byte_index in byte_indexes:
        start = byte_index * 8
        values = []
        for i in range(start, start + 8):
            cell = by_no[i]
            val = cell.state if cell.new_state is None else cell.new_state
            values.append(1 if val == 'on' else 0)
        values.reverse()
        values = ''.join(map(str, values))
        value = int(values, 2)
        result.append((byte_index, value))
    return result


def get_unit(unit_id):
    se = db.Session()
    cells = se.query(db.Cell).filter_by(unit_id=unit_id).all()


def checksum(l):
    c = 0
    for i, v in enumerate(l):
        c ^= (v + i) & 0xff
    return c & 0xff


def make_block(l):
    n = len(l)
    r = []
    for byte_index, value in l:
        index_val = 0x8000 | byte_index
        r.append(index_val >> 8)
        r.append(index_val & 0xff)
        r.append(value)
    while len(r) < 15:
        r.append(0)
    r.append(checksum(r))
    return r


def empty_block():
    r = 15 * [0]
    r.append(checksum(r))
    return r


def hex_block(b):
    # return '{%s}' % ', '.join(map(str, b))
    return '{%s}' % ', '.join(map(lambda v: '%02x' % v, b))
    return '{%s}' % ', '.join(map(lambda v: '0x%02x' % v, b))


def get_blocks(data=None):
    res = []
    if data is None:
        data = get_changes()
    if len(data) == 0:
        return res
    # data = [(0, 0xf0), (1, 0xf1), (2, 0xf2),  (3, 0xf3),  (4, 0xf4),
    #         (5, 0xf5), (6, 0xf6), (7, 0xf7),  (8, 0xf8),  (9, 0xf9)]
    # data = [(23, 31)]
    for j in range(0, len(data), 5):
        l = data[j:j + 5]
        b = make_block(l)
        res.append(b)
    res.append(empty_block())
    return res


def main():
    l = get_blocks()
    print l
    for i in l:
        print hex_block(i)


if __name__ == '__main__':
    main()
