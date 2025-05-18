import requests
import json
import threading
import os
from datetime import datetime
import sys
import time as t
if "alerts.html" not in os.listdir():
	with open("alerts.html","w") as file:
		file.write("")
if "past_alerts.html" not in os.listdir():
	with open("past_alerts.html","w") as file:
		file.write("")

url = "https://api.weather.gov/alerts/active?area="
name = ""
state = "KY"
local_list = ["MI","OH","TX","KY"]

fetch_cmd = ["curl", "-s", url]

fetch_time = 0
done = 0

def alert():
	return

def cast():
	return

def helpy():
	return

def config():
	return
	
def timer():
	global fetch_time
	fetch_time = 0
	while True:
		t.sleep(1)
		fetch_time += 1
		if done:
			break

def webpage_update():
	global done
	while True:
		with open("alerts.html","w") as file:
			file.write("<!DOCTYPE html><html><body>")
		try:
			done = 0
			time_thread = threading.Thread(target=timer)
			time_thread.start()
			weatherdata = requests.get(url+state)
			errors = weatherdata.raise_for_status()
			data = weatherdata.json()
			alert_data = data.get("features", [])
			if not alert_data:
				print(f"NO ALERTS FOR {state}, YAYYYY :DDD")
				t.sleep(1800)
			else:
				print(f"!!{len(alert_data)} ALERTS FOR {state} FOUND..")
			for alert in alert_data:
				properties = alert.get("properties", {})
				event = properties.get("event", "unknown event")
				headline = properties.get("headline", "no headline")
				details = properties.get("description", "no description")
				effective = properties.get("effective")
				if effective:
					time = datetime.fromisoformat(effective.rstrip('Z')).strftime('%Y-%m-%d %H:%M %Z')
				if not effective:
					time = "Unknown time"
				if "no properties" in properties:
					properties = "no properties"
				with open("alerts.html", "a") as file:
					file.write(f"<h1>WEATHER ALERT: {event}</h1>")
					file.write(f"<h2>{headline}\r\nISSUED AT: {time}</h2>")
					file.write(f"<p><b>{details}</b></p>")
				with open("past_alerts.html", "r") as file1:
					file_data = file1.read()
					if time not in file_data:
						with open("past_alerts.html", "a") as file2:
							past = file1.readlines()
							for i in past:
								if "DOCTYPE" not in check[0]:
									past.insert(f"<!DOCTYPE html><html><body>", 0)
							del past[-2:]
							past.append(f"<h1>WEATHER ALERT: {event}</h1>")
							past.append(f"<h2>{headline}\r\nISSUED AT: {time}</h2>")
							past.append(f"<p><b>{details}</b></p>")
							past.append(f"</body></html>")
							for item in past:
								file2.write(f"{item}\n")
			with open("alerts.html", "a") as file:
				file.write(f"<p>processing and fetch time:{fetch_time}</p>")
				file.write(f"</body></html>")
			done = 1
			time_thread.join()
			print(time)
			print("10 minutes before next request")
			t.sleep(600)
		except requests.exceptions.RequestException as e:
			print("ERROR GETTING ALERT DATA, ", e, "\n\n")
			if "400" in str(e):
				print("***THE ERROR YOUVE ENCOUNTERED IS BECAUSE THE STATE IS NOT SET.***\nEITHER SET IT IN THE CONFIG FILE OR USE THE 'set-default' COMMAND")

def shell():
	while True:
		print("YOU HAVE SPAWNED A SHELL TO ACCESS THE WEATHER COMMANDLINE")
		print("IF WEATHER EVENTS ARE WITHIN EYESIGHT PLEASE SEEK IMMEDIATE SHELTER...")
		print("FOR ALERTS: alert (STATE)")
		print("FOR BASIC WEATHER: cast (STATE)")
		print("\nEXAMPLE COMMAND: alert ky")

update_thread = threading.Thread(target=webpage_update)
update_thread.start()


cmd_list = {"alert": alert, "cast": cast, "help": helpy, "set-default":config}

