
######################################################################
# Available applications:
######################################################################
# get
# post
# persistent
# websockets
# basicAuth
# shoutcast
######################################################################

MANGO_APP = get



MANGO_SRC = \
	mango/mango.c \
	mango/mangoPort.c \
	mango/mangoHelper.c \
	mango/mangoDP.c \
	mango/mangoSM.c \
	mango/mangoWS.c \
	mango/crypto/mangoCrypto_base64.c


all:
	gcc -Wall -I mango apps/$(MANGO_APP)/main.c $(MANGO_SRC)
