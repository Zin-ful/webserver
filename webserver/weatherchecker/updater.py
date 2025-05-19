#!/usr/bin/env python

import requests
link1 = "https://raw.githubusercontent.com/Zin-ful/webserver/refs/heads/main/webserver/weatherchecker/weathertool.py"
link2 = "https://raw.githubusercontent.com/Zin-ful/webserver/refs/heads/main/webserver/weatherchecker/weathertool-backgroundproc.py"
print("What would you like to update?\n\n1. Weather Tool\n2.Weather Tool Web")
def updateme(link, name):
	newfile = requests.get(link)
	errors = newfile.raise_for_status()
	with open(name, "w") as file:
		file.write(str(newfile))

while True:
	inp = input(">>> ")
	if "1" in inp or inp == "Weather Tool" or inp == "weather tool" or inp == "WeatherTool" or inp == "weathertool":
		updateme(link1, "weathertool.py")
	elif "2" in inp or "web" in inp or "Web" in inp or "WEB" in inp or "wEB" in inp:
		updateme(link2, "wt-background.py")
	else:
		print("Sorry but that isnt an option")
		continue
