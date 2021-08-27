all: tag_verify proof_gen
tag_verify:
	gcc tag_verify.c ../utils/sha256.c ../utils/randys.c -o tag_verify.out -L. -lgmp -lpbc
proof_gen:
	gcc proof_gen.c ../utils/sha256.c ../utils/randys.c -o proof_gen.out -L. -lgmp -lpbc
clean_all: 
	rm ./tag_verify.out
	rm ./proof_gen.out
	rm -r ./data/*