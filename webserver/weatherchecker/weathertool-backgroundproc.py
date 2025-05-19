#!/usr/bin/env python


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

wait_time = 120 #seconds
wait_time_err = 60
wait_time_noalert = 1800
url = "https://api.weather.gov/alerts/active?area="
name = ""
state = "KY"
county = "none"
silent = 1
local_list = ["MI","OH","TX","KY"]

fetch_time = 0
fetch_cmd = ["curl", "-s", url]

parameters = {"wait_time": wait_time, "wait_time_noalert": wait_time_noalert, "wait_time_err": wait_time_err, "silent": silent, "state": state, "county": county}

if "parameters.conf" not in os.listdir():
	with open("parameters.conf", "w") as file:
		for key, value in parameters.items():
			file.write(f"{key}={value}\n")
else:
	with open("parameters.conf","r") as file:
		paradata = file.readlines()
		for item in paradata:
			if item == "" or item == " " or item == "\n":
				continue
			key, value = item.split("=")
			parameters[key] = value.strip().strip('\n')


def custom_update(msg):
	with open("alerts.html", "w") as file:
		file.write(f"<!DOCTYPE html><html><body><p>{msg}</p><a href=past_alerts.html>Previous Alerts</a></body></html>")

def timer():
	fetch_time = 0
	while True:
		t.sleep(1)
		fetch_time += 1
		if done:
			break

def webpage_update():
	global done
	while True:
		with open("parameters.conf","r") as file:
			paradata = file.readlines()
			for item in paradata:
				if item == "" or item == " " or item == "\n":
					continue
				key, value = item.split("=")
				parameters[key] = value.strip().strip('\n')
		with open("alerts.html","w") as file:
			file.write("<!DOCTYPE html><html><body>")
		try:
			done = 0
			time_thread = threading.Thread(target=timer)
			time_thread.start()
			weatherdata = requests.get(url+parameters['state'])
			errors = weatherdata.raise_for_status()
			data = weatherdata.json()
			done = 1
			time_thread.join()
			alert_data = data.get("features", [])
			if not alert_data:
				no_alert = f"NO ALERTS FOR {parameters['state']}, YAYYYY :DDD (waiting {int(parameters['wait_time_noalert']) / 60} minutes before next request)"
				if not silent:
					print(no_alert)
				custom_update(no_alert)
				t.sleep(int(parameters['wait_time_noalert']))
			else:
				msg = f"!!{len(alert_data)} ALERTS FOR {parameters['state']} FOUND.."
				if not silent:
					print(msg)
				with open(f"alerts.html", "a") as file:
					file.write(f"<h1>{msg}</h1>")
					
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
				try:
					

					with open(f"{headline.replace(' ', '_')}.html", "a") as file:
						file.write(f"<h1>WEATHER ALERT: {event}</h1>")
						file.write(f"<h2>{headline}\r\nISSUED AT: {time}</h2>")
						file.write(f"<p><b>{details}</b></p>")
					with open(f"alerts.html", "a") as file:
						file.write(f"<li><a href={headline.replace(' ', '_')}.html>{headline}</a></li>")
				except:
					headline_cache = headline.replace(":", "").replace(" ", "_").replace("/", "").replace(";", "").replace("?", "")
					with open(f'{headline_cache}.html', "w") as file:
						file.write(f"<h1>WEATHER ALERT: {event}</h1>")
						file.write(f"<h2>{headline}\r\nISSUED AT: {time}</h2>")
						file.write(f"<p><b>{details}</b></p>")
					with open(f"alerts.html", "a") as file:
						file.write(f"<li><a href={headline_cache}.html>{headline}</a></li>")
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
				file.write(f"<li><a href=past_alerts.html>Look at previous alerts here</a></li></body></html>")
			if not silent:
				print(time)
				print(f"{int(parameters['wait_time']) / 60} minutes before next request")
			t.sleep(int(parameters['wait_time']))
		except requests.exceptions.RequestException as e:
			if "400" in str(e):
				msg = f"*ERROR GETTING ALERT DATA ({int(parameters['wait_time_err']) / 60} minute(s) before next attempt): \n\n{e}\n**THE ERROR YOUVE ENCOUNTERED IS BECAUSE THE STATE IS NOT SET.***\nEITHER SET IT IN THE CONFIG FILE OR USE THE 'set-default' COMMAND"
				if not silent:
					print(msg)
				custom_update(msg)
			elif "Temporary failure in name resolution" in str(e):
				msg = f"ERROR GETTING ALERT DATA ({int(parameters['wait_time_err']) / 60} minute(s) before next attempt): \n\n{e}\n\n***THE ERROR YOUVE ENCOUNTERED IS BECAUSE THE DEVICE CANNOT REACH THE SPECIFIED URL***\nEITHER THE URL IS INCORRECT, NO LONGER MAINTAINED (INACTIVE), OR THE DEVICE DOES NOT HAVE INTERNET ACCESS/ACCESS TO THIS SPECIFIC URL: \n{url+parameters['state']}"
				if not silent:
					print(msg)
				custom_update(msg)
			else:
				msg = f"ERROR GETTING ALERT DATA ({int(parameters['wait_time_err']) / 60} minute(s) before next attempt): \n\n{e}"
				if not silent:
					print(msg)
				custom_update(msg)
			t.sleep(int(parameters['wait_time_err']))
		except Exception as e:
			done = 1
			print(e)
			custom_update(f"UNEXPECTED ERROR ENCOUNTERED: {e}")
			time_thread.join()
webpage_update()
