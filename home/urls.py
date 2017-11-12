from django.conf.urls import url
from . import views
app_name='home'

urlpatterns = [
url(r'^$', views.Home.as_view(), name = 'home'),
url(r'^crops/', views.check_crops, name = 'crops'),
url(r'^resp/', views.get_details, name = 'resp'),
url(r'^details/', views.all_details, name = 'det'),
]
