import requests

response = requests.post("http://192.168.0.71/load_pill")

print(response.text)