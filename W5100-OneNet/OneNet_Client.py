#! /usr/bin/env python


#        __master_key = 'dAjZ9gqANMuTdQI=Q27mCkTmCqs='
#        __device_apikey = 'c3KGtwqLBzQpze3hAOD9xxllSkU='
#        __device_id = '773039'


import httplib, urllib

class OneNetClient:
	
	def __init__(self, server, *device_ids, **api_key):
		self.server = server
		self.device_ids = device_ids
		self.api_key = api_key

		print 'Connecting to the OneNet...'
		self.link = httplib.HTTPConnection(self.server)
		self.link.request('GET', '/')
		res = self.link.getresponse()
		if res.status == 200:
			print res.reason, 'Connected'
			res.read()		#to read the whole res data, but no print it
		else:
			print 'Connection Failed, please try a later'
	def con_close(self):
		self.link.close()

	def gentl_print(self, data):
		print data   #for temporary
		print '\n'	

	def get_single_device_info(self, index):
		url = '/devices/'+self.device_ids[index]
		self.link.request('GET', url, headers=self.api_key)
		res = self.link.getresponse()
		print res.status, '\t', res.reason
		return res.read() 

	def send_cmd_to_device(self, index, cmd_body):
		url = '/cmds?device_id='+self.device_ids[index]
		self.link.request('POST', url, cmd_body, self.api_key)
		res = self.link.getresponse()
		print res.status, '\t', res.reason
		return res.read()
	
	def get_sendcmd_status(self, cmd_uuid):
		url = '/cmds/'+cmd_uuid
		self.link.request('GET', url, headers=self.api_key)
		res = self.link.getresponse()
		print res.status, '\t', res.reason
		return res.read()

	def get_resp_exec_cmd(self, body, cmd_uuid):
		url = '/cmds/'+cmd_uuid+'/resp'
		self.link.request('GET', url, body, headers=self.api_key)	
		res = self.link.getresponse()
		print res.status, '\t', res.reason
		return res.read()

	def get_single_device_datastreams(self, index):
		url = '/devices/'+self.device_ids[index]+'/datastreams'
		self.link.request('GET', url, headers = self.api_key)
		res = self.link.getresponse()
		print res.status, '\t', res.reason
		return res.read()
