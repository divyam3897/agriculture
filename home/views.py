from django.shortcuts import render,redirect
from .forms import *
from django.views.generic import TemplateView
import random
import paho.mqtt.client as mqtt
from django.views.decorators.csrf import csrf_exempt, csrf_protect
import json
from django.http import JsonResponse
from watson_developer_cloud import VisualRecognitionV3

class Home(TemplateView):
    template_name = 'home.html'

def on_connect(client, userdata, flags, rc):
    print(str(rc))
    mqttc.subscribe("/temp",qos=0)
    print("sub")

def on_message(client, obj, msg):
    print(msg.topic + " " + str(msg.qos) + " " + str(msg.payload))


def check_crops(request):
    mqttc=mqtt.Client()
    mqttc.on_message=on_message
    #mqttc.max_queued_messages_set(self, 10)
    mqttc.username_pw_set("dfxukbgb", "lq_IHyYetOBV")
    mqttc.connect("m20.cloudmqtt.com", 14917)
    mqttc.on_connect=on_connect

    form = cropsForm()
    if request.method == 'POST': # If the form has been submitted...
        form = cropsForm(request.POST) # A form bound to the POST data
        if form.is_valid(): # All validation rules pass

            result = request.POST
            mqttc.publish("crop",result["crop"])
            mqttc.publish("devId",result["devId"])
            mqttc.publish("date",result["date"])
    return render(request, 'symptoms.html', {'form': form,'type':"Heart"})

@csrf_exempt
def get_details(request):
    data = json.loads(request.body)
    print("hekklo",data)
    return JsonResponse({'status':'200'})

def all_details(request):
    return render(request,'success.html',{'ideal': '0.0956654457', 'temp': '0', 'curr_date_water': '162.0', 'humidity': '0', 'actual': '0', 'curr_date': '3', 'total_water_vol': '486.0'})


def pests_predictor(request):
    return render(request,'pests.html')

def image_submit(request):
    visual_recognition = VisualRecognitionV3('2016-05-20', api_key='8c3cf9aeb2d85c594739a9a90c0ff684a5e6aaa7')
    print(json.dumps(visual_recognition.classify(images_url="https://transfer.sh/b2BIm/garden.jpg"), indent=2))
    return render(request,'home.html')
