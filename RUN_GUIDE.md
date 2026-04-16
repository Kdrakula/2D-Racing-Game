# 🏁 2D Racing Game - Instrukcja Uruchomienia

Ten dokument opisuje krok po kroku, w jaki sposób z czystym kontem uruchomić projekt (Klienta C++ oraz całą infrastrukturę serwerową) u siebie na komputerze po jego ponownym włączeniu.

---

## Krok 1: Uruchomienie Serwera i Bazy Danych
Twoja gra korzysta z mikrousług zapisujących wyniki w bazie danych. To najpierw musi "wstać", żeby klient C++ miał z czym rozmawiać.

1. **Uruchom demona Dockera**
   - Otwórz aplikację **OrbStack** (lub Docker Desktop), która zarządza kontenerami na Twoim Macu.
   - Upewnij się, że kółko świeci się na zielono (Running).

2. **Wystartuj kontenery z konsoli**
   - Otwórz terminal (lub ten wbudowany w VSCode).
   - Przejdź do głównego folderu gry: `cd /Users/kuba/2D-Racing-Game`
   - Wpisz komendę, by odpalić serwery w tle: 
     ```bash
     docker-compose up -d
     ```
   *(Powyższa komenda automatycznie uruchomi i serwer API FastAPI i bazę bazę PostgreSQL obok siebie!)*

3. **Sprawdzenie statusu (Opcjonalnie)**
   - Jeśli chcesz zobaczyć, czy kontenery poprawnie wystartowały, wpisz: `docker ps`
   - Oraz by zobaczyć zapisane czasy z bazy i upewnić się, że działa: `curl http://localhost:4000/api/laptimes`

---

## Krok 2: Uruchomienie Klienta (Gry w C++)
Kiedy strona "Backendowa" Dockera już działa i nasłuchuje na porcie `4000`, czas uruchomić samą grę.

1. **Budowanie projektu cmake** (Wymagane tylko jeśli zmieniałeś kod w plikach [.cpp](file:///Users/kuba/2D-Racing-Game/src/lapTimer.cpp) / `.hpp`)
   - Będąc w tym samym folderze (`/Users/kuba/2D-Racing-Game`), wpisz:
     ```bash
     cmake -S . -B build
     cmake --build build --config Release
     ```

2. **Włączenie aplikacji**
   - Aby odpalić okienko gry, wystarczy uruchomić skompilowany plik z folderu `build`:
     ```bash
     ./build/MyRacingGame
     ```

## Zakończenie pracy
Jeżeli chcesz zwolnić zasoby na komputerze i wstrzymać serwery, użyj polecenia:
```bash
docker-compose down
```
*(Baza zapamięta swoje stany na dysku, czasy okrążeń **nie znikną**!).*


## BeagleBoard
```bash
cd leaderboard-server/
uvicorn main:app --host 0.0.0.0 --port 4000
```