import requests
import time
from datetime import datetime
import csv

def get_thingspeak_data(channel_id, read_api_key, results=10):  # Retrieve multiple results
    """Retrieves data from ThingSpeak."""
    try:
        url = f"https://api.thingspeak.com/channels/{channel_id}/feeds.json?api_key={read_api_key}&results={results}"  # Get multiple results
        response = requests.get(url, timeout=5)
        response.raise_for_status()
        return response.json()  # Parse the JSON response
    except requests.exceptions.RequestException as e:
        print(f"Error retrieving data from ThingSpeak: {e}")
        return None

def write_to_csv(data, filename="thingspeak_data.csv"):
    """Writes ThingSpeak data to a CSV file."""
    try:
        with open(filename, "a", newline="") as csvfile:
            writer = csv.writer(csvfile)
            if csvfile.tell() == 0:  # Write header only if file is empty
                writer.writerow(["Time", "Humidity", "Temperature", "Soil_Moisture"])

            if data and "feeds" in data:
                for entry in data["feeds"]: # Iterate through each feed.
                    time_str = entry["created_at"]
                    humidity = entry["field1"]
                    temperature = entry["field2"]
                    soil_moisture = entry["field3"]
                    ip_adress = entry["field4"]
                    writer.writerow([time_str, humidity, temperature, soil_moisture, ip_adress])

    except IOError as e:
        print(f"Error writing to CSV file: {e}")

if __name__ == "__main__":
    channel_id = "2887470"  # Replace with your ThingSpeak channel ID
    read_api_key = "7W07WL45AH35JO6K"  # Replace with your ThingSpeak read API key
    interval = 10  # Seconds between data retrievals
    results = 10 # get 10 results at a time

    print(f"Retrieving data from ThingSpeak channel {channel_id} every {interval} seconds...")
    print("Press Ctrl+C to stop.")

    try:
        while True:
            data = get_thingspeak_data(channel_id, read_api_key, results) # get multiple results
            if data:
                write_to_csv(data)
                print(f"{datetime.now().strftime('%H:%M:%S')} - Data retrieved and saved to CSV.")
            time.sleep(interval)
    except KeyboardInterrupt:
        print("\nData retrieval stopped.")