typedef std::vector<Poliz_elem *>    poliz_t;
typedef std::stack<Poliz_elem *>     poliz_stack;


struct var_table_elem
{
    mystring name;
    Var *elem;

    var_table_elem() {}
    var_table_elem(const mystring &s, Var *el): name(s), elem(el) {}
};
typedef std::vector<var_table_elem *>  var_table;


struct func_table_elem
{
    mystring    name;
    int         params;
    var_table   variables;
    poliz_t     poliz;

    func_table_elem() {}
    func_table_elem(const mystring &s): name(s), params(0) {}
};
typedef std::vector<func_table_elem *> func_table;


Var *addvar(const mystring &name, var_table &table)
{
    for (auto it:table)
    {
        if (it->name == name)
            return it->elem;
    }
    table.push_back(new var_table_elem(name, new Var(0)));
    return table.back()->elem;
}

class Parser
{
    std::list<Token *> Lexems;

    func_table      funcs;
    poliz_stack     Stack;

    poliz_stack break_stack;
    poliz_stack continue_stack;
    poliz_stack return_stack;

    Token *cur_lex;
    std::list<Token *>::iterator it;

    int in_cycle;

    void P();
    void F0();
    void S(poliz_t &, var_table &);
    void S_IF(poliz_t &, var_table &);
    void S_WHILE(poliz_t &, var_table &);
    void S_FOR(poliz_t &, var_table &);
    void E(poliz_t &, var_table &);
    void E1(poliz_t &, var_table &);
    void E2(poliz_t &, var_table &);
    void E3(poliz_t &, var_table &);

    func_table_elem *get_func(const mystring &);

public:
    Parser(std::list<Token *> &L): Lexems(L)
    {
        in_cycle = 0;
        funcs.push_back(new func_table_elem(mystring("main")));
        it = Lexems.begin();
        cur_lex = *it;
    }

    Token *gl() { cur_lex = *(++it); cur_lex->print(); return cur_lex; }

    void run();
    void print();
    void print_poliz(const poliz_t &, const var_table &);
    func_table &get_table() { return funcs; }
};

var_table_elem *findvar(const Var *addr, const var_table &table)
{
    for (auto it: table)
    {
        if (addr == it->elem)
            return it;
    }
    return NULL;
}

void Parser::print_poliz(const poliz_t &poliz, const var_table &variables)
{
    for (int it = 0; it < poliz.size(); ++it)
    {
        std::cout << it << ": ";
        if (poliz[it]->type == POLIZ_VAR && findvar(dynamic_cast<Var *>(poliz[it]), variables))
            std::cout << findvar(dynamic_cast<Var *>(poliz[it]), variables)->name.ptr() << " | VAR\n";
        else
            poliz[it]->print();
    }
}

void Parser::print()
{
    for (auto it: funcs)
    {
        std::cout << "function:  " << it->name.ptr() << std::endl;
        std::cout << "arguments: " << it->params << std::endl;

        std::cout << "variables: ";
        for (auto it2: it->variables)
            std::cout << it2->name.ptr() << ", ";
        std::cout << std::endl;

        std::cout << "poliz:" << std::endl;
        print_poliz(it->poliz, it->variables);
        std::cout << "------------------------------------------\n";
    }
}

func_table_elem *Parser::get_func(const mystring &s)
{
    for (auto it: funcs)
    {
        if (it->name == s)
            return it;
    }

    return NULL;
}

void Parser::run()
{
    P();
    std::cout << "The program is correct\n";
}

void Parser::P()
{
    while (cur_lex->type != LEX_NULL)
    {
        if (cur_lex->gets() == "def") F0();
        else                          S(funcs[0]->poliz, funcs[0]->variables);   
    }
}

void Parser::F0()
{
    func_table_elem *func = new func_table_elem;
    func->params = 0;

    Var *p1 = new Var(0);

    return_stack.push(p1);

    if (gl()->type != LEX_ID) throw Exception("Error: wrong function declaration. Identifier expected.");

    if (get_func(cur_lex->gets())) throw Exception("Error: re-declaration of a function.");

    func->name = cur_lex->gets();

    if (gl()->gets() != "(") throw Exception("Error: wrong function declaration. '(' expected.");

    if (gl()->type == LEX_ID)
    {
        func->params++;
        func->poliz.push_back(addvar(cur_lex->gets(), func->variables));

        while (gl()->gets() == ",")
        {
            if (gl()->type != LEX_ID) throw Exception("Error: wrong function declaration. Argument expected.");
            func->params++;
            func->poliz.push_back(addvar(cur_lex->gets(), func->variables));
        }
    }

    if (cur_lex->gets() != ")") throw Exception("Error: wrong function declaration. ')' expected.");

    if (gl()->gets() != "{") throw Exception("Error: wrong function declaration. '{' expected.");

    gl();
    while (cur_lex->gets() != "}") S(func->poliz, func->variables);

    func->poliz.push_back(new Var(0));
    func->poliz.push_back(new Jump(POLIZ_RET));
    funcs.push_back(func);
    gl();

    *p1 = Var((int) func->poliz.size());
    return_stack.pop();
} 

void Parser::S(poliz_t &poliz, var_table &variables) 
{
    if (cur_lex->gets() == "break")
    {
        if (break_stack.empty()) throw Exception("Error: Incorrect use of break.");
        if (gl()->gets() != ";") throw Exception("Error: ';' expected.");

        poliz.push_back(break_stack.top());
        poliz.push_back(new Jump(POLIZ_JMP));

        gl();
    } 
    else if (cur_lex->gets() == "continue")
    {

        if (continue_stack.empty()) throw Exception("Error: Incorrect use of continue.");
        if (gl()->gets() != ";") throw Exception("Error: ';' expected.");

        poliz.push_back(continue_stack.top());
        poliz.push_back(new Jump(POLIZ_JMP));

        gl();
    } 
    else if (cur_lex->gets() == "return")
    {
        if (return_stack.empty()) throw Exception("Error: Incorrect use of return.");

        //poliz.push_back(return_stack.top());
        gl(); 
        E(poliz, variables); 
        if (cur_lex->gets() != ";") throw Exception("Error: ';' expected.");
        poliz.push_back(new Jump(POLIZ_RET));
                
        gl();
    }
    else if (cur_lex->gets() == "print") 
    {     
        gl(); E(poliz, variables); 

        poliz.push_back(new Operator(mystring("print")));

        if (cur_lex->gets() != ";") throw Exception("Error: ';' expected.");
            
        gl();
    } 
    else if (cur_lex->gets() == "if") 
    {
        gl(); S_IF(poliz, variables);
    } 
    else if (cur_lex->gets() == "while") 
    {
        gl(); S_WHILE(poliz, variables);
    }
    else if (cur_lex->gets() == "for") 
    {
        gl(); S_FOR(poliz, variables);
    }
    else if (cur_lex->type == LEX_ID)
    {
        poliz.push_back(addvar(cur_lex->gets(), variables));

        mystring tmp = gl()->gets();
        if ((tmp != "=") && (tmp != "+=") && (tmp != "-=") && (tmp != "*=") && (tmp != "/=")) throw Exception("Error: assignment expected after identifier.");

        gl(); E(poliz, variables);
        poliz.push_back(new Operator(tmp));

        if (cur_lex->gets() != ";") throw Exception("Error: ';' expected.");
        poliz.push_back(new Operator(cur_lex->gets()));

        gl();
    }
    else throw Exception("Error: operator expected.");
}

void Parser::S_IF(poliz_t &poliz, var_table &variables)
{
    Var *p1 = new Var(0),
        *p2 = new Var(0);

    poliz.push_back(p1);
    E(poliz, variables);
    poliz.push_back(new Jump(POLIZ_JNE));

    if (cur_lex->gets() != "{") throw Exception("IF-operator Error: '{' expected.");

    gl();
    while (cur_lex->gets() != "}")
        S(poliz, variables);

    poliz.push_back(p2);
    poliz.push_back(new Jump(POLIZ_JMP));
    *p1 = Var((int) poliz.size());

    if (gl()->gets() == "elif")
    {
        gl(); 
        S_IF(poliz, variables);
    }
    else if (cur_lex->gets() == "else")
    {
        if (gl()->gets() != "{") throw Exception("IF-operator ELSE Error: '{' expected.");
                
        gl();
        while (cur_lex->gets() != "}")
            S(poliz, variables);

        gl();
    }

    *p2 = Var((int) poliz.size());
}

void Parser::S_WHILE(poliz_t &poliz, var_table &variables)
{
    Var *p1 = new Var(0),
        *p2 = new Var(0);

    break_stack.push(p2);
    continue_stack.push(p1);

    *p1 = Var((int) poliz.size());

    poliz.push_back(p2);
    E(poliz, variables);
    poliz.push_back(new Jump(POLIZ_JNE));

    if (cur_lex->gets() != "{") throw Exception("WHILE-loop Error: '{' expected.");

    gl();
    while (cur_lex->gets() != "}")  S(poliz, variables); 

    poliz.push_back(p1); 
    poliz.push_back(new Jump(POLIZ_JMP));

    *p2 = Var((int) poliz.size());

    gl();
    break_stack.pop();
    continue_stack.pop();
}

//ITER IDENT EXPR [p2] FOR JNE

void Parser::S_FOR(poliz_t &poliz, var_table &variables)
{
    Var *p1 = new Var(0),
        *p2 = new Var(0);

    break_stack.push(p2);
    continue_stack.push(p1);

    poliz.push_back(new Var(0));

    *p1 = Var((int) poliz.size());

    if (cur_lex->type != LEX_ID) throw Exception("FOR_loop Error: Identifier expected.");

    poliz.push_back(addvar(cur_lex->gets(), variables));

    if (gl()->gets() != "in") throw Exception("FOR_loop Error: 'in' expected.");

    gl(); 
    E(poliz, variables);
    poliz.push_back(p2);
    poliz.push_back(new Operator("for"));
    poliz.push_back(new Jump(POLIZ_JNE));

    if (cur_lex->gets() != "{") throw Exception("FOR_loop Error: '{' expected.");

    gl();
    while (cur_lex->gets() != "}") S(poliz, variables);

    poliz.push_back(p1);
    poliz.push_back(new Jump(POLIZ_JMP));
    *p2 = Var((int) poliz.size());

    gl();
    break_stack.pop();
    continue_stack.pop();
}

void Parser::E(poliz_t &poliz, var_table &variables)
{
    E1(poliz, variables);

    mystring op = cur_lex->gets();
    if ((op == "==") || (op == "<") || (op == ">") || (op == "<=") || (op == ">=") || (op == "!="))
    {
        gl(); E1(poliz, variables);

        poliz.push_back(new Operator(op));
    }
}

void Parser::E1(poliz_t &poliz, var_table &variables)
{
    E2(poliz, variables);

    int it = 0;
    mystring op = cur_lex->gets();
    while ((op == "+") || (op == "-") || (op == "+=") || (op == "-=") || (op == "or"))
    {
        gl(); E2(poliz, variables);

        Stack.push(new Operator(op));
        op = cur_lex->gets();
        it++;
    }

    while ((it--))
    {
        poliz.push_back(Stack.top());
        Stack.pop();
    }
}

void Parser::E2(poliz_t &poliz, var_table &variables)
{
    E3(poliz, variables);

    int it = 0;
    mystring op = cur_lex->gets();
    while ((op == "*") || (op == "/") || (op == "*=") || (op == "/=") || (op == "and"))
    {
        gl(); E3(poliz, variables);

        Stack.push(new Operator(op));
        op = cur_lex->gets();
        it++;
    }

    while ((it--))
    {
        poliz.push_back(Stack.top());
        Stack.pop();
    }
}

void Parser::E3(poliz_t &poliz, var_table &variables)
{
    if (cur_lex->type == LEX_NULL)
        throw Exception("Error: expression expected, but EOF found.");

    else if (cur_lex->type == LEX_STRING)
    {
        poliz.push_back(new Var(cur_lex->gets()));
        gl();
    }

    else if (cur_lex->type == LEX_INT)
    {
        poliz.push_back(new Var(cur_lex->geti()));
        gl();
    }

    else if (cur_lex->type == LEX_FLOAT)
    {
        poliz.push_back(new Var(cur_lex->getd()));
        gl();
    }

    else if (cur_lex->type == LEX_ID)
    {
        Token *tmp = cur_lex;
        gl();

        if (cur_lex->gets() == "(")
        {
            int it = 1;

            if (!get_func(tmp->gets())) throw Exception("Error: Implicit use of a function.");

            gl(); E(poliz, variables);
            while (cur_lex->gets() == ",")
            {
                it++;
                gl(); E(poliz, variables);
            }

            poliz.push_back(new Func(tmp->gets()));

            if (cur_lex->gets() != ")") throw Exception("Error: ')' expected in function call.");
            
            if (it != get_func(tmp->gets())->params) throw Exception("Error: Incorrect count of arguments to function.");

            gl();
            return;
        }

        poliz.push_back(addvar(tmp->gets(), variables));

        if (cur_lex->gets() == "=")
        {
            Stack.push(new Operator(cur_lex->gets()));
            gl(); 
            E(poliz, variables);
            poliz.push_back(Stack.top());
            Stack.pop();
        }
    }

    else if (cur_lex->gets() == "input")
    {
        if (gl()->gets() != "(") throw Exception("input Error: '(' expected.");

        if (gl()->gets() != ")") throw Exception("input Error: ')' expected.");

        poliz.push_back(new Operator(mystring("input")));
        
        gl();
    }

    else if (cur_lex->gets() == "not")
    {
        Stack.push(new Operator(cur_lex->gets()));
        gl(); 
        E3(poliz, variables);
        poliz.push_back(Stack.top());
        Stack.pop();
    }

    else if (cur_lex->gets() == "(")
    {
        gl(); 
        E(poliz, variables);
        if (cur_lex->gets() != ")") throw Exception("expression Error: ')' expected.");
        
        gl();
    }

    else throw Exception("Error: invalid expression.");
}