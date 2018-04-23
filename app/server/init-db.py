import database as db


unit_names = [
    'unit-11-1', 'unit-12-2', 'unit-13-3', 'unit-14-4', 'unit-21-5', 'unit-22-6', 'unit-23-7', 'unit-24-8',
    'unit-31-9', 'unit-32-10', 'unit-33-11', 'unit-34-12', 'unit-41-13', 'unit-42-14', 'unit-43-15', 'unit-44-16',
    'unit-51-17', 'unit-52-18', 'unit-53-19', 'unit-54-20', 'unit-61-21', 'unit-62-22', 'unit-63-23', 'unit-64-24',
    'unit-71-25', 'unit-72-26', 'unit-73-27', 'unit-74-28', 'unit-81-29', 'unit-82-30', 'unit-83-31', 'unit-84-32',
    'unit-91-33', 'unit-92-34', 'unit-93-35', 'unit-94-36', 'unit-101-37', 'unit-102-38', 'unit-103-39', 'unit-104-40',
]
num_of_cells = 64 * 8

se = db.Session()
se.query(db.Cell).delete()
se.query(db.Unit).delete()
se.commit()
for name in unit_names:
    x = db.Unit(name=name, permission=False, new_permission=None)
    se.add(x)
for i in range(num_of_cells):
    x = db.Cell(no=i, state='free')
    se.add(x)
se.commit()
