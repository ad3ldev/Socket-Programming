import requests
import base64

with open("image2.jpg", "rb") as image2string:
    converted_string = base64.b64encode(image2string.read())
res = requests.post('http://localhost:8989/trial.jpg',data = converted_string)
print(res.text)

# req = requests.get('http://localhost:8989/2.txt')
# e = req.encoding
# print("Encoding: ",e)
# s = req.status_code
# print("Response code: ",s)
# t = req.elapsed
# print("Response Time: ",t)
# t = req.headers['Content-Type']
# print("Header: ",t)
# z = req.text
# print("\nSome text from the web page:\n",z[0:200])