#!/usr/bin/env python3

import os
import cgi

def main():
	error_page_path = "www/error_pages/418.html"
	error_page_url = "/error_pages/418.html"
	form = cgi.FieldStorage()

	try:
		with open(error_page_path, "r") as file:
			error_content = file.read()
		
		print("HTTP/1.1 302 Found")
		print(f"Location: {error_page_url}")
		print("Content-Type: text/html")
		print()
		print(f'<html><body><h1>302 Found</h1><p>Redirecting to <a href="{error_page_url}">{error_page_url}</a></p></body></html>')

	
	except FileNotFoundError:
		print("HTTP/1.1 418 I'm a teapot")
		print("Content-Type: text/html")
		print("Content-Length: 0")
		print()

if __name__ == "__main__":
	main()