from django.db import models
#rom django.contrib.gis.geos import Point
#from location_field.models.spatial import LocationField
from django.utils.translation import gettext as _
import datetime

class cropModel(models.Model):
    crop = models.CharField(max_length=200)
    devId = models.IntegerField()
    date = models.DateField(_("Date"), default=datetime.date.today)
    #city = models.CharField(max_length=255)
    #location = LocationField(based_fields=['city'], zoom=7, default=Point(1.0, 1.0))
    #objects = models.GeoManager()

