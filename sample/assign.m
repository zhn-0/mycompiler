main()
{
	int i,j;
	i=8;
	j=0;
	i++;
	goto ab;
	i++;
	ab:
	print(i, " ", j, "\n");
	for(int k=0 ; k<6 ;++k){
		for(int l=0 ;l<10;++l){
			++j;
		}
	}
	print(i, " ",j, "\n");
}
