from flask import Flask, request, jsonify

app = Flask(__name__)

@app.route('/api/laptime', methods=['POST'])
def receive_laptime():
    # Ensure the request contains JSON data
    if not request.is_json:
        return jsonify({"error": "Request must be JSON"}), 400

    data = request.get_json()
    player = data.get('player', 'Unknown')
    time = data.get('time', 0.0)

    # Log the received data to the console
    print("\n" + "="*40)
    print(f"[SERVER] New Lap Record Received!")
    print(f"[SERVER] Player: {player}")
    print(f"[SERVER] Time:   {time}s")
    print("="*40 + "\n")

    # Send a success response back to the C++ game
    return jsonify({
        "status": "success",
        "message": "Lap time saved successfully!"
    }), 200

if __name__ == '__main__':
    # Run the server on port 5000 (matching the C++ httplib configuration)
    print("[SERVER] Starting backend on port 4000...")
    app.run(host='0.0.0.0', port=4000, debug=True)