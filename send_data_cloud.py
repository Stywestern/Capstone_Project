import requests

channel_id = "2887470"  # Replace with your ThingSpeak channel ID
write_api_key = "C72BBAEET0UFASC6"  # Replace with your ThingSpeak Write API Key

url = f"https://api.thingspeak.com/update?api_key={write_api_key}&field1=10&field2=20&field3=30"

try:
    response = requests.get(url)
    response.raise_for_status()
    print("Data sent to ThingSpeak successfully!")
    print(response.text)  # Print the response from ThingSpeak
except requests.exceptions.RequestException as e:
    print(f"Error sending data to ThingSpeak: {e}")