from random import randint


def encrypt(data):
    first_byte = randint(0, 255)
    use_low_nibble = first_byte % 2 == 0
    res = [first_byte]
    for i in data:
        l_nib = i & 0x0f
        h_nib = (i & 0xf0) >> 4
        nib_rnd0 = randint(0, 15)
        nib_rnd1 = randint(0, 15)
        if use_low_nibble:
            l_byte = (nib_rnd0 << 4) | l_nib
            h_byte = (nib_rnd1 << 4) | h_nib
        else:
            l_byte = (l_nib << 4) | nib_rnd0
            h_byte = (h_nib << 4) | nib_rnd1
        res.extend([h_byte ^ first_byte, l_byte ^ first_byte])
    return res


def decrypt(data):
    first_byte = data[0]
    use_low_nibble = first_byte % 2 == 0
    res = []
    for i in range(1, len(data), 2):
        h_byte = data[i] ^ first_byte
        l_byte = data[i + 1] ^ first_byte
        if use_low_nibble:
            l_nib = l_byte & 0x0f
            h_nib = h_byte & 0x0f
        else:
            l_nib = l_byte >> 4
            h_nib = h_byte >> 4
        val = (h_nib << 4) | l_nib
        res.append(val)
    return res


def test():
    while True:
        data = [randint(0, 255) for i in range(randint(0, 20))]
        print data
        cr = encrypt(data)
        print cr
        de = decrypt(cr)
        print de
        print de == data
        assert de == data


if __name__ == '__main__':
    data = [1, 2, 3, 4]
    for i in range(30):
        print ['%02x' % i for i in encrypt(data)]
    # test()
