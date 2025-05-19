#!/usr/bin/env python3

import requests
import json
import threading
import os
from datetime import datetime
import sys
import time as t

stuff = "/etc/emergency"

wait_time = 120 #seconds
wait_time_err = 60
wait_time_noalert = 1800
silent = 1
url = "https://api.weather.gov/alerts/active?area="
name = ""
state = "KY"
county = "none"

fetch_cmd = ["curl", "-s", url]

parameters = {"wait_time": wait_time, "wait_time_noalert": wait_time_noalert, "wait_time_err": wait_time_err, "silent": silent, "state": state, "county": county}

if "parameters.conf" not in os.listdir(stuff):
	with open(f"{stuff}/parameters.conf", "w") as file:
		for key, value in parameters.items():
			file.write(f"{key}={value}\n")
else:
	with open(f"{stuff}/parameters.conf","r") as file:
		paradata = file.readlines()
		for item in paradata:
			if item == "" or item == " " or item == "\n":
				continue
			key, value = item.split("=")
			parameters[key] = value.strip().strip('\n')

fetch_time = 0
done = 0

def cast():
	return

def cleanfiles(param):
	to_rm = []
	count = 0
	for item in os.listdir(stuff):
		if "issued" in item:
			to_rm.append(item)
			count += 1
	print(f"this will remove all cached files ({count} files)")
	inp = input("type yes to continue\n(clean up)>>> ")
	if "y" in inp and item:
		for item in to_rm:
			print(f"removing {item}")
			os.remove(f"{stuff}/{item}")
	print("back to shell...")

def helpy(param):
	print("\nalert (state): gets alerts for the specified state, if no state is specified, then the default is used.")
	print("clean: removed all HTML files other than alerts.html and past_alerts.html")
	print("set-default: opens the configuration tool")
	print("debug: prints the values of in-use parameters")
	print("help: what do you think?")
	cmd_list = {"alert": get_upd, "cast": cast, "help": helpy, "set-default":config, "debug": debug, "clean": cleanfiles}

def paraupd():
	with open(f"{stuff}/parameters.conf", "w") as file:
		for key, value in parameters.items():
			file.write(f"{key}={value}\n")
def debug(param):
	for key, value in parameters.items():
		print(f"{key}={value}")
def config(param):
	global parameters
	print("\nYou have opened the config tool. Please select one of these options to configure:\n")
	while True:
		print(f"\nwait_time (webupdater) - the amount of time the auto-updater waits before making a new request Value: {parameters['wait_time']}")
		print(f"wait_time_noalert (webupdater) - the amount of time after no alerts were found that the updater waits before making a new request Value: {parameters['wait_time_noalert']}")
		print(f"wait_time_err (webupdater) - the amount of time the updater waits after getting an error before making a new request. Value: {parameters['wait_time_err']}")
		print(f"url (not able to be changed)- the URL that the program will request data from (json formatted). Value: {url}")
		print(f"state - the state in which you want alerts from. Value: {parameters['state']}")
		print(f"county - setting the county will force open a browser if theres an alert found for your county. Value: {parameters['county']}\n**If no browser exists, it will take over the command-line and will force you to confirm that youve seen it.")
		inp = input("\n(config)>>> ")
		if "ex" in inp:
			return
		for key, value in parameters.items():
			if inp.lower() == key:
				inp2 = input(f"Selected: {key}\nNew value: ")
				if key == "county" or key == "state":
					if key == "state":
						inp2 = inp2.upper()
				else:
					try:
						inp3 = int(inp2)
					except:
						print("Numbers only please.")
						continue

				parameters[key] = str(inp2)
				print(f"\nupdated {inp}. New value: {parameters[key]}")
				paraupd()
				
				
def timer():
	global fetch_time, done
	fetch_time = 0
	while True:
		t.sleep(1)
		fetch_time += 1
		if done:
			break

def get_upd(param):
	global done
	if param:
		state_cache = parameters['state']
		parameters['state'] = param.upper()
	alert_list = []
	with open("alerts.html","w") as file:
		file.write("<!DOCTYPE html><html><body>")
	try:
		count = 1
		done = 0
		time_thread = threading.Thread(target=timer)
		time_thread.start()
		weatherdata = requests.get(url+parameters['state'].upper())
		errors = weatherdata.raise_for_status()
		data = weatherdata.json()
		alert_data = data.get("features", [])
		done = 1
		time_thread.join()
		if not alert_data:
			no_alert = f"NO ALERTS FOR {parameters['state']}, YAYYYY :DDD"
			print(no_alert)
			custom_update(no_alert)
		else:
			print(f"!!{len(alert_data)} ALERTS FOR {parameters['state']} FOUND..")
		for alert in alert_data:
			properties = alert.get("properties", {})
			event = properties.get("event", "unknown event")
			headline = properties.get("headline", "no headline")
			details = properties.get("description", "no description")
			effective = properties.get("effective")
			if parameters["county"] in headline or parameters["county"].upper() in headline or parameters["county"].lower() in headline:
				emergency(event, headline, details, effective)
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
			print(f"\n\n********ALERT {count}************** \nTIME: {time}\nEVENT: {event}\n{headline}\nDESCRIPTION: {details}\n")
			count += 1
			print("PRESS *ENTER* TO ACKNOWLEDGE (or type x to exit early)")
			ack = input()
			if "x" in ack.lower():
				break
			alert_list.append(f"{time}: {headline}")
		if param:
			parameters['state'] = state_cache
		print("here is a shortened version if your terminal cant scroll:")
		for i in alert_list:
			print(i)
		print(f"fetched in: {fetch_time}\n")
		
	except requests.exceptions.RequestException as e:
		if "400" in str(e):
			msg = f"*ERROR GETTING ALERT DATA: \n\n{e}\n**THE ERROR YOUVE ENCOUNTERED IS BECAUSE THE STATE IS NOT SET OR IT IS FORMATTED WRONG.***\nEITHER SET IT IN THE CONFIG FILE OR USE THE 'set-default' COMMAND"
			print(msg)
		elif "Temporary failure in name resolution" in str(e):
			msg = f"ERROR GETTING ALERT DATA: \n\n{e}\n\n***THE ERROR YOUVE ENCOUNTERED IS BECAUSE THE DEVICE CANNOT REACH THE SPECIFIED URL***\nEITHER THE URL IS INCORRECT, NO LONGER MAINTAINED (INACTIVE), OR THE DEVICE DOES NOT HAVE INTERNET ACCESS/ACCESS TO THIS SPECIFIC URL: \n{url+parameters['state']}"
			print(msg)
		else:
			msg = f"ERROR GETTING ALERT DATA: \n\n{e}"
			print(msg)
		if param:
			parameters['state'] = state_cache
		done = 1

def emergency(event, headline, details, effective):
	while True:
		print(f"********{event} ALERT FOR {parameters["county"]}**************")
		print(f"********{event} ALERT FOR {parameters["county"]}**************")
		print(f"\n{headline}\n{details}\n")
		print(f"********{event} ALERT FOR {parameters["county"]}**************")
		print(f"********{event} ALERT FOR {parameters["county"]}**************")
		inp = input("\nPLEASE TYPE 'yes' TO ACKNOWLEDGE\n>>> ")
		if "y" in inp:
			break


			
def custom_update(msg):
	with open(f"{stuff}/alerts.html", "w") as file:
		file.write(f"<!DOCTYPE html><html><body><p>{msg}</p><a href=past_alerts.html>Previous Alerts</a></body></html>")

cmd_list = {"alert": get_upd, "cast": cast, "help": helpy, "set-default":config, "debug": debug, "clean": cleanfiles}

def shell():
	global silent, done
	print("YOU HAVE SPAWNED A SHELL TO ACCESS THE WEATHER COMMANDLINE")
	print("IF WEATHER EVENTS ARE WITHIN EYESIGHT PLEASE SEEK IMMEDIATE SHELTER...")
	while True:
		print("\nFOR ALERTS: alert (STATE)")
		print("FOR BASIC WEATHER: cast (STATE)")
		print("\nEXAMPLE COMMAND: alert ky")
		inp = input(">>> ")
		if "ex" in inp:
			print("exiting shell.. update process still running. Setting turning off silence")
			break
		if " " in inp:
			inp, option = inp.split(" ")
		else:
			option = None
		xcute = cmd_list.get(inp.lower())
		if xcute:
			xcute(option)
			done = 1

shell()
done = 1
