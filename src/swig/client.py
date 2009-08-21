from django.utils import simplejson
import urllib2

def Q(server, str):
	request = urllib2.Request(server, 'q=' + str)
	try:
		raw = urllib2.urlopen(request).read()
	except urllib2.HTTPError:
		return None
	result = simplejson.loads(raw)
	return result
