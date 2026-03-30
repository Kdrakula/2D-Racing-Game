from fastapi import FastAPI, Depends, HTTPException
from pydantic import BaseModel
from sqlalchemy.orm import Session
import datetime
import os

import models
from database import engine, get_db

# Create the database tables
models.Base.metadata.create_all(bind=engine)

app = FastAPI(title="Leaderboard API")

class LapTimeCreate(BaseModel):
    player: str
    map_id: str
    time: float

class LapTimeResponse(BaseModel):
    player: str
    time: float
    date: str

@app.post("/api/laptime", status_code=200)
def receive_laptime(lap: LapTimeCreate, db: Session = Depends(get_db)):
    db_lap = models.LapTime(
        player=lap.player, 
        map_id=lap.map_id,
        time=lap.time, 
        date=datetime.datetime.now()
    )
    db.add(db_lap)
    db.commit()
    print(f"[API] New Lap Record! Player: {lap.player} | Map: {lap.map_id} | Time: {lap.time}s | Date: {db_lap.date.strftime('%Y-%m-%d %H:%M:%S')}")
    return {"status": "success", "message": "Lap time saved successfully!"}

@app.get("/api/laptimes", response_model=list[LapTimeResponse])
def get_laptimes(map_id: str = None, db: Session = Depends(get_db)):
    # Retrieve top 10 fastest times
    query = db.query(models.LapTime)
    if map_id:
        query = query.filter(models.LapTime.map_id == map_id)
    
    records = query.order_by(models.LapTime.time.asc()).limit(10).all()
    # Format to match the old C++ expectations
    # The datetime needs to be a string like "YYYY-MM-DD HH:MM:SS"
    return [
        {
            "player": r.player, 
            "time": r.time, 
            "date": r.date.strftime("%Y-%m-%d %H:%M:%S")
        } for r in records
    ]
