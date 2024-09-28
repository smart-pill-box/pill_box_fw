import requests
import time
import datetime

choice = input(''' O que deseja fazer?
1 - Iniciar recarga
2 - Preparar para recarregar remedio
3 - Carregar remedio
4 - Tomar remédio
''')

now = datetime.datetime.utcnow()

if int(choice) == 1:
    response = requests.post("http://192.168.0.118/start_reloading")
elif int(choice) == 2:
    seconds_delta = int(input("Daqui a quantos segundos ?"))
    pill_time = now + datetime.timedelta(seconds=seconds_delta)
    pill_time_str = datetime.datetime.strftime(pill_time, "%Y-%m-%dT%H:%M:%S.%f")[:-3] + "Z"
    response = requests.post("http://192.168.0.118/will_load_pill", json={
        "pill_key": "abcabcabc",
        "pill_datetime": pill_time_str
    })
elif int(choice) == 3:
    response = requests.post("http://192.168.0.118/load_pill")
elif int(choice) == 4:
    response = requests.post("http://192.168.0.118/take_pill")
else:
    raise Exception("Bad input")

print(response.text)

