main()
{
	int i,j,k,l;
	i=8;
	j=0;
	k=0;
	l=0;
	i++;
	goto ab;
	i++;
	ab:
	print(j);
	for( ; k<6 ;++k){
		for( ;l<10;++l){
			++j;
		}
	}
	print(j, " ");
}
