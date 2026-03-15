# Używamy lekkiego obrazu bazowego z Pythonem 3.11
FROM python:3.11-slim

# Tworzymy i ustawiamy katalog roboczy wewnątrz kontenera
WORKDIR /app

# Kopiujemy pliki zależności najpierw (optymalizacja cache Dockera)
COPY requirements.txt .

# Instalujemy zależności
RUN pip install --no-cache-dir -r requirements.txt

# Kopiujemy właściwy kod serwera Pythona
COPY server.py .

# Opcjonalnie: upewnienie się, że print() z Pythona natychmiast pojawia się w logach Dockera
ENV PYTHONUNBUFFERED=1

# Otwarcie portu 4000 dla świata zewnętrznego (poza kontener)
EXPOSE 4000

# Komenda uruchamiająca serwer
CMD ["python", "server.py"]
