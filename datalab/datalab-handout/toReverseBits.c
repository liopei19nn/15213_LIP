unsigned toReverseBits(unsigned input){
	unsigned comparator = 0x1;
	unsigned output = 0x0;
	int i;
	for (i = 0; i < 32; i++){
		output = output | ((input & comparator) << (31-i));
		comparator = comparator << 1;
	}
	return output;
}
int main(void){
	unsigned input = 8;
	printf("input %x\n",input);
	unsigned output = 0x1;
	printf("output %x\n",output);
	unsigned comp = 0x1;
	printf("comp %x\n",comp);
	comp = comp << 3;
	printf("comp %x\n",comp);
	unsigned store = comp & input;
	printf("store %x\n",store);
	printf("store %x\n",store << 26);	
	// 0000 0000 0000 0000 0000 0000 0000 0100
	// 0010 0000 0000 0000 0000 0000 0000 0000
		
	

	
	
	return 0;
}