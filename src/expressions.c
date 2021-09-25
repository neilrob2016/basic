#include "globals.h"

/* Doesn't need to be any bigger than the number of operator precedences but
   add a few on just in case */
#define MAX_EVAL_STACK 7

int getValue(st_runline *runline, int *pc, st_value *result);
int evalStack(
	int min_prec,
	st_value *val_stack,
	int *op_stack,
	int *val_stack_cnt,
	int *op_stack_cnt);
void evalPreOpStack(int *pre_op_stack, int pre_op_stack_cnt, st_value *result);
void clearStack(st_value *stack, int top);
int  skipRHSofAND(st_runline *runline, int pc, bool *is_end);
bool isEndOfExpression(st_token *token);



/*** Returns OK else error code ***/
int evalExpression(st_runline *runline, int *pc, st_value *result)
{
	st_value val_stack[MAX_EVAL_STACK];
	st_token *token;
	int op_stack[MAX_EVAL_STACK];
	int pre_op_stack[MAX_EVAL_STACK];
	int val_stack_cnt;
	int op_stack_cnt;
	int pre_op_stack_cnt;
	int pc2;
	int end_pc;
	int err;
	int prec;
	int i;
	bool is_end;

	for(i=0;i < MAX_EVAL_STACK;++i)
	{
		initValue(&val_stack[i]);
		op_stack[i] = -1;
		pre_op_stack[i] = -1;
	}

	/* These point to the next valid positions */
	val_stack_cnt = 0;
	op_stack_cnt = 0;
	pre_op_stack_cnt = 0;
	end_pc = runline->num_tokens - 1;
	initValue(result);

	/* Should have <val/expr> <op> <val/expr> */
	for(pc2=*pc;pc2 < runline->num_tokens;++pc2)
	{
		if (val_stack_cnt >= MAX_EVAL_STACK)
		{
			err = ERR_STACK_OVERFLOW;
			goto ERROR;
		}
		token = &runline->tokens[pc2];

		/* Check for single arg ops: NOT and ~ */
		if (IS_OP_TYPE(token,OP_NOT) || IS_OP_TYPE(token,OP_BIT_COMPL))
		{
			if (pre_op_stack_cnt == MAX_EVAL_STACK)
			{
				err = ERR_STACK_OVERFLOW;
				goto ERROR;
			}
			if (pc2 == end_pc)
			{
				err = ERR_SYNTAX;
				goto ERROR;
			}
			pre_op_stack[pre_op_stack_cnt++] = token->subtype;
			continue;
		}

		/* Check for sub expression */
		if (IS_OP_TYPE(token,OP_L_BRACKET))
		{
			/* Skip "(" */
			++pc2;

			clearValue(result);
			if ((err = evalExpression(runline,&pc2,result)) != OK)
				goto ERROR;

			/* Should now be at ")" at the end of the expression */
			if (pc2 > end_pc || 
			    !IS_OP_TYPE(&runline->tokens[pc2],OP_R_BRACKET))
			{
				err = ERR_SYNTAX;
				goto ERROR;
			}

			/* Skip ")" */
			++pc2;
		}
		else if ((err = getValue(runline,&pc2,result)) != OK)
			goto ERROR;

		if (token->negative)
		{
			if (result->type == VAL_STR)
			{
				/* Can't negate a string */
				err = ERR_INVALID_NEG;
				goto ERROR;
			}
			result->dval = -result->dval;
		}

		/* Can't use pre operators on strings */
		if (result->type == VAL_STR && pre_op_stack_cnt)
		{
			err = ERR_INVALID_ARG;
			goto ERROR;
		}
		else
		{
			evalPreOpStack(pre_op_stack,pre_op_stack_cnt,result);
			pre_op_stack_cnt = 0;
		}
		copyValue(&val_stack[val_stack_cnt++],result);
		clearValue(result);

		/* Are we at the end of the runline? */
		if (pc2 > end_pc) break;

		/* Get next token which could be an op or sub command */
		token = &runline->tokens[pc2];

		/* Are we at the end of the expression? */
		if (isEndOfExpression(token)) break;

		if (!IS_OP(token))
		{
			err = ERR_SYNTAX;
			goto ERROR;
		}
		if (pc2 == end_pc)
		{
			err = ERR_INCOMPLETE_EXPR;
			goto ERROR;
		}

		/* If precendence of current operator is <= than previous
		   operator(s) then evaluate stack down to equivalent level.
		   If current op is AND then force evaluation of LHS in order 
		   to do lazy evaluation. */
		prec = op_info[token->subtype].prec;
		if (IS_OP_TYPE(token,OP_AND) ||
		    (op_stack_cnt && prec <= op_info[op_stack[op_stack_cnt-1]].prec))
		{
			if ((err = evalStack(
				prec,
				val_stack,
				op_stack,
				&val_stack_cnt,
				&op_stack_cnt)) != OK) goto ERROR;

			/* Do lazy evaluation - ie if the LHS value of AND is 
			   false then don't bother evaluating the RHS. */
			if (IS_OP_TYPE(token,OP_AND) && 
			    !trueValue(&val_stack[val_stack_cnt-1]))
			{
				pc2 = skipRHSofAND(runline,pc2,&is_end);
				if (is_end)
				{
					*pc = pc2;
					setValue(result,VAL_NUM,NULL,0);
					clearStack(val_stack,val_stack_cnt);
					return OK;
				}
				token = &runline->tokens[pc2];
			}
		}
		op_stack[op_stack_cnt++] = token->subtype;
	}

	/* Evaluate remainder of stack */
	if ((err = evalStack(
		-1,
		val_stack,
		op_stack,
		&val_stack_cnt,
		&op_stack_cnt)) == OK) 
	{
		assert(val_stack[0].type != VAL_UNDEF);
		copyValue(result,&val_stack[0]);
		clearStack(val_stack,val_stack_cnt);
		*pc = pc2;
		return OK;
	}

	ERROR:
	clearStack(val_stack,val_stack_cnt);
	clearValue(result);
	return err;
}




int getValue(st_runline *runline, int *pc, st_value *result)
{
	st_token *token;
	st_value icnt_or_key;
	int index[MAX_INDEXES];
	int err;
	int pc2;

	clearValue(result);
	token = &runline->tokens[*pc];

	switch(token->type)
	{
	case TOK_NUM:
		setValue(result,VAL_NUM,NULL,token->dval);
		++*pc;
		break;

	case TOK_STR:
		setValue(result,VAL_STR,token->str,0);
		++*pc;
		break;

	case TOK_VAR:
		if (!token->var && !(token->var = getVariable(token->str)))
			return ERR_UNDEFINED_VAR;

		/* Get array index if there is one */
		if (++*pc < runline->num_tokens && 
		    IS_OP_TYPE(&runline->tokens[*pc],OP_L_BRACKET))
		{
			initValue(&icnt_or_key);
			if ((err = getVarIndexes(runline,pc,&icnt_or_key,index)) != OK)
				return err;
			err = getVarValue(token->var,&icnt_or_key,index,result);
			clearValue(&icnt_or_key);
		}
		else err = getVarValue(token->var,NULL,NULL,result);
		if (err != OK) return err;
		break;
	
	case TOK_FUNC:
		if ((err = callFunction(runline,pc,result)) != OK)
			return err;
		break;

	case TOK_OP:
	case TOK_COM:
		return ERR_SYNTAX;

	case TOK_DEFEXP:
		/* +1 to ignore '!' at start */
		if (!token->exp && !(token->exp = getDefExp(token->str+1)))
			return ERR_UNDEFINED_DEFEXP;

		pc2 = 3;
		if ((err = evalExpression(
			token->exp->runline,&pc2,result)) != OK)
			return err;
		++*pc;
		break;

	default:
		assert(0);
	}
	return OK;
}




/*** Evaluate entire stack down where whats left has lower precendence levels. 
     Returns 0 or 1 ***/
int evalStack(
	int min_prec,
	st_value *val_stack, 
	int *op_stack, int *val_stack_cnt, int *op_stack_cnt)
{
	st_value *val1;
	st_value *val2;
	double dval;
	int vp;
	int op;

	vp = *val_stack_cnt - 1;
	op = *op_stack_cnt - 1;
	for(;vp >= 0 && op >= 0 && op_info[op_stack[op]].prec >= min_prec;--op)
	{
		assert(vp > 0);
		assert(op >= 0);
		val2 = &val_stack[vp];
		val1 = &val_stack[--vp];

		/* Logical operators can work on mix of strings and numbers */
		switch(op_stack[op])
		{
		case OP_AND:
			setValue(
				val1,VAL_NUM,NULL,
				trueValue(val1) && trueValue(val2));
			continue;

		case OP_OR:
			setValue(
				val1,VAL_NUM,NULL,
				trueValue(val1) || trueValue(val2));
			continue;

		case OP_XOR:
			setValue(
				val1,VAL_NUM,NULL,
				trueValue(val1) ^ trueValue(val2));
			continue;

		default:
			break;
		}


		if (val1->type == VAL_STR)
		{
			/* Can multiply strings by numbers to do padding 
			   operations */
			if (val2->type == VAL_NUM && op_stack[op] != OP_MULT)
				return ERR_INVALID_ARG;

			/* Use setValue so it clears val1->sval */
			switch(op_stack[op])
			{
			case OP_EQUALS:
				setValue(val1,VAL_NUM,NULL,!strcmp(val1->sval,val2->sval));
				break;

			case OP_NOT_EQUALS:
				setValue(val1,VAL_NUM,NULL,!!strcmp(val1->sval,val2->sval));
				break;

			case OP_GREATER:
				setValue(val1,VAL_NUM,NULL,strcmp(val1->sval,val2->sval) > 0);
				break;

			case OP_GREATER_EQUALS:
				setValue(val1,VAL_NUM,NULL,strcmp(val1->sval,val2->sval) >= 0);
				break;

			case OP_LESS:
				setValue(val1,VAL_NUM,NULL,strcmp(val1->sval,val2->sval) < 0);
				break;

			case OP_LESS_EQUALS:
				setValue(val1,VAL_NUM,NULL,strcmp(val1->sval,val2->sval) <= 0);
				break;

			case OP_ADD:
				appendStringValue(val1,val2);
				break;

			case OP_SUB:
				subtractStringValue(val1,val2);
				break;

			case OP_MULT:
				if (val2->type == VAL_STR || val2->dval < 0)
					return ERR_INVALID_ARG;
				multStringValue(val1,val2->dval);
				break;

			case OP_DIV:
			case OP_INT_DIV:
			case OP_MOD:
			case OP_BIT_AND:
			case OP_BIT_OR:
			case OP_BIT_XOR:
			case OP_LEFT_SHIFT:
			case OP_RIGHT_SHIFT:
				return ERR_INVALID_ARG;

			default:
				return ERR_SYNTAX;
			}
			continue;
		}

		if (val2->type == VAL_STR)
		{
			/* Multiply strings. As above. */
			if (op_stack[op] != OP_MULT || val1->dval < 0)
				return ERR_INVALID_ARG;
			multStringValue(val2,val1->dval);
			copyValue(val1,val2);
			continue;
		}

		/* Numeric only arguments */
		switch(op_stack[op])
		{
		case OP_EQUALS:
			val1->dval = (val1->dval == val2->dval);
			break;

		case OP_NOT_EQUALS:
			val1->dval = (val1->dval != val2->dval);
			break;

		case OP_GREATER:
			val1->dval = (val1->dval > val2->dval);
			break;

		case OP_GREATER_EQUALS:
			val1->dval = (val1->dval >= val2->dval);
			break;

		case OP_LESS:
			val1->dval = (val1->dval < val2->dval);
			break;

		case OP_LESS_EQUALS:
			val1->dval = (val1->dval <= val2->dval);
			break;

		case OP_ADD:
			val1->dval += val2->dval;
			break;

		case OP_SUB:
			val1->dval -= val2->dval;
			break;

		case OP_MULT:
			val1->dval *= val2->dval;
			break;

		case OP_DIV:
			if (!val2->dval) return ERR_DIVIDE_BY_ZERO;
			val1->dval /= val2->dval;
			break;

		case OP_INT_DIV:
			if (!val2->dval) return ERR_DIVIDE_BY_ZERO;
			val1->dval = (long)val1->dval / (long)val2->dval;
			break;

		case OP_BIT_AND:
			val1->dval = (u_long)val1->dval & (u_long)val2->dval;
			break;

		case OP_BIT_OR:
			val1->dval = (u_long)val1->dval | (u_long)val2->dval;
			break;

		case OP_BIT_XOR:
			val1->dval = (u_long)val1->dval ^ (u_long)val2->dval;
			break;

		case OP_LEFT_SHIFT:
			/* Result of a left shift of >= 32 with int types is
			   undefined in C so zero it */
			if ((u_long)val2->dval >= sizeof(u_long) * 8)
				val1->dval = 0;
			else
				val1->dval = (u_long)val1->dval << (u_long)val2->dval;
			break;

		case OP_RIGHT_SHIFT:
			/* Right shifting isn't wrapped. Go figure */
			val1->dval = (u_long)val1->dval >> (u_long)val2->dval;
			break;

		case OP_MOD:
			if (!val2->dval) return ERR_DIVIDE_BY_ZERO;
			dval = (long)val1->dval % (long)val2->dval;
			/* Add back any floating point part of the 1st param */
			val1->dval = dval + (val1->dval - (long)val1->dval);
			break;

		default:
			return ERR_SYNTAX;
		}
	} /* End for() */

	*op_stack_cnt = op + 1;
	*val_stack_cnt = vp + 1;
	return OK;
}




/*** Evaluate from the top down ***/
void evalPreOpStack(int *pre_op_stack, int pre_op_stack_cnt, st_value *result)
{
	int i;

	for(i=pre_op_stack_cnt-1;i >= 0;--i)
	{
		if (pre_op_stack[i] == OP_NOT)
			result->dval = !result->dval;
		else if (pre_op_stack[i] == OP_BIT_COMPL)
			result->dval = ~(u_int)result->dval;
		else assert(0);
	}
}




void clearStack(st_value *stack, int cnt)
{
	int i;
	for(i=0;i < cnt;++i) clearValue(stack);
}




/*** Used when we need to skip the RHS of the AND clause in order to do lazy 
     evaluation ***/
int skipRHSofAND(st_runline *runline, int pc, bool *is_end)
{
	st_token *token;
	st_token *start_token = NULL;
	int brackets = 0;

	/* Find end of expression then return */
	*is_end = 1;
	for(++pc;pc < runline->num_tokens;++pc)
	{
		token = &runline->tokens[pc];
		if (!start_token)
		{
			if (token->lazy_jump)
			{
				*is_end = token->lazy_end;
				return token->lazy_jump;
			}
			start_token = token;
		}

		if (token->type == TOK_OP)
		{
			switch(token->subtype)
			{
			case OP_L_BRACKET:
				++brackets;
				break;
			case OP_R_BRACKET:
				if (--brackets < 0) goto DONE;
				break;
			case OP_OR:
			case OP_XOR:
				/* This allows expressions such as 0 AND 1 OR 1
				   to work properly by not skipping OR/XOR */
				if (brackets < 1)
				{
					*is_end = 0;
					goto DONE;
				}
				break;
			}
		}
		else if (brackets < 1 && isEndOfExpression(&runline->tokens[pc]))
			goto DONE;
	}

	DONE:
	if (start_token)
	{
		start_token->lazy_jump = pc;
		start_token->lazy_end = *is_end;
	}
	return pc;
}




/* Can only end on an operator if its a comma, semi colon or close right 
   bracket. Can also end on certain sub commands. */
bool isEndOfExpression(st_token *token)
{
	return IS_OP_TYPE(token,OP_COMMA) || 
	       IS_OP_TYPE(token,OP_SEMI_COLON) || 
	       IS_OP_TYPE(token,OP_R_BRACKET) ||
	       IS_COM_TYPE(token,COM_THEN) || 
	       IS_COM_TYPE(token,COM_TO) || 
	       IS_COM_TYPE(token,COM_FROM) || 
	       IS_COM_TYPE(token,COM_STEP);
}
