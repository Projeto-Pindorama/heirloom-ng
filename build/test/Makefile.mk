all: tests

tests: pass.txt
	@echo Olá, Mundo! > pass.txt
	@echo Olá, Mundo!
	
pass.txt:
	@touch pass.txt

clean:
	rm pass.txt
