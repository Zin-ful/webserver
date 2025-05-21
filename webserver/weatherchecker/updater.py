#!/usr/bin/env python3

import requests
link0 = "https://raw.githubusercontent.com/Zin-ful/zin-minimal-suite/refs/heads/main/Universal%20Tools/updater.py"
print("Tool Version: 1.5\nType 0.")
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
	print("updated!!")
while True:
	inp = input(">>> ")
	if "exit" in inp:
		exit()
	if "0" in inp or "upd" in inp or "Upd" in inp or "uPD" in inp:
		updateme(link0, "updater.py")
		break
	else:
		print("Sorry but that isnt an option")
		continue
