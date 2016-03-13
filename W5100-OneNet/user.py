#! /usr/bin/env python
# -*- coding:utf-8 -*-

'''
This python stript is for user who is using OneNet
If you have any problem in using, just input 'help'

date: /2016/3/8
by  : wi
'''

from OneNet_Client import OneNetClient
import term 
import ast

class User:
	
	def __init__(self):
		self.__help()
	
	def __help(self):
		help_doc = '''So far, we just need to control the led on or off. Here are four operations:on, off, quit, and check.
on(*arg): you can turn on one or more led lights simultaneously. eg, on id1 id2 id3 ...
off(*arg): this method for turning off one or more lights. eg, off id1 id2 id3 ...
check(): for getting the status of all the lights in room. eg, check
quit(): to quit the interface and end the application, eg, quit
'''	
		term.clear()
		term.writeLine(help_doc, term.cyan, term.blink)

	def on_or_off(self, on, *datastream_id):
		cmd_body = {}
		data_value = []
		for item in datastream_id:
			sub_body = {}
			sub_body['id'] = item
			sub_body['current_value'] = on
			data_value.append(sub_body)
		cmd_body['data'] = data_value
		print cmd_body
		return str(cmd_body)

	def check(self, value):
		root = ast.literal_eval(value)
		term.writeLine('led_id\tstatus', term.yellow)
		for item in root["data"]:
			try:
				print item['id'], "\t", item['current_value']
			except:
				print "analysis error"
	
	def quit(self):
		return True
		

def main():
	
	device_ids = ('773497',)  #if you have more device ids, you can add here as a tuple     
	headers = {'api-key':'1qHWLbTgkOUbQdpgE2zm26V0nc4='}
	client = OneNetClient('api.heclouds.com', *device_ids, **headers)
	
	options = ('off', 'on', 'check', 'quit')
	user = User()
	while(True):
		user_input = raw_input('>>>>').split()
		if user_input[0].lower() not in options:
			term.writeLine('Damn it! Wrong command. "quit" to exit', term.red, term.bold)
			continue

		elif len(user_input) > 1:
			arg = tuple(user_input[1:])
			body = user.on_or_off(options.index(user_input[0].lower()), *arg)
			client.send_cmd_to_device(0, body)			
		
		elif user_input[0].lower() == options[2]:
			re_value = client.get_single_device_datastreams(0)   # return response message body(str)
			user.check(re_value)

		else:	
			if user.quit():
				client.con_close()
			break
	
	term.writeLine('Have A Good Living, Dear User', term.red, term.reverse)

if __name__ == '__main__':
	main()
