import os
import json
from datetime import datetime
from flask import Flask, request, jsonify

app = Flask(__name__)

# Define the file where we will store the results
RESULTS_FILE = 'lap_times.json'


@app.route('/api/laptime', methods=['POST'])
def receive_laptime():
    # Ensure the request contains JSON data
    if not request.is_json:
        return jsonify({"error": "Request must be JSON"}), 400

    data = request.get_json()
    player = data.get('player', 'Unknown')
    time = data.get('time', 0.0)
    current_date = datetime.now().strftime("%Y-%m-%d %H:%M:%S")

    # Log to the console
    print(f"\n[SERVER] New Lap Record! Player: {player} | Time: {time}s | Date: {current_date}")

    # --- NEW: Permanent Storage Logic ---
    records = []

    # Check if the file already exists and load previous records
    if os.path.exists(RESULTS_FILE):
        try:
            with open(RESULTS_FILE, 'r') as f:
                records = json.load(f)
        except json.JSONDecodeError:
            print("[SERVER] Warning: Could not read existing file. Starting fresh.")

    # Append the new record to the list
    records.append({
        "player": player,
        "time": time,
        "date": current_date
    })

    # Save the updated list back to the JSON file
    with open(RESULTS_FILE, 'w') as f:
        json.dump(records, f, indent=4)

    print(f"[SERVER] Record saved to {RESULTS_FILE}")

    # Send success response back to C++
    return jsonify({
        "status": "success",
        "message": "Lap time saved successfully!"
    }), 200


@app.route('/api/laptimes', methods=['GET'])
def get_laptimes():
    records = []
    if os.path.exists(RESULTS_FILE):
        try:
            with open(RESULTS_FILE, 'r') as f:
                records = json.load(f)
        except json.JSONDecodeError:
            pass
    
    # Sort records by time (ascending) and take the top 10
    sorted_records = sorted(records, key=lambda x: x.get('time', float('inf')))[:10]
    
    return jsonify(sorted_records), 200

if __name__ == '__main__':
    print("[SERVER] Starting backend on port 4000...")
    app.run(host='0.0.0.0', port=4000, debug=True)