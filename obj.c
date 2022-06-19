#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "tac.h"
#include "obj.h"

int get_reg(SYM *c);
int get_empty_r();

void init_lru()
{
	lru_head = (pLRU)malloc(sizeof(struct LRU));
	pLRU p = lru_head;
	int or ;
	for (or = R_GEN; or < R_NUM; or ++)
	{
		p->next = (pLRU)malloc(sizeof(struct LRU));
		p = p->next;
		p->id = or ;
	}
	lru_tail = p;
}

void free_lru()
{
	pLRU p = lru_head, p2;
	while (p)
	{
		printf("p:%p\n", p);
		p2 = p->next;
		free(p);
		p = p2;
	}
}

void update_lru()
{
	pLRU p = lru_head->next;
	lru_head->next = p->next;
	lru_tail->next = p;
	p->next = NULL;
	lru_tail = p;
}

void restart_lru()
{
	// printf("Into restart_lru!\n");
	pLRU p = lru_head->next;
	int i = R_GEN;
	while (p)
	{
		p->id = i++;
		p = p->next;
	}
}

int get_r()
{
	int r = lru_head->next->id;
	update_lru();
	return r;
}

void clear_desc(int r)
{
	rdesc[r].var = NULL;
}

void insert_desc(int r, SYM *n, int mod)
{
	/* Search through each register in turn looking for "n". There should be at most one of these. */
	int or ; /* Old descriptor */
	for (or = R_GEN; or < R_NUM; or ++)
	{
		if (rdesc[or].var == n)
		{
			/* Found it, clear it and break out of the loop. */
			clear_desc(or);
			break;
		}
	}

	/* Insert "n" in the new descriptor */

	rdesc[r].var = n;
	rdesc[r].modified = mod;
}

void spill_one(int r)
{
	if ((rdesc[r].var != NULL) && rdesc[r].modified)
	{
		if (rdesc[r].var->store == 1) /* local var */
		{
			// printf("	STO (R%u+%u),R%u\n", R_BP, rdesc[r].var->offset, r);
			// if(rdesc[r].var->useReg == 1) error("x0-x7被使用做通用寄存器\n");
			if (rdesc[r].var->useReg != 1)
				printf("	str w%u, [x29, #%d]\n", r, rdesc[r].var->offset);
		}
		else /* global var */
		{
			// printf("	LOD R%u,STATIC\n", R_TP);
			// printf("	STO (R%u+%u),R%u\n", R_TP, rdesc[r].var->offset, r);
			int rtmp = get_empty_r();
			printf("	adrp	x%u, %s\n", rtmp, rdesc[r].var->name);
			printf("	add	x%u, x%u, :lo12:%s\n", rtmp, rtmp, rdesc[r].var->name);
			printf("	str w%u, [x%u]\n", r, rtmp);
		}
		rdesc[r].modified = UNMODIFIED;
	}
}

int get_empty_r()
{
	int r = get_r();
	if (rdesc[r].var == NULL)
	{
		return r;
	}
	spill_one(r); /* Modified register */
	clear_desc(r);
	return r;
}

void spill_all(void)
{
	int r;
	for (r = R_GEN; r < R_NUM; r++)
		spill_one(r);
	restart_lru();
	// free_lru(); init_lru();
}

void flush_para(void)
{
	for (int r = 0; r < R_GEN; r++)
		spill_one(r), clear_desc(r);
}

void flush_all(void)
{
	int r;

	spill_all();

	for (r = R_GEN; r < R_NUM; r++)
		clear_desc(r);

	// free_lru(); init_lru();

	clear_desc(R_TP); /* Clear result register */
}

void load_reg(int r, SYM *n)
{
	int s;

	/* Look for a register */
	for (s = 0; s < R_NUM; s++)
	{
		if (rdesc[s].var == n)
		{
			printf("	mov w%u, w%u\n", r, s);
			insert_desc(r, n, rdesc[s].modified);
			return;
		}
	}

	/* Not in a reg. Load appropriately */
	switch (n->type)
	{
	case SYM_INT:
		printf("	mov w%u, #%d\n", r, n->value);
		break;

	case SYM_VAR:
		if (n->store == 1) /* local var */
		{
			// printf("	LOD R%u,(R%u+%d)\n", r, R_BP, n->offset);
			// if(rdesc[r].var->useReg == 1)
			// 	printf("	mov w%u, w%u\n", n->offset, r);
			// else
			printf("	ldr w%u, [x29, #%d]\n", r, n->offset);
		}
		else /* global var */
		{
			// printf("	LOD R%u,STATIC\n", R_TP);
			// printf("	LOD R%u,(R%u+%d)\n", r, R_TP, n->offset);

			// printf("	adrp x%u, %s\n", r, rdesc[r].var->name);
			// printf("	add	x%u, x%u, :lo12:%s\n", r, r, rdesc[r].var->name);
			// printf("	ldr w%u, [x%u]\n", r, r);

			printf("	ldr w%u, %s\n", r, n->name);
		}
		break;

	case SYM_ARRAY:
		n->store = n->array_base->store;
		if (n->array_index->type == SYM_INT)
		{
			n->offset = n->array_base->offset + n->array_index->value * 4;
			if (n->store == 1) /* local var */
			{
				printf("	ldr w%u, [x29, #%d]\n", r, n->offset);
			}
			else /* global var */
			{
				int rtmp = get_r();
				printf("	adr w%u, %s\n", rtmp, n->name);
				printf("	ldr w%u, [w%u, #%d]\n", r, rtmp, n->array_index->value * 4);
			}
		}
		else
		{
			int rt = get_reg(n->array_index);
			int rtmp = get_r();
			if (n->store == 1) /* local var */
			{
				printf("	mov x%u, #%d\n", rtmp, n->array_base->offset);
				printf("	add x%u, x%u, x29\n", rtmp, rtmp);
				printf("	ldr w%u, [x%u, x%u, lsl #2]\n", r, rtmp, rt);
			}
			else /* global var */
			{
				printf("	adr x%u, %s\n", rtmp, n->array_base->name);
				printf("	ldr w%u, [x%u, x%u, lsl #2]\n", r, rtmp, rt);
			}
		}
		break;

	case SYM_TEXT:
		// printf("	LOD R%u,L%u\n", r, n->label);
		printf("	adr x%u, L%u\n", r, n->label);
		break;
	}

	insert_desc(r, n, UNMODIFIED);
}

/* Get the first reg as a destination reg. */
int get_reg(SYM *c)
{
	int r;
	for (r = 0; r < R_NUM; r++) /* Already in a register */
	{
		if (rdesc[r].var == c)
		{
			spill_one(r);
			return r;
		}
	}
	r = get_r();
	if (rdesc[r].var == NULL)
	{
		load_reg(r, c);
		return r;
	}

	spill_one(r); /* Modified register */
	clear_desc(r);
	load_reg(r, c);
	return r;
}

void asm_bin(char *op, SYM *a, SYM *b, SYM *c)
{
	int reg1 = get_reg(b); /* Result register */
	int reg2 = get_reg(c); /* One more register */

	printf("	%s x%u, x%u, x%u\n", op, reg1, reg1, reg2);

	/* Delete c from the descriptors and insert a */
	clear_desc(reg1);
	insert_desc(reg1, a, MODIFIED);
}

void asm_cmp(int op, SYM *a, SYM *b, SYM *c)
{
	int reg1 = get_reg(b); /* Result register */
	int reg2 = get_reg(c); /* One more register */

	printf("	cmp w%u, w%u\n", reg1, reg2);
	// printf("	TST R%u\n", reg1);

	switch (op)
	{
	case TAC_EQ:
		printf("	cset w%u, eq\n", reg1);
		break;

	case TAC_NE:
		printf("	cset w%u, ne\n", reg1);
		break;

	case TAC_LT:
		printf("	cset w%u, lt\n", reg1);
		break;

	case TAC_LE:
		printf("	cset w%u, le\n", reg1);
		break;

	case TAC_GT:
		printf("	cset w%u, gt\n", reg1);
		break;

	case TAC_GE:
		printf("	cset w%u, ge\n", reg1);
		break;
	}

	/* Delete c from the descriptors and insert a */
	clear_desc(reg1);
	insert_desc(reg1, a, MODIFIED);
}

void asm_copy(SYM *a, SYM *b)
{
	if (scope_local)
	{
		int reg1 = get_reg(b); /* Load b into a register */
		if (a->type == SYM_ARRAY)
		{
			a->store = a->array_base->store;
			int rt = get_reg(a->array_index);
			int rtmp = get_r();
			if (a->store == 1) /* local var */
			{
				// printf("	ADD R%d,R%d\n", rt, R_BP);
				// printf("	STO (R%d),R%d\n", rt, reg1);
				printf("	mov x%u, #%d\n", rtmp, a->array_base->offset);
				printf("	add x%u, x%u, x29\n", rtmp, rtmp);
				printf("	str w%u, [x%u, x%u, lsl #2]\n", reg1, rtmp, rt);
			}
			else /* global var */
			{
				// printf("	LOD R4,STATIC\n");
				// printf("	ADD R%d,R%d\n", rt, R_TP);
				// printf("	STO (R%d),R%d\n", rt, reg1);
				printf("	adr x%u, %s\n", rtmp, a->array_base->name);
				printf("	str w%u, [x%u, x%u, lsl #2]\n", reg1, rtmp, rt);
			}
		}
		else
			insert_desc(reg1, a, MODIFIED); /* Indicate a is there */
	}
	else
	{
		a->value = b->value;
	}
}

void asm_cond(char *op, SYM *a, char *l)
{
	spill_all();

	if (a != NULL)
	{
		int r;

		for (r = R_GEN; r < R_NUM; r++) /* Is it in reg? */
		{
			if (rdesc[r].var == a)
				break;
		}

		if (r < R_NUM)
			printf("	cmp w%u, #0\n", r);
		else
			printf("	cmp w%u, #0\n", get_reg(a)); /* Load into new register */
	}

	printf("	%s %s\n", op, l);
}

void asm_return(SYM *a)
{
	if (a != NULL) /* return value */
	{
		spill_one(0);
		load_reg(0, a);
	}

	printf("	ldp x29, x30, [sp], #16\n"); /* restore bp */
	printf("	ret\n");					 /* return */
}

void asm_head()
{
	char head[] =
		"	# head\n"
		"	LOD R2,STACK\n"
		"	STO (R2),0\n"
		"	LOD R4,EXIT\n"
		"	STO (R2+4),R4";

	puts(head);
}

void asm_str(SYM *s)
{
	char *t = s->name; /* The text */
	int i;

	printf("L%u:\n", s->label); /* Label for the string */
	printf("	DBS ");			/* Label for the string */

	for (i = 1; t[i + 1] != 0; i++)
	{
		if (t[i] == '\\')
		{
			switch (t[++i])
			{
			case 'n':
				printf("%u,", '\n');
				break;

			case '\"':
				printf("%u,", '\"');
				break;
			}
		}
		else
			printf("%u,", t[i]);
	}

	printf("0\n"); /* End of string */
}

void asm_static(void)
{
	printf("	.data\n");
	int i;

	SYM *sl;

	for (sl = sym_tab_global; sl != NULL; sl = sl->next)
	{
		if (sl->type == SYM_TEXT)
			printf("L%u:	.asciz %s\n", sl->label, sl->name);
		else if (sl->type == SYM_VAR)
		{
			printf("	.align 2\n");
			printf("%s:	.word %d\n", sl->name, sl->value);
		}
		else if (sl->type == SYM_ARRAY)
		{
			printf("	.align 2\n");
			printf("%s:	.word ", sl->name);
			int i;
			for (i = 1; i < sl->value; i++)
				printf("%d, ", 0);
			if (sl->value > 0)
				printf("%d\n", 0);
		}
	}

	// printf("STATIC:\n");
	// printf("	DBN 0,%u\n", tos);
	// printf("STACK:\n");
}

void asm_code(TAC *c)
{
	int r;

	switch (c->op)
	{
	case TAC_UNDEF:
		error("cannot translate TAC_UNDEF");
		return;

	case TAC_ADD:
		asm_bin("add", c->a, c->b, c->c);
		return;

	case TAC_SUB:
		asm_bin("sub", c->a, c->b, c->c);
		return;

	case TAC_MUL:
		asm_bin("mul", c->a, c->b, c->c);
		return;

	case TAC_DIV:
		asm_bin("sdiv", c->a, c->b, c->c);
		return;

	case TAC_NEG:
		asm_bin("sub", c->a, mk_const(0), c->b);
		return;

	case TAC_EQ:
	case TAC_NE:
	case TAC_LT:
	case TAC_LE:
	case TAC_GT:
	case TAC_GE:
		asm_cmp(c->op, c->a, c->b, c->c);
		return;

	case TAC_COPY:
		asm_copy(c->a, c->b);
		return;

	case TAC_GOTO:
		asm_cond("b", NULL, c->a->name);
		return;

	case TAC_IFZ:
		asm_cond("beq", c->b, c->a->name);
		return;

	case TAC_LABEL:
		flush_all();
		if (c->a->type == SYM_FUNC)
		{
			printf("	.text\n");
			printf("	.type %s, %%function\n", c->a->name);
			printf("	.global %s\n", c->a->name);
		}
		printf("%s:\n", c->a->name);
		return;

	case TAC_ACTUAL:
		r = get_reg(c->a);
		oon -= 8;
		// printf("	STO (R2+%d),R%u\n", tof+oon, r);
		// printf("	str x%u, [x29, #%d]\n", r, tof + oon);
		if (!actualCnt)
			flush_para();
		printf("	mov x%u, x%u\n", actualCnt++, r);
		return;

	case TAC_CALL:
		flush_all();
		// printf("	STO (R2+%d),R2\n", tof+oon);	/* store old bp */
		// oon += 4;
		// printf("	LOD R4,R1+32\n"); 				/* return addr: 4*8=32 */
		// printf("	STO (R2+%d),R4\n", tof+oon);	/* store return addr */
		// oon += 4;
		// printf("	LOD R2,R2+%d\n", tof+oon-8);	/* load new bp */

		// int i,id=0,flag=0;
		// if(strcmp((char *)c->b,"printf")==0) id = (-oon)/8-1,flag=1;
		// for(i = oon; i < 0; i += 8)
		// {
		// 	if(id > 7)error("Too many args for a function, max is 8\n");
		// 	printf("	ldr x%u, [x29, #%d]\n",id, i + tof);
		// 	if(flag) id--;
		// 	else id++;
		// }
		// if((oon / 8)&1)
		// {
		// 	oon -= 8;
		// 	int offset = oon;
		// 	for(i = 0; i < id; i++,offset+=8)
		// 		printf("	str x%u, [x29, #%d]\n", i, offset);
		// }
		printf("	sub sp, sp, #%d\n", ((-tof - 1) / 16 + 1) * 16);
		printf("	bl %s\n", (char *)c->b); /* jump to new func */
		printf("	add sp, sp, #%d\n", ((-tof - 1) / 16 + 1) * 16);
		oon = 0;
		actualCnt = 0;
		if (c->a != NULL)
			insert_desc(0, c->a, MODIFIED);
		return;

	case TAC_BEGINFUNC:
		/* We reset the top of stack, since it is currently empty apart from the link information. */
		scope_local = 1;
		tof = LOCAL_OFF;
		oof = FORMAL_OFF;
		oon = 0;
		printf("	stp x29, x30, [sp, #-16]!\n");
		printf("	mov x29, sp\n");
		return;

	case TAC_FORMAL:
		c->a->store = 1; /* parameter is special local var */
		c->a->offset = oof;
		c->a->useReg = 1; // 用寄存器传参数。
		insert_desc(oof, c->a, UNMODIFIED);
		oof += 1;
		return;

	case TAC_VAR:
		if (scope_local)
		{
			c->a->store = 1; /* local var */
			tof -= 8;
			c->a->offset = tof;
		}
		else
		{
			c->a->store = 0; /* global var */
			c->a->offset = tos;
			tos += 4;
		}
		return;

	case TAC_ARRAY_DECLARE:
		if (scope_local)
		{
			c->a->store = 1; /* local array */
			tof -= 8 * c->a->value;
			c->a->offset = tof;
		}
		else
		{
			c->a->store = 0; /* global array */
			c->a->offset = tos;
			tos += 4 * c->a->value;
		}
		return;

	case TAC_RETURN:
		asm_return(c->a);
		return;

	case TAC_ENDFUNC:
		printf("	.size %s,(. - %s)\n\n", c->a->name, c->a->name);
		scope_local = 0;
		return;

	default:
		/* Don't know what this one is */
		error("unknown TAC opcode to translate");
		return;
	}
}

void tac_obj()
{
	tof = LOCAL_OFF; /* TOS allows space for link info */
	oof = FORMAL_OFF;
	oon = 0;

	int r;
	for (r = 0; r < R_NUM; r++)
		rdesc[r].var = NULL;
	insert_desc(0, mk_const(0), UNMODIFIED); /* R0 holds 0 */

	// asm_head();

	TAC *cur;
	for (cur = tac_first; cur != NULL; cur = cur->next)
	{
		printf("\n	// ");
		tac_print(cur);
		printf("\n");
		asm_code(cur);
	}
	// asm_lib();
	asm_static();
}
