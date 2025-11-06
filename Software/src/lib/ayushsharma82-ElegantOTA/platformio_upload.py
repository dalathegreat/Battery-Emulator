# Allows PlatformIO to upload directly to ElegantOTA
#
# To use:
# - copy this script into the same folder as your platformio.ini
# - set the following for your project in platformio.ini:
#
# extra_scripts = platformio_upload.py
# upload_protocol = custom
# custom_upload_url = <your upload URL>
# 
# An example of an upload URL:
#                custom_upload_url = http://192.168.1.123/update 
# also possible: custom_upload_url = http://domainname/update

import sys
import requests
import hashlib
from urllib.parse import urlparse
import time
from requests.auth import HTTPDigestAuth
Import("env")

try:
    from requests_toolbelt import MultipartEncoder, MultipartEncoderMonitor
    from tqdm import tqdm
except ImportError:
    env.Execute("$PYTHONEXE -m pip install requests_toolbelt")
    env.Execute("$PYTHONEXE -m pip install tqdm")
    from requests_toolbelt import MultipartEncoder, MultipartEncoderMonitor
    from tqdm import tqdm

def on_upload(source, target, env):
    firmware_path = str(source[0])

    auth = None
    upload_url_compatibility = env.GetProjectOption('custom_upload_url')
    upload_url = upload_url_compatibility.replace("/update", "")

    with open(firmware_path, 'rb') as firmware:
        md5 = hashlib.md5(firmware.read()).hexdigest()

        parsed_url = urlparse(upload_url)
        host_ip = parsed_url.netloc

        # Führe die GET-Anfrage aus
        start_url = f"{upload_url}/ota/start?mode=fr&hash={md5}"

        start_headers = {
            'Host': host_ip,
            'User-Agent': 'Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:109.0) Gecko/20100101 Firefox/118.0',
            'Accept': '*/*',
            'Accept-Language': 'de,en-US;q=0.7,en;q=0.3',
            'Accept-Encoding': 'gzip, deflate',
            'Referer': f'{upload_url}/update',
            'Connection': 'keep-alive'
            }
        
        try:
            checkAuthResponse = requests.get(f"{upload_url_compatibility}/update")
        except Exception as e:
            return 'Error checking auth: ' + repr(e)
        
        if checkAuthResponse.status_code == 401:
            try:
                username = env.GetProjectOption('custom_username')
                password = env.GetProjectOption('custom_password')
            except:
                username = None
                password = None
                print("No authentication values specified.")
                print('Please, add some Options in your .ini file like: \n\ncustom_username=username\ncustom_password=password\n')
            if username is None or password is None:
                return "Authentication required, but no credentials provided."
            print("Serverconfiguration: authentication needed.")
            auth = HTTPDigestAuth(username, password)
            try:
                doUpdateAuth = requests.get(start_url, headers=start_headers, auth=auth)
            except Exception as e:
                return 'Error while authenticating: ' + repr(e)

            if doUpdateAuth.status_code != 200:
                return "Authentication failed " + str(doUpdateAuth.status_code)
            print("Authentication successful")
        else:
            auth = None
            print("Serverconfiguration: authentication not needed.")
            try:
                doUpdate = requests.get(start_url, headers=start_headers)
            except Exception as e:
                return 'Error while starting upload: ' + repr(e)

            if doUpdate.status_code != 200:
                return "Start request failed " + str(doUpdate.status_code)

        firmware.seek(0)
        encoder = MultipartEncoder(fields={
            'MD5': md5,
            'firmware': ('firmware', firmware, 'application/octet-stream')}
        )

        bar = tqdm(desc='Upload Progress',
                   total=encoder.len,
                   dynamic_ncols=True,
                   unit='B',
                   unit_scale=True,
                   unit_divisor=1024
                   )

        monitor = MultipartEncoderMonitor(encoder, lambda monitor: bar.update(monitor.bytes_read - bar.n))

        post_headers = {
            'Host': host_ip,
            'User-Agent': 'Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:109.0) Gecko/20100101 Firefox/118.0',
            'Accept': '*/*',
            'Accept-Language': 'de,en-US;q=0.7,en;q=0.3',
            'Accept-Encoding': 'gzip, deflate',
            'Referer': f'{upload_url}/update',
            'Connection': 'keep-alive',
            'Content-Type': monitor.content_type,
            'Content-Length': str(monitor.len),
            'Origin': f'{upload_url}'
        }

        try:
            response = requests.post(f"{upload_url}/ota/upload", data=monitor, headers=post_headers, auth=auth)
        except Exception as e:
            return 'Error while uploading: ' + repr(e)
        
        bar.close()
        time.sleep(0.1)
        
        if response.status_code != 200:
            message = "\nUpload failed.\nServer response: " + response.text
            tqdm.write(message)
        else:
            message = "\nUpload successful.\nServer response: " + response.text
            tqdm.write(message)

            
env.Replace(UPLOADCMD=on_upload)