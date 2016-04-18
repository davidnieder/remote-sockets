# -*- coding: utf-8 -*-

import os
import hmac
import socket

from hashlib import sha256
from Crypto.Cipher import AES
from binascii import hexlify, unhexlify


AES_BLOCK_SIZE = 16
AES_IV_SIZE = 16

class Remote(object):

    def __init__(self, address, client_secret, server_secret):
        self.host = address.split(':')[0]
        self.port = int(address.split(':')[1])

        self.client_aes_key = hmac.new('cak', client_secret, sha256).digest()
        self.client_hmac_key = hmac.new('chk', client_secret, sha256).digest()
        self.server_aes_key = hmac.new('sak', server_secret, sha256).digest()
        self.server_hmac_key = hmac.new('shk', server_secret, sha256).digest()

    @staticmethod
    def from_user(user):
        return Remote(user.remote_addr, user.client_secret, user.server_secret)

    def send_command(self, sockets=[], mode='off'):
        if not sockets:
            return (False, 'sockets is empty')
        if mode not in ['on', 'off']:
            return (False, 'invalid value for mode')

        sockets = [str(s) for s in sockets]
        command = 'set' + ':' + ','.join(sockets) + ':' + mode

        if len(command) >= 16:
            return (False, 'command too long')

        success, response = self.send(command)

        if success and response == 'ack:set':
            return (True, response)
        return (False, response)

    def send_ping(self):
        command = 'ping'
        success, response = self.send(command)

        if success and response == 'ack:ping':
            return (True, response)
        return (False, response)

    def send(self, message):
        packet = self.make_packet(message)

        try:
            s = socket.create_connection((self.host, self.port), timeout=10)

            ret = s.send(packet)
            if (ret != len(packet)):
                s.close()
                return (False, 'send()')

            ret = s.recv(130)
            if (len(ret) != 130):
                s.close()
                return (False, 'recv()')

            s.close()
        except BaseException as e:
            return (False, str(e))

        aes_block = unhexlify(ret[:64])
        hmac_block = unhexlify(ret[64:128])

        if self.verify_server_hmac(aes_block, hmac_block) is False:
            return (False, 'hmac error')

        cleartext = self.decrypt_server_message(aes_block)
        cleartext = pkcs5_unpad(cleartext)
        return (True, cleartext)

    def make_packet(self, message):
        message = pkcs5_padding(message)

        message = self.encrypt_client_message(message)
        mac = self.hmac_client_message(message)
        newline = '\x0a'

        packet = hexlify(message + mac + newline)
        return packet

    def encrypt_client_message(self, message):
        iv = os.urandom(AES_IV_SIZE)
        aes = AES.new(self.client_aes_key, AES.MODE_CBC, iv)
        ciphertext = aes.encrypt(message)

        return iv+ciphertext

    def hmac_client_message(self, message):
        return hmac.new(self.client_hmac_key, message, sha256).digest()

    def verify_server_hmac(self, msg, msg_hmac):
        if getattr(hmac, 'compare_digest', None):
            return hmac.compare_digest(hmac.new(self.server_hmac_key, msg,
                            sha256).digest(), msg_hmac)
        # python < 2.7.7
        return hmac.new(self.server_hmac_key, msg, sha256).digest() == msg_hmac

    def decrypt_server_message(self, aes_block):
        iv = aes_block[:AES_IV_SIZE]
        cipher_text = aes_block[AES_IV_SIZE:]

        assert(len(iv) == AES_IV_SIZE)
        assert(len(cipher_text) == AES_BLOCK_SIZE)

        clear_text = AES.new(self.server_aes_key, AES.MODE_CBC, iv) \
                        .decrypt(cipher_text)
        return clear_text

def pkcs5_padding(message):
    padding_value = (AES_BLOCK_SIZE - len(message)) % AES_BLOCK_SIZE
    padding = hex(padding_value)[2:] * padding_value
    return message + padding

def pkcs5_unpad(message):
    padding_value = int(message[-1], 16)
    return message[:-padding_value]

