import requests
import time

esp32_ip = "192.168.1.18"

def send_command(command):
    url = f"http://{esp32_ip}/command?command={command}"
    try:
        response = requests.get(url)
        response.raise_for_status()
        print(response.text)
    except requests.exceptions.RequestException as e:
        print(f"Error sending command: {e}")

try:
    while True:
        try:
            print("Sending commands...")
            send_command("led_on")
            time.sleep(5)
            send_command("led_off")
            time.sleep(5)
        except requests.exceptions.RequestException as e:
            print(f"Network error: {e}")
            time.sleep(10) # wait 10 seconds, before retrying.
        except Exception as e:
            print(f"An unexpected error occurred: {e}")
            break # break the loop, as something unexpected happend.
except KeyboardInterrupt:
    print("\nExecution stopped.")