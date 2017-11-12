from django import forms
from .models import *

class cropsForm(forms.ModelForm):

    class Meta:
        model = cropModel
        fields = ('crop', 'devId','date')

