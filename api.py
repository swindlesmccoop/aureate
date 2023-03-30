#!/usr/bin/python3

import requests
import sys

url = "https://aur.archlinux.org/rpc/?v=5&type=search&by=name&arg={package_name}"

if len(sys.argv) < 2:
	print("Please provide a package name as an argument.")
	sys.exit()
package_name = sys.argv[1]

response = requests.get(url.format(package_name=package_name))

if response.status_code == 200:
	data = response.json()
	if data["resultcount"] > 0:
		for package_info in data["results"]:
			print(f"Package name: {package_info['Name']}")
			print(f"Description: {package_info['Description']}")
			print(f"Maintainer: {package_info['Maintainer']}")
			print(f"URL: {package_info['URL']}")
			print("-----------------------")
	else:
		print(f"No results found for {package_name}.")
else:
	print(f"Error: {response.status_code} - {response.reason}")
