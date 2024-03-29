import requests
import time

# response = requests.post("http://192.168.0.19/start_reloading")
# response = requests.post("http://192.168.0.19/will_load_pill", json={
#     "pill_key": "abcabcabc",
#     "pill_datetime": "2024-03-29T04:00:00.000Z"
# })
# print(response.text)
response = requests.post("http://192.168.0.19/load_pill")
