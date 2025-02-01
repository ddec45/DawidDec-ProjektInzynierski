# -*- coding:utf-8 -*-
import LCD_1in44
from PIL import Image,ImageDraw,ImageFont,ImageColor
import textwrap
from enum import Enum
import time
import requests
import json

class State(Enum):
    MAIN = 1
    DISPLAY_STATS = 2
    START_MINER = 3
    STOP_MINER = 4
    DISPLAY_STATS_EMPTY = 5
    NO_MINER_SERVER = 6

f = open('../aplikacja_serwera/user_api_key.txt', 'r', encoding="utf-8")
user_api_key = f.readline().strip()
f.close()
api_key_header = {'X-API-Key': user_api_key}
print(api_key_header)

def get_miner_stats():
    url = "https://127.0.0.1:8080/user/miner/instance/statistics/list"
    r = requests.get(url, verify=False, headers=api_key_header)
    if r.status_code != 200:
        return False
    return r.json()

def get_miner_instances(id=None):
    if id is None:
        url = "https://127.0.0.1:8080/user/miner/instance/list"
    else:
        url = f"https://127.0.0.1:8080/user/miner/instance/{id}"
    r = requests.get(url, verify=False, headers=api_key_header)
    if r.status_code != 200:
        print(r.status_code)
        return False
    return r.json()

def get_miner_apps():
    url = "https://localhost:8080/user/miner/application/list"
    r = requests.get(url, verify=False, headers=api_key_header)
    if r.status_code != 200:
        return False
    return r.json()

def post_new_miner_instance(id):
    url = f"https://localhost:8080/user/miner/instance/start/{id}"
    r = requests.post(url, json={'input_arguments':''}, verify=False, headers=api_key_header)
    if r.status_code != 201:
        return False
    return r.json()

def delete_miner_instance(id=None):
    url = f"https://localhost:8080/user/miner/instance/delete/{id}"
    r = requests.delete(url, verify=False, headers=api_key_header)
    if r.status_code != 200:
        return False
    return r.json()

class rising_edge:
  def __init__(self):
      self.temp = False   
  def __call__(self, var):
    up = bool(var) and (not self.temp)
    self.temp = bool(var)
    return up
    
up_rising_edge = rising_edge()
down_rising_edge = rising_edge()
left_rising_edge = rising_edge()
right_rising_edge = rising_edge()

key1_rising_edge = rising_edge()
key2_rising_edge = rising_edge()
key3_rising_edge = rising_edge()

# 240x240 display with hardware SPI:
disp = LCD_1in44.LCD()
Lcd_ScanDir = LCD_1in44.SCAN_DIR_DFT  #SCAN_DIR_DFT = D2U_L2R
disp.LCD_Init(Lcd_ScanDir)
disp.LCD_Clear()

# Create blank image for drawing.
# Make sure to create image with mode '1' for 1-bit color.
image = Image.new('RGB', (disp.width, disp.height))

# Get drawing object to draw on image.
draw = ImageDraw.Draw(image)
font = ImageFont.load_default()

# Draw a black filled box to clear the image.
draw.rectangle((0,0,disp.width,disp.height), outline=0, fill=0)
disp.LCD_ShowImage(image,0,0)

state = State.NO_MINER_SERVER
y_cnt = 0
x_cnt = 0
timestamp = 0.0
current_time = 0.0
timediff = 20.0
scrolling_timestamp = 0.0
scrolling_timediff = 0.05

miner_apps = None
miner_stats = None
miner_instances = None
current_miner_app = None
current_miner_stats = None
current_miner_id = None
last_miner_id = None
current_miner_name = ''

# try:
draw.rectangle((0,0,disp.width,disp.height), outline=0, fill=0)
while True:
    time.sleep(0.05)
    disp.LCD_ShowImage(image,0,0)
    (_, _, width, height) = font.getbbox("Sample text")
    draw.rectangle((0,0,disp.width,disp.height), outline=0, fill=0)
    try:
        if state == State.MAIN:
            if key1_rising_edge(disp.digital_read(disp.GPIO_KEY1_PIN)):
                if y_cnt == 0:
                    state = State.DISPLAY_STATS
                elif y_cnt == 1:
                    state = State.START_MINER
                else:
                    state = State.STOP_MINER
                y_cnt = 0
                timestamp = 0.0
                continue

            if up_rising_edge(disp.digital_read(disp.GPIO_KEY_UP_PIN)):
                if y_cnt > 0:
                    y_cnt -= 1
            if down_rising_edge(disp.digital_read(disp.GPIO_KEY_DOWN_PIN)):
                if y_cnt < 2:
                    y_cnt += 1

            fill1 = "WHITE"
            fill2 = "WHITE"
            fill3 = "WHITE"
            if y_cnt == 0:
                fill1 = "RED"
            elif y_cnt == 1:
                fill2 = "RED"
            else:
                fill3 = "RED"

            y = 0
            draw.line([(0,y),(disp.width,y)], fill="WHITE", width = 1)
            draw.text((0, y), "Cryptominer Server", fill = "WHITE", align="center")
            y += height
            draw.line([(0,y),(disp.width,y)], fill="WHITE", width = 1)
            draw.text((0, y), "Display statistics", fill = fill1, align="center")
            y += height
            draw.text((0, y), "Start instance", fill = fill2, align="center")
            y += height
            draw.text((0, y), "Stop instance", fill = fill3, align="center")


        elif state == State.DISPLAY_STATS:
            if key2_rising_edge(disp.digital_read(disp.GPIO_KEY2_PIN)):
                state = State.MAIN
                y_cnt = 0
                x_cnt = 0
                current_miner_id = None
                continue

            current_time = time.time()
            if key3_rising_edge(disp.digital_read(disp.GPIO_KEY3_PIN)) or current_time > timestamp + timediff:
                timestamp = current_time
                miner_stats = get_miner_stats()
                miner_instances = get_miner_instances()
            length = len(miner_stats)

            if disp.digital_read(disp.GPIO_KEY_UP_PIN) and current_time > scrolling_timestamp + scrolling_timediff: #up_rising_edge(disp.digital_read(disp.GPIO_KEY_UP_PIN)):
                if y_cnt > 0:
                    y_cnt -= 1
                    scrolling_timestamp = current_time
            if disp.digital_read(disp.GPIO_KEY_DOWN_PIN) and current_time > scrolling_timestamp + scrolling_timediff: #down_rising_edge(disp.digital_read(disp.GPIO_KEY_DOWN_PIN)):
                if y_cnt < 255:
                    y_cnt += 1
                    scrolling_timestamp = current_time
            if left_rising_edge(disp.digital_read(disp.GPIO_KEY_LEFT_PIN)):
                if x_cnt > 0:
                    x_cnt -= 1
            if right_rising_edge(disp.digital_read(disp.GPIO_KEY_RIGHT_PIN)):
                if x_cnt < length-1:
                    x_cnt += 1

            if length <= 0:
                t = "There are no"
                x = (disp.width/2) - (font.getbbox(t)[2]/2)
                y = (disp.height/2) - height*2
                draw.text((x, y), t, fill = "RED", align='center')
                t = "running instances."
                x = (disp.width/2) - (font.getbbox(t)[2]/2)
                y = (disp.height/2)
                draw.text((x, y), t, fill = "RED", align='center')
                continue

            if x_cnt >= length:
                x_cnt = length - 1

            current_miner_stats = miner_stats[x_cnt]
            current_miner_id = current_miner_stats['miner_instance_id']
            current_miner_name = ''
            for instance in miner_instances:
                id = instance['id']
                if current_miner_id == id:
                    current_miner_name = instance["name"]
                    break

            y = 0
            draw.line([(0,y),(disp.width,y)], fill="WHITE", width = 1)
            draw.text((0, y), current_miner_name, fill = "WHITE")
            t = f"id={current_miner_id} {x_cnt+1}/{length}"
            x = disp.width - font.getbbox(t)[2]
            draw.text((x, y), t, fill = "WHITE")
            y += height
            draw.line([(0,y),(disp.width,y)], fill="WHITE", width = 1)

            text = json.dumps(current_miner_stats['stats'])
            lines = textwrap.wrap(text, width=21)
            temp_cnt = y_cnt
            for line in lines:
                if temp_cnt > 0:
                    temp_cnt -= 1
                    continue
                draw.text((1, y), line, font=font, fill="WHITE")
                y += height


        elif state == State.START_MINER:
            if key2_rising_edge(disp.digital_read(disp.GPIO_KEY2_PIN)):
                state = State.MAIN
                y_cnt = 0
                x_cnt = 0
                current_miner_id = None
                continue

            if miner_apps is None:
                miner_apps = get_miner_apps()
            length = len(miner_apps)

            current_time = time.time()
            if disp.digital_read(disp.GPIO_KEY_UP_PIN) and current_time > scrolling_timestamp + scrolling_timediff: #up_rising_edge(disp.digital_read(disp.GPIO_KEY_UP_PIN)):
                if y_cnt > 0:
                    y_cnt -= 1
                    scrolling_timestamp = current_time
            if disp.digital_read(disp.GPIO_KEY_DOWN_PIN) and current_time > scrolling_timestamp + scrolling_timediff: #down_rising_edge(disp.digital_read(disp.GPIO_KEY_DOWN_PIN)):
                if y_cnt < 255:
                    y_cnt += 1
                    scrolling_timestamp = current_time
            if left_rising_edge(disp.digital_read(disp.GPIO_KEY_LEFT_PIN)):
                if x_cnt > 0:
                    x_cnt -= 1
            if right_rising_edge(disp.digital_read(disp.GPIO_KEY_RIGHT_PIN)):
                if x_cnt < length-1:
                    x_cnt += 1

            if key1_rising_edge(disp.digital_read(disp.GPIO_KEY1_PIN)):
                r = post_new_miner_instance(current_miner_id)

            if length <= 0:
                t = "There are no"
                x = (disp.width/2) - (font.getbbox(t)[2]/2)
                y = (disp.height/2) - height*2
                draw.text((x, y), t, fill = "RED", align='center')
                t = "available applications."
                x = (disp.width/2) - (font.getbbox(t)[2]/2)
                y = (disp.height/2)
                draw.text((x, y), t, fill = "RED", align='center')
                continue

            if x_cnt >= length:
                x_cnt = length - 1

            current_miner_app = miner_apps[x_cnt]
            current_miner_id = current_miner_app['id']
            current_miner_name = current_miner_app['name']

            y = 0
            draw.text((0, y), "START", fill = "RED")
            y += height
            draw.line([(0,y),(disp.width,y)], fill="WHITE", width = 1)
            draw.text((0, y), current_miner_name, fill = "WHITE")
            t = f"id={current_miner_id} {x_cnt+1}/{length}"
            x = disp.width - font.getbbox(t)[2]
            draw.text((x, y), t, fill = "WHITE")
            y += height
            draw.line([(0,y),(disp.width,y)], fill="WHITE", width = 1)

            text = current_miner_app['description']
            lines = textwrap.wrap(text, width=21)
            temp_cnt = y_cnt
            for line in lines:
                if temp_cnt > 0:
                    temp_cnt -= 1
                    continue
                draw.text((1, y), line, font=font, fill="WHITE")
                y += height


        elif state == State.STOP_MINER:
            if key2_rising_edge(disp.digital_read(disp.GPIO_KEY2_PIN)):
                state = State.MAIN
                y_cnt = 0
                x_cnt = 0
                current_miner_id = None
                continue

            current_time = time.time()
            if key3_rising_edge(disp.digital_read(disp.GPIO_KEY3_PIN)) or current_time > timestamp + timediff:
                timestamp = current_time
                miner_stats = get_miner_stats()
                miner_instances = get_miner_instances()
            length = len(miner_stats)

            if disp.digital_read(disp.GPIO_KEY_UP_PIN) and current_time > scrolling_timestamp + scrolling_timediff: #up_rising_edge(disp.digital_read(disp.GPIO_KEY_UP_PIN)):
                if y_cnt > 0:
                    y_cnt -= 1
                    scrolling_timestamp = current_time
            if disp.digital_read(disp.GPIO_KEY_DOWN_PIN) and current_time > scrolling_timestamp + scrolling_timediff: #down_rising_edge(disp.digital_read(disp.GPIO_KEY_DOWN_PIN)):
                if y_cnt < 255:
                    y_cnt += 1
                    scrolling_timestamp = current_time
            if left_rising_edge(disp.digital_read(disp.GPIO_KEY_LEFT_PIN)):
                if x_cnt > 0:
                    x_cnt -= 1
            if right_rising_edge(disp.digital_read(disp.GPIO_KEY_RIGHT_PIN)):
                if x_cnt < length-1:
                    x_cnt += 1

            if key1_rising_edge(disp.digital_read(disp.GPIO_KEY1_PIN)):
                r = delete_miner_instance(current_miner_id)

            if length <= 0:
                t = "There are no"
                x = (disp.width/2) - (font.getbbox(t)[2]/2)
                y = (disp.height/2) - height*2
                draw.text((x, y), t, fill = "RED", align='center')
                t = "running instances."
                x = (disp.width/2) - (font.getbbox(t)[2]/2)
                y = (disp.height/2)
                draw.text((x, y), t, fill = "RED", align='center')
                continue

            if x_cnt >= length:
                x_cnt = length - 1

            current_miner_stats = miner_stats[x_cnt]
            current_miner_id = current_miner_stats['miner_instance_id']
            current_miner_name = ''
            for instance in miner_instances:
                id = instance['id']
                if current_miner_id == id:
                    current_miner_name = instance["name"]
                    break

            y = 0
            draw.text((0, y), "STOP", fill = "RED")
            y += height
            draw.line([(0,y),(disp.width,y)], fill="WHITE", width = 1)
            draw.text((0, y), current_miner_name, fill = "WHITE")
            t = f"id={current_miner_id} {x_cnt+1}/{length}"
            x = disp.width - font.getbbox(t)[2]
            draw.text((x, y), t, fill = "WHITE")
            y += height
            draw.line([(0,y),(disp.width,y)], fill="WHITE", width = 1)

            text = json.dumps(current_miner_stats['stats'])
            lines = textwrap.wrap(text, width=21)
            temp_cnt = y_cnt
            for line in lines:
                if temp_cnt > 0:
                    temp_cnt -= 1
                    continue
                draw.text((1, y), line, font=font, fill="WHITE")
                y += height


        elif state == State.NO_MINER_SERVER:
            draw.rectangle((0,0,disp.width,disp.height), outline=0, fill=0)
            (_, _, width, height) = font.getbbox("Sample text")
            t = "Waiting for"
            x = (disp.width/2) - (font.getbbox(t)[2]/2)
            y = (disp.height/2) - height*2
            draw.text((x, y), t, fill = "WHITE", align='center')
            t = "cryptominer start..."
            x = (disp.width/2) - (font.getbbox(t)[2]/2)
            y = (disp.height/2)
            draw.text((x, y), t, fill = "WHITE", align='center')
            disp.LCD_ShowImage(image,0,0)
            while True:
                current_time = time.time()
                if current_time > timestamp + 2:
                    try:
                        timestamp = current_time
                        get_miner_instances()
                        state = State.MAIN
                        break
                    except:
                        continue
                time.sleep(1.0)


    except:
        state = State.NO_MINER_SERVER
        continue


        # if disp.digital_read(disp.GPIO_KEY_UP_PIN ) == 0: # button is released       
        #     draw.polygon([(20, 20), (30, 2), (40, 20)], outline=255, fill=0xff00)  #Up           
        # else: # button is pressed:
        #     draw.polygon([(20, 20), (30, 2), (40, 20)], outline=255, fill=0)  #Up filled
        #     print ("Up" ) 

        # if disp.digital_read(disp.GPIO_KEY_LEFT_PIN) == 0: # button is released
        #     draw.polygon([(0, 30), (18, 21), (18, 41)], outline=255, fill=0xff00)  #left      
        # else: # button is pressed:       
        #     draw.polygon([(0, 30), (18, 21), (18, 41)], outline=255, fill=0)  #left filled
        #     print ("left")

        # if disp.digital_read(disp.GPIO_KEY_RIGHT_PIN) == 0: # button is released
        #     draw.polygon([(60, 30), (42, 21), (42, 41)], outline=255, fill=0xff00) #right
        # else: # button is pressed:
        #     draw.polygon([(60, 30), (42, 21), (42, 41)], outline=255, fill=0) #right filled       
        #     print ("right")

        # if disp.digital_read(disp.GPIO_KEY_DOWN_PIN) == 0: # button is released
        #     draw.polygon([(30, 60), (40, 42), (20, 42)], outline=255, fill=0xff00) #down   
        # else: # button is pressed:
        #     draw.polygon([(30, 60), (40, 42), (20, 42)], outline=255, fill=0) #down filled
        #     print ("down")

        # if disp.digital_read(disp.GPIO_KEY_PRESS_PIN) == 0: # button is released
        #     draw.rectangle((20, 22,40,40), outline=255, fill=0xff00) #center        
        # else: # button is pressed:
        #     draw.rectangle((20, 22,40,40), outline=255, fill=0) #center filled
        #     print ("center")

        # if disp.digital_read(disp.GPIO_KEY1_PIN) == 0: # button is released
        #     draw.ellipse((70,0,90,20), outline=255, fill=0xff00) #A button      
        # else: # button is pressed:
        #     draw.ellipse((70,0,90,20), outline=255, fill=0) #A button filled

        #     draw.text((0, 60), "XMRig Miner id=1", fill = "WHITE")
        #     text = 'Total hashrate: [312.8, 324.1, 308.9]'
        #     lines = textwrap.wrap(text, width=20)
        #     y = 80
        #     for line in lines:
        #         width, height = font.getsize(line)
        #         draw.text((1, y), line, font=font, fill="WHITE")
        #         y += height

        #     print ("KEY1")

        # if disp.digital_read(disp.GPIO_KEY2_PIN) == 0: # button is released
        #     draw.ellipse((100,20,120,40), outline=255, fill=0xff00) #B button] 
        # else: # button is pressed:
        #     draw.ellipse((100,20,120,40), outline=255, fill=0) #B button filled
        #     print ("KEY2")

        # if disp.digital_read(disp.GPIO_KEY3_PIN) == 0: # button is released
        #     draw.ellipse((70,40,90,60), outline=255, fill=0xff00) #A button
        # else: # button is pressed:
        #     draw.ellipse((70,40,90,60), outline=255, fill=0) #A button filled
        #     print ("KEY3")

# except:
# 	print("except")
disp.module_exit()
