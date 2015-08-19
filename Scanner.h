enum typeoflex
{
    LEX_NULL,
    LEX_SERVICE,
    LEX_ID,
    LEX_FUNC,
    LEX_INT,
    LEX_FLOAT,
    LEX_STRING,
    LEX_OPERATOR,
    LEX_DEVIDER,      
};

class Token
{
public:
    typeoflex type;

    Token(typeoflex t = LEX_NULL): type(t) {}

    virtual void print() const = 0;

    virtual mystring gets() const { return mystring(""); }
    virtual int      geti() const { return 0; }
    virtual double   getd() const { return 0; }
};

class pointer
{
    int *pos;
public:

    pointer()
    {
        pos = new int;
    }

    pointer(int p)
    {
        pos = new int;
        *pos = p;
    }

    void operator=(pointer &p) { pos = p.pos; }

    void set_ptr(int p) { *pos = p; }

    int get_pos() const { return *pos; }
};

class Token_NULL: public Token
{
public:
    Token_NULL(): Token() {}
    void print() const { std::cout << "NULL\n"; }
};

class Token_Service: public Token
{ 
    char *word;
public:

    Token_Service(typeoflex t, const char *s): Token(t)
    {
        word = new char[strlen(s) + 1];
        strcpy(word, s);
    }

    mystring gets() const { return mystring(word); }

    void print() const { std::cout << word << " SERVICE\n"; }
};

class Token_Ident: public Token
{
    char *name;
public:

    Token_Ident(typeoflex t, const char *s): Token(t)
    {
        name = new char[strlen(s) + 1];
        strcpy(name, s);
    }

    mystring gets() const { return mystring(name); }

    void print() const 
    { 
        if (type == LEX_ID)
            std::cout << name << " IDENT\n";
        else 
            std::cout << name << " FUNC\n";
    }
};

class Token_Operator: public Token
{
    char *op;
public:

    Token_Operator(typeoflex t, const char *s): Token(t)
    {
        op = new char[8];
        strcpy(op, s);
    }

    mystring gets() const { return mystring(op); }

    void print() const { std::cout << op << " OPERATOR\n"; }
};

class Token_Devider: public Token
{
    char *dev;
public:

    Token_Devider(typeoflex t, const char *s): Token(t)
    {
        dev = new char[8];
        strcpy(dev, s);
    }

    mystring gets() const { return mystring(dev); }

    void print() const { std::cout << dev << " DEVIDER\n"; }
};

class Token_Int: public Token
{
    int val;
public:

    Token_Int(typeoflex t, const char *s): Token(t)
    {
        val = atoi(s);
    }

    Token_Int(typeoflex t, const int v): Token(t), val(v) {}

    int geti() const { return val; }

    void print() const { std::cout << val << " INT\n"; }
};

class Token_Double: public Token
{
    double val;
public:

    Token_Double(typeoflex t, const char *s): Token(t)
    {
        val = atof(s);
    }

    double getd() const { return val; }

    void print() const { std::cout << val << " DOUBLE" << std::endl; }
};

class Token_String: public Token
{
    char *str;
public:

    Token_String(typeoflex t, const char *s): Token(t)
    {
        str = new char[strlen(s) + 1];
        strcpy(str, s);
    }

    mystring gets() const { return mystring(str); }

    void print() const { std::cout << str << " STRING\n"; }
};

class Token_Jump: public Token
{
    pointer ptr;
public:

    Token_Jump(typeoflex t, pointer &p): Token(t) { ptr = p; }

    int geti() const { return ptr.get_pos(); }

    void print() const { std::cout << " JUMP\n"; }
};

class Scanner
{
    enum state{H, IDENT, DEC, FLOAT, STR, COMMENT, OPS, DELIM};

    static char *TS[];
    state CS;
    char buf[80];
    int buftop;

    int is_service(const char *s);

    void clear()
    {
        buftop = 0;
        *buf = '\0';
    }

    void add()
    {
        buf[buftop++] = c;
        buf[buftop] = '\0';
    }

    FILE *fp;
    int c;

    void gc()
    {
        c = fgetc(fp);
    }
public:

    void scan(std::list<Token *> &);

    Scanner (const char *program)
    {
        fp = fopen(program, "r");
        CS = H;
        clear();
        gc();
    }

    ~Scanner()
    {
        fclose(fp);
    }
};

char *Scanner::TS[] = 
{
    (char *)"if",
    (char *)"else",
    (char *)"elif",
    (char *)"else",
    (char *)"while",
    (char *)"for",
    (char *)"in",
    (char *)"continue",
    (char *)"break",
    (char *)"pass",
    (char *)"input",
    (char *)"print",
    (char *)"def",
    (char *)"return"
};

int Scanner::is_service(const char *s)
{
    for (char *it:TS)
        if (!strcmp(it, s)) return 1;
    return 0;
}

void Scanner::scan(std::list<Token *> &Lexems)
{
    CS = H;

    for(;;)
    {
        switch(CS)
        {
            case H:
                if ((c == ' ') || (c == '\n') || (c == '\r') || (c == '\t'))
                {
                    gc();
                }
                else if(isalpha(c))
                {
                    clear();
                    add();
                    gc();
                    CS = IDENT;
                }
                else if (isdigit(c))
                {
                    clear();
                    add();
                    gc();
                    CS = DEC;
                }
                else if (c == '#')
                {
                    gc();
                    CS = COMMENT;
                }
                else if (c == '"')
                {
                    clear();
                    gc();
                    CS = STR;
                }
                else if(strchr("+-*/!=><", c))
                {
                    clear();
                    add();
                    gc();
                    CS = OPS;
                }
                else if (c == EOF)
                {
                    Lexems.push_back(new Token_NULL());
                    return;
                }
                else
                    CS = DELIM;
            break;

            case IDENT:
                if (isalpha(c) || isdigit(c))
                {
                    add();
                    gc();
                }
                else
                {
                    if (is_service(buf))
                        Lexems.push_back(new Token_Service(LEX_SERVICE, buf));
                    else
                        Lexems.push_back(new Token_Ident(LEX_ID, buf));
                    CS = H;
                }
            break;

            case DEC:
                if (isdigit(c))
                {
                    add();
                    gc();
                }
                else if (c == '.')
                {
                    add();
                    gc();
                    CS = FLOAT;
                }
                else if (!strchr("+=-/*!<>; \t\n)}],", c))
                {
                    fprintf(stderr, "Wrong symbol '%c' after digit '%c'\n", c, buf[strlen(buf)-1]);
                    exit(1);
                }
                else
                {
                    Lexems.push_back(new Token_Int(LEX_INT, buf));
                    CS = H;
                }
            break;

            case FLOAT:
                if (isdigit(c))
                {
                    add();
                    gc();
                }
                else if (!strchr("+=-/*!<>; \t\n)}],", c))
                {
                    fprintf(stderr, "Wrong symbol '%c' after digit '%c'\n", c, buf[strlen(buf)-1]);
                    exit(1);
                }
                else
                {
                    Lexems.push_back(new Token_Double(LEX_FLOAT, buf));
                    CS = H;
                }
            break;

            case COMMENT:
                while ((c != '\n') && (c != EOF))
                    gc();

                CS = H;
            break;

            case OPS:
                if (c == '=')
                {
                    add();
                    gc();
                }

                Lexems.push_back(new Token_Operator(LEX_OPERATOR, buf));
                CS = H;
            break;

            case STR:
                if (c == '\"')
                {
                    gc();
                    Lexems.push_back(new Token_String(LEX_STRING, buf));
                    CS = H;
                }
                else if (c == EOF)
                {
                    std::cerr << "Error: '\"' expected, but EOF found\n";
                    exit(1);
                }
                else
                {
                    add();
                    gc();
                }
            break;

            case DELIM:
                clear();
                add();
                gc();

                Lexems.push_back(new Token_Devider(LEX_DEVIDER, buf));
                CS = H;
            break;
        }
    }
}