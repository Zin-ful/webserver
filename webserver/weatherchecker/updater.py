#!/usr/bin/env python

import subprocess
link1 = "https://raw.githubusercontent.com/Zin-ful/webserver/refs/heads/main/webserver/weatherchecker/weathertool.py"
link2 = "https://raw.githubusercontent.com/Zin-ful/webserver/refs/heads/main/webserver/weatherchecker/weathertool-backgroundproc.py"
print("What would you like to update?\n\n1. Weather Tool\n2.Weather Tool Web")
while True:
	inp = input(">>> ")
	if "1" in inp or inp == "Weather Tool" or inp == "weather tool" or inp == "WeatherTool" or inp == "weathertool":
		updateme(link1)
	elif "2" in inp or "web" in inp or "Web" in inp or "WEB" in inp or "wEB" in inp:
		updateme(link2)
	else:
		print("Sorry but that isnt an option")
		continue

def updateme(link):
	subprocess.run(f"wget {link}", shell=True)