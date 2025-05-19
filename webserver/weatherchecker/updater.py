#!/usr/bin/env python

import requests
link1 = "https://raw.githubusercontent.com/Zin-ful/webserver/refs/heads/main/webserver/weatherchecker/weathertool.py"
link2 = "https://raw.githubusercontent.com/Zin-ful/webserver/refs/heads/main/webserver/weatherchecker/weathertool-backgroundproc.py"
print("What would you like to update?\n\n1. Weather Tool\n2. Weather Tool Web")
def updateme(link, name):
	response = requests.get(link)
	errors = response.raise_for_status()
	if response.status_code == 200:
		with open(name, "wb") as file:
			file.write(response.content)
	elif response.status_code == 400:
		print(f"\n****Err: {response.status_code}, code 400 points to the link being incorrect or it could be a server-side issue")
	elif response.status_code == 404:
		print(f"\n****Err: {response.status_code}, code 404 points to the link/file being unavalible or the site is down/no longer maintained")
	elif response.status_code == 408:
		print(f"\n****Err: {response.status_code}, code 408 points to your request not reaching the site before it timed out")
	else:
		print(f"\n****Err: {response.status_code}")
while True:
	inp = input(">>> ")
	if "1" in inp or inp == "Weather Tool" or inp == "weather tool" or inp == "WeatherTool" or inp == "weathertool":
		updateme(link1, "weathertool.py")
		break
	elif "2" in inp or "web" in inp or "Web" in inp or "WEB" in inp or "wEB" in inp:
		updateme(link2, "wt-background.py")
		break
	else:
		print("Sorry but that isnt an option")
		continue
