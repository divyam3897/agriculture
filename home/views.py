from django.shortcuts import render
from .forms import *
from django.views.generic import TemplateView
import random
import paho.mqtt.client as mqtt
from django.views.decorators.csrf import csrf_exempt, csrf_protect
import json


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
    print("g")

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
    print(data)
