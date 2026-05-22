from sqlalchemy import Column, Integer, String, Float, DateTime, LargeBinary
from database import Base
import datetime

class LapTime(Base):
    __tablename__ = "lap_times"

    id = Column(Integer, primary_key=True, index=True)
    player = Column(String, index=True)
    player_id = Column(String, index=True, nullable=True) # Unique ID for the player
    map_id = Column(String, index=True)
    time = Column(Float, index=True)
    date = Column(DateTime, default=datetime.datetime.utcnow)
    ghost_data = Column(LargeBinary, nullable=True)
