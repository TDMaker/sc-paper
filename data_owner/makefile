utils = ../utils
publics = ../public_params
data_swap = ../swap_zone
storages = ../storage_provider/storage

all: setup sig_gen arbitrate
setup: 
	gcc setup.c $(utils)/randys.c -o setup.out -L. -lgmp -lpbc
sig_gen:
	gcc sig_gen.c $(utils)/randys.c $(utils)/sha256.c -o sig_gen.out -L. -lgmp -lpbc
arbitrate:
	gcc arbitrate.c $(utils)/randys.c $(utils)/sha256.c -o arbitrate.out -L. -lgmp -lpbc
clean_all: clean_setup clean_sig_gen clean_arbitrate
clean_setup:
	rm $(publics)/g
	rm $(publics)/h
	rm $(publics)/public.key
	rm ./secret.key
	rm ./setup.out
clean_sig_gen:
	rm $(data_swap)/*.info
	find $(publics) -type f -not -name 'a.param' -delete
	rm $(storages)/sigs/*
	rm $(storages)/blks/*
	rm ./sig_gen.out
clean_arbitrate:
	rm ./arbitrate.out
	rm -r ./data/*