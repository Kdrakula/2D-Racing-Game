import os
from sqlalchemy import create_engine
from sqlalchemy.orm import sessionmaker, declarative_base

# The DATABASE_URL will be passed from docker-compose
# By default we fall back to a local sqlite if not provided (for local non-docker testing)
SQLALCHEMY_DATABASE_URL = os.environ.get("DATABASE_URL", "sqlite:///./sql_app.db")

# For the docker setup it's postgres. We don't need check_same_thread for postgres.
if SQLALCHEMY_DATABASE_URL.startswith("sqlite"):
    engine = create_engine(SQLALCHEMY_DATABASE_URL, connect_args={"check_same_thread": False})
else:
    engine = create_engine(SQLALCHEMY_DATABASE_URL)

SessionLocal = sessionmaker(autocommit=False, autoflush=False, bind=engine)
Base = declarative_base()

def get_db():
    db = SessionLocal()
    try:
        yield db
    finally:
        db.close()
