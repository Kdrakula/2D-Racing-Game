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

import base64
from typing import Optional

class LapTimeCreate(BaseModel):
    player: str
    map_id: str
    time: float
    ghost: Optional[str] = None # Base64 encoded ghost data

class LapTimeResponse(BaseModel):
    player: str
    time: float
    date: str
    ghost: Optional[str] = None # Base64 encoded ghost data for world record

@app.post("/api/laptime", status_code=200)
def receive_laptime(lap: LapTimeCreate, db: Session = Depends(get_db)):
    ghost_binary = None
    if lap.ghost:
        try:
            ghost_binary = base64.b64decode(lap.ghost)
        except Exception as e:
            print(f"[API] Error decoding ghost: {e}")

    db_lap = models.LapTime(
        player=lap.player, 
        map_id=lap.map_id,
        time=lap.time, 
        date=datetime.datetime.now(),
        ghost_data=ghost_binary
    )
    db.add(db_lap)
    db.commit()
    print(f"[API] New Lap Record! Player: {lap.player} | Map: {lap.map_id} | Time: {lap.time}s | Ghost: {'Yes' if ghost_binary else 'No'}")
    return {"status": "success", "message": "Lap time saved successfully!"}

@app.get("/api/laptimes", response_model=list[LapTimeResponse])
def get_laptimes(map_id: str = None, db: Session = Depends(get_db)):
    # Retrieve top 10 fastest times
    query = db.query(models.LapTime)
    if map_id:
        query = query.filter(models.LapTime.map_id == map_id)
    
    records = query.order_by(models.LapTime.time.asc()).limit(10).all()
    
    response = []
    for i, r in enumerate(records):
        item = {
            "player": r.player, 
            "time": r.time, 
            "date": r.date.strftime("%Y-%m-%d %H:%M:%S"),
            "ghost": None
        }
        # Only include ghost data for the #1 record to save bandwidth
        if i == 0 and r.ghost_data:
            item["ghost"] = base64.b64encode(r.ghost_data).decode('utf-8')
            
        response.append(item)
    
    return response
