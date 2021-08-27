all: challenge proof_verify
challenge:
	gcc challenge.c ../utils/randys.c ../utils/sha256.c -o challenge.out -L. -lgmp -lpbc
proof_verify:
	gcc proof_verify.c ../utils/randys.c ../utils/sha256.c -o proof_verify.out -L. -lgmp -lpbc
clean_all: 
	rm ./challenge.out
	rm ./proof_verify.out
	rm -r ./data/*