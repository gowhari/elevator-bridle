import sqlalchemy as sa
import sqlalchemy.orm as saorm
import sqlalchemy.ext.declarative
from sqlalchemy import event
import config


Base = sqlalchemy.ext.declarative.declarative_base()
db_engine = sa.create_engine(config.db.url, echo=False, encoding='utf-8', pool_recycle=3600)
Session = saorm.sessionmaker(bind=db_engine)


class Unit(Base):
    __tablename__ = 'unit'
    id = sa.Column(sa.Integer, autoincrement=True, primary_key=True)
    name = sa.Column(sa.String(128), nullable=False)
    permission = sa.Column(sa.Boolean, nullable=False)
    new_permission = sa.Column(sa.Boolean, nullable=True, default=None)

    def __repr__(self):
        return '%s(%d)' % (self.__class__.__name__, self.id or -1)


class Cell(Base):
    __tablename__ = 'cell'
    id = sa.Column(sa.Integer, autoincrement=True, primary_key=True)
    no = sa.Column(sa.Integer, nullable=False)
    state = sa.Column(sa.String(16), default='free')
    new_state = sa.Column(sa.String(16), default=None)
    card = sa.Column(sa.String(16))
    unit_id = sa.Column(sa.Integer, sa.ForeignKey('unit.id'), nullable=True)
    unit = saorm.relationship('Unit')

    def __repr__(self):
        return '%s(%d)' % (self.__class__.__name__, self.id or -1)


Base.metadata.create_all(db_engine)
