import requests

response = requests.post("http://192.168.0.71/start_reloading")

print(response.text)