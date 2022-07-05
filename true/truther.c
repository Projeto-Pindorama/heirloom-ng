/* truther.c - provide truth values 
 * This file contain code for both true(1) and false(1).
 * Copyright (C) 2022: Luiz Antônio (takusuman). */
#if defined(FALSE)
	#define n 255
#elif !defined(FALSE) || defined(TRUE)
	#define n 0
#endif

int main(void){
	return(n);
}
