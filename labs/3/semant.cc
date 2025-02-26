#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include "semant.h"
#include "utilities.h"
#include <map>
#include <algorithm>
#include <vector>  

using namespace std;

extern int semant_debug;
extern char *curr_filename;

ClassTable *classtable;

static std::map<Symbol, Class_> class_map;

typedef std::pair<Symbol, Symbol> method_id;
std::map<method_id, method_class *> method_env;
//////////////////////////////////////////////////////////////////////
//
// Symbols
//
// For convenience, a large number of symbols are predefined here.
// These symbols include the primitive type and method names, as well
// as fixed names used by the runtime system.
//
//////////////////////////////////////////////////////////////////////
static Symbol 
    arg,
    arg2,
    Bool,
    concat,
    cool_abort,
    copy_sym,
    Int,
    in_int,
    in_string,
    IO,
    length,
    Main,
    main_meth,
    No_class,
    No_type,
    Object,
    out_int,
    out_string,
    prim_slot,
    self,
    SELF_TYPE,
    Str,
    str_field,
    substr,
    type_name,
    val;
//
// Initializing the predefined symbols.
//
static void initialize_constants(void)
{
    arg         = idtable.add_string("arg");
    arg2        = idtable.add_string("arg2");
    Bool        = idtable.add_string("Bool");
    concat      = idtable.add_string("concat");
    cool_abort  = idtable.add_string("abort");
    copy_sym    = idtable.add_string("copy");
    Int         = idtable.add_string("Int");
    in_int      = idtable.add_string("in_int");
    in_string   = idtable.add_string("in_string");
    IO          = idtable.add_string("IO");
    length      = idtable.add_string("length");
    Main        = idtable.add_string("Main");
    main_meth   = idtable.add_string("main");
    No_class    = idtable.add_string("_no_class");
    No_type     = idtable.add_string("_no_type");
    Object      = idtable.add_string("Object");
    out_int     = idtable.add_string("out_int");
    out_string  = idtable.add_string("out_string");
    prim_slot   = idtable.add_string("_prim_slot");
    self        = idtable.add_string("self");
    SELF_TYPE   = idtable.add_string("SELF_TYPE");
    Str         = idtable.add_string("String");
    str_field   = idtable.add_string("_str_field");
    substr      = idtable.add_string("substr");
    type_name   = idtable.add_string("type_name");
    val         = idtable.add_string("_val");
}

enum VisitState { UNVISITED, VISITING, VISITED };

bool dfs_check(Symbol cls_sym,
    std::map<Symbol, Class_>& class_map,
    std::map<Symbol, VisitState>& state) {
    state[cls_sym] = VISITING;
    Class_ cls = class_map[cls_sym];
    Symbol parent = cls->get_parent();

    if (parent != Object) { 
        if (class_map.find(parent) == class_map.end()) {
            classtable->semant_error(cls) << "Parent class " << parent << " is not defined." << std::endl;
            return true;
        }
        if (parent == Int || parent == Bool || parent == Str || parent == SELF_TYPE) {
            classtable->semant_error(cls) << "Classes cannot inherit from basic class " << parent << std::endl;
            return true;
        }
        if (state[parent] == VISITING)
            return true;

        if (state[parent] == UNVISITED) {
            if (dfs_check(parent, class_map, state))
                return true;
        }
    }
    state[cls_sym] = VISITED;
    return false;
}

bool check_inheritance(std::map<Symbol, Class_>& class_map) {
    std::map<Symbol, VisitState> state;
    
    // Initialize all classes as UNVISITED using traditional iterator
    std::map<Symbol, Class_>::iterator it;
    for (it = class_map.begin(); it != class_map.end(); ++it) {
        state[it->first] = UNVISITED;
    }
    
    // Check each unvisited class using traditional iterator
    for (it = class_map.begin(); it != class_map.end(); ++it) {
        Symbol cls_sym = it->first;
        if (state[cls_sym] == UNVISITED) {
            if (dfs_check(cls_sym, class_map, state)) {
                return true;  // Error found
            }
        }
    }
    return false;  // No errors found
}

ClassTable::ClassTable(Classes classes) : semant_errors(0) , error_stream(cerr) {

    /* Fill this in */
    install_basic_classes();

    if (classes == NULL) {
        semant_error() << "No classes defined." << endl;
        return;
    }

    for (int i = classes->first(); classes->more(i); i = classes->next(i))
    {
        Class_ c = classes->nth(i);
        Symbol class_name = c->get_name();

        if (class_name == SELF_TYPE) {
            semant_error(c) << "Redefinition of basic class SELF_TYPE." << endl;
            return;
        }

        if (class_name == No_class) {
            semant_error(c) << "Redefinition of basic class No_class." << endl;
            return;
        }

        if (class_map.find(class_name) != class_map.end()) {
            semant_error(c) << "Redefinition of class." << endl;
            return;
        }

        class_map[class_name] = c;

        if (class_map.find(Main) == class_map.end()) {
            semant_error(c) << "Class Main is not defined." << endl;
            return;
        }

        if (check_inheritance(class_map)) {
            return;
        }
    }
}

void ClassTable::install_basic_classes() {

    // The tree package uses these globals to annotate the classes built below.
   // curr_lineno  = 0;
    Symbol filename = stringtable.add_string("<basic class>");
    
    // The following demonstrates how to create dummy parse trees to
    // refer to basic Cool classes.  There's no need for method
    // bodies -- these are already built into the runtime system.
    
    // IMPORTANT: The results of the following expressions are
    // stored in local variables.  You will want to do something
    // with those variables at the end of this method to make this
    // code meaningful.

    // 
    // The Object class has no parent class. Its methods are
    //        abort() : Object    aborts the program
    //        type_name() : Str   returns a string representation of class name
    //        copy() : SELF_TYPE  returns a copy of the object
    //
    // There is no need for method bodies in the basic classes---these
    // are already built in to the runtime system.

    Class_ Object_class =
	class_(Object, 
	       No_class,
	       append_Features(
			       append_Features(
					       single_Features(method(cool_abort, nil_Formals(), Object, no_expr())),
					       single_Features(method(type_name, nil_Formals(), Str, no_expr()))),
			       single_Features(method(copy_sym, nil_Formals(), SELF_TYPE, no_expr()))),
	       filename);

    // 
    // The IO class inherits from Object. Its methods are
    //        out_string(Str) : SELF_TYPE       writes a string to the output
    //        out_int(Int) : SELF_TYPE            "    an int    "  "     "
    //        in_string() : Str                 reads a string from the input
    //        in_int() : Int                      "   an int     "  "     "
    //
    Class_ IO_class = 
	class_(IO, 
	       Object,
	       append_Features(
			       append_Features(
					       append_Features(
							       single_Features(method(out_string, single_Formals(formal(arg, Str)),
										      SELF_TYPE, no_expr())),
							       single_Features(method(out_int, single_Formals(formal(arg, Int)),
										      SELF_TYPE, no_expr()))),
					       single_Features(method(in_string, nil_Formals(), Str, no_expr()))),
			       single_Features(method(in_int, nil_Formals(), Int, no_expr()))),
	       filename);  

    //
    // The Int class has no methods and only a single attribute, the
    // "val" for the integer. 
    //
    Class_ Int_class =
	class_(Int, 
	       Object,
	       single_Features(attr(val, prim_slot, no_expr())),
	       filename);

    //
    // Bool also has only the "val" slot.
    //
    Class_ Bool_class =
	class_(Bool, Object, single_Features(attr(val, prim_slot, no_expr())),filename);

    //
    // The class Str has a number of slots and operations:
    //       val                                  the length of the string
    //       str_field                            the string itself
    //       length() : Int                       returns length of the string
    //       concat(arg: Str) : Str               performs string concatenation
    //       substr(arg: Int, arg2: Int): Str     substring selection
    //       
    Class_ Str_class =
	class_(Str, 
	       Object,
	       append_Features(
			       append_Features(
					       append_Features(
							       append_Features(
									       single_Features(attr(val, Int, no_expr())),
									       single_Features(attr(str_field, prim_slot, no_expr()))),
							       single_Features(method(length, nil_Formals(), Int, no_expr()))),
					       single_Features(method(concat, 
								      single_Formals(formal(arg, Str)),
								      Str, 
								      no_expr()))),
			       single_Features(method(substr, 
						      append_Formals(single_Formals(formal(arg, Int)), 
								     single_Formals(formal(arg2, Int))),
						      Str, 
						      no_expr()))),
	       filename);

    class_map[Object] = Object_class;
    class_map[IO] = IO_class;
    class_map[Int] = Int_class;
    class_map[Bool] = Bool_class;
    class_map[Str] = Str_class;
}

////////////////////////////////////////////////////////////////////
//
// semant_error is an overloaded function for reporting errors
// during semantic analysis.  There are three versions:
//
//    ostream& ClassTable::semant_error()                
//
//    ostream& ClassTable::semant_error(Class_ c)
//       print line number and filename for `c'
//
//    ostream& ClassTable::semant_error(Symbol filename, tree_node *t)  
//       print a line number and filename
//
///////////////////////////////////////////////////////////////////

ostream& ClassTable::semant_error(Class_ c)
{                                                             
    return semant_error(c->get_filename(),c);
}    

ostream& ClassTable::semant_error(Symbol filename, tree_node *t)
{
    error_stream << filename << ":" << t->get_line_number() << ": ";
    return semant_error();
}

ostream& ClassTable::semant_error()                  
{                                                 
    semant_errors++;                            
    return error_stream;
} 

bool cls_is_defined(Symbol cls_name) {
    return cls_name == SELF_TYPE || class_map.find(cls_name) != class_map.end();
}

bool is_subclass(Symbol child, Symbol parent, type_env &tenv) {
    if (child == SELF_TYPE) {
        if (parent == SELF_TYPE) {
            return true;
        }
        child = tenv.c->get_name();

    }

    for (Symbol cls = child; cls != Object; cls = class_map[cls]->get_parent()) {
        if (cls == parent) {
            return true;
        }
    }
    return false;
}

Symbol cls_join(Symbol t1, Symbol t2, type_env &tenv) {
    if (t1 == SELF_TYPE) {
        t1 = tenv.c->get_name();
    }
    if (t2 == SELF_TYPE) {
        t2 = tenv.c->get_name();
    }

    Class_ cl = class_map[t1];

    for (; !is_subclass(t2, cl->get_name(), tenv); cl = class_map[cl->get_parent()]) {
    }

    return cl->get_name();
}

// Type checking


Symbol int_const_class::typecheck(type_env &tenv) {
    return Int;
}

Symbol bool_const_class::typecheck(type_env &tenv) {
    return Bool;
}

Symbol string_const_class::typecheck(type_env &tenv) {
    return Str;
}

Symbol no_expr_class::typecheck(type_env &tenv) {
    return No_type;
}

Symbol isvoid_class::typecheck(type_env &tenv) {
    e1->typecheck(tenv);
    return Bool;
}

Symbol new__class::typecheck(type_env &tenv) {
    if (!cls_is_defined(type_name)) {
        classtable->semant_error() << "Class " << type_name << " is not defined." << std::endl;
        return Object;
    }
    return type_name;
}

Symbol comp_class::typecheck(type_env &tenv) {
    Symbol t1 = e1->typecheck(tenv);
    if (t1 != Bool) {
        classtable->semant_error() << "Argument of 'not' has type " << t1 << " instead of Bool." << std::endl;
    }
    return Bool;
}

Symbol attr_class::typecheck(type_env &tenv) {
    if (name == self) {
        classtable->semant_error() << "'self' cannot be the name of an attribute." << std::endl;
        return Object;
    }

    Symbol t0 = type_decl;
    Symbol t1 = init->typecheck(tenv);

    if (t1 == No_type) {
        return t0;
    }

    if (!is_subclass(t1, t0, tenv)) {
        classtable->semant_error() << "Inferred type " << t1 << " of initialization of attribute " << name << " does not conform to declared type " << t0 << "." << std::endl;
    }

}

Symbol loop_class::typecheck(type_env &tenv) {
    if (pred->typecheck(tenv) != Bool) {
        classtable->semant_error() << "Loop condition does not have type Bool." << std::endl;
    }
    body->typecheck(tenv);
    return Object;
}

Symbol block_class::typecheck(type_env &tenv) {
    for (int i = body->first(); body->more(i); i = body->next(i)) {
        type = body->nth(i)->typecheck(tenv);
    }
    return type;
}

Symbol cond_class::typecheck(type_env &tenv) {
    Symbol t1 = pred->typecheck(tenv);
    Symbol t2 = then_exp->typecheck(tenv);
    Symbol t3 = else_exp->typecheck(tenv);

    if (t1 != Bool) {
        classtable->semant_error() << "Predicate of 'if' does not have type Bool." << std::endl;
    }
    return cls_join(t2, t3, tenv);
}

Symbol plus_class::typecheck(type_env &tenv) {
    if (e1->typecheck(tenv) != Int || e2->typecheck(tenv) != Int) {
        classtable->semant_error() << "non-Int arguments: " << e1->typecheck(tenv) << " + " << e2->typecheck(tenv) << std::endl;
    }
    return Int;
}

Symbol sub_class::typecheck(type_env &tenv) {
    if (e1->typecheck(tenv) != Int || e2->typecheck(tenv) != Int) {
        classtable->semant_error() << "non-Int arguments: " << e1->typecheck(tenv) << " - " << e2->typecheck(tenv) << std::endl;
    }
    return Int;
}

Symbol mul_class::typecheck(type_env &tenv) {
    if (e1->typecheck(tenv) != Int || e2->typecheck(tenv) != Int) {
        classtable->semant_error() << "non-Int arguments: " << e1->typecheck(tenv) << " * " << e2->typecheck(tenv) << std::endl;
    }
    return Int;
}

Symbol divide_class::typecheck(type_env &tenv) {
    if (e1->typecheck(tenv) != Int || e2->typecheck(tenv) != Int) {
        classtable->semant_error() << "non-Int arguments: " << e1->typecheck(tenv) << " / " << e2->typecheck(tenv) << std::endl;
    }
    return Int;
}

Symbol neg_class::typecheck(type_env &tenv) {
    if (e1->typecheck(tenv) != Int) {
        classtable->semant_error() << "Argument of '~' has type " << e1->typecheck(tenv) << " instead of Int." << std::endl;
    }
    return Int;
}

Symbol lt_class::typecheck(type_env &tenv) {
    if (e1->typecheck(tenv) != Int || e2->typecheck(tenv) != Int) {
        classtable->semant_error() << "non-Int arguments: " << e1->typecheck(tenv) << " < " << e2->typecheck(tenv) << std::endl;
    }
    return Bool;
}

Symbol eq_class::typecheck(type_env &tenv) {
    Symbol t1 = e1->typecheck(tenv);
    Symbol t2 = e2->typecheck(tenv);

    if ((t1 == Int || t1 == Bool || t1 == Str) && t1 != t2) {
        classtable->semant_error() << "Illegal comparison with a basic type." << std::endl;
    }
    return Bool;
}

Symbol leq_class::typecheck(type_env &tenv) {
    if (e1->typecheck(tenv) != Int || e2->typecheck(tenv) != Int) {
        classtable->semant_error() << "non-Int arguments: " << e1->typecheck(tenv) << " <= " << e2->typecheck(tenv) << std::endl;
    }
    return Bool;
}





/*   This is the entry point to the semantic checker.

     Your checker should do the following two things:

     1) Check that the program is semantically correct
     2) Decorate the abstract syntax tree with type information
        by setting the `type' field in each Expression node.
        (see `tree.h')

     You are free to first do 1), make sure you catch all semantic
     errors. Part 2) can be done in a second stage, when you want
     to build mycoolc.
 */
void program_class::semant()
{
    initialize_constants();

    /* ClassTable constructor may do some semantic analysis */
    ClassTable *classtable = new ClassTable(classes);

    /* some semantic analysis code may go here */

    if (classtable->errors()) {
	cerr << "Compilation halted due to static semantic errors." << endl;
	exit(1);
    }
}


