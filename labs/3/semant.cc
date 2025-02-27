// 
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include "utilities.h"
#include "semant.h"
#include <map>
#include <algorithm>
#include <vector>  


extern int semant_debug;
extern char *curr_filename;

ClassTable *classtable;

std::map<Symbol, Class_> class_map;

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
    install_basic_classes();

    for(int i = classes->first(); classes->more(i); i = classes->next(i)) {
        Class_ cls = classes->nth(i);
        Symbol name = cls->get_name();

        if (class_map.find(name) != class_map.end()) {
            semant_error(cls) << "redefinition of class " << name << "." << std::endl;
            return;
        }

        if (name == SELF_TYPE) {
            semant_error(cls) << "Redefinition of basic class SELF_TYPE." << std::endl;
            return;
        }

        class_map.insert(std::make_pair(name, cls));
    }

    if (class_map.find(Main) == class_map.end()) {
        semant_error() << "Class Main is not defined." << std::endl;
        return;
    }

    for (int i = classes->first(); classes->more(i); i = classes->next(i)) {
        Class_ cls = classes->nth(i);
        Symbol starting_class = cls->get_name();

        for (Symbol parent = cls->get_parent(); parent != Object; cls = class_map[parent], parent = cls->get_parent()) {
            if (class_map.find(parent) == class_map.end()) {
                semant_error(cls) << "Parent class " << parent << " is not defined." << std::endl;
                return;
            }

            if (parent == Int || parent == Bool || parent == Str || parent == SELF_TYPE) {
                semant_error(cls) << "Classes cannot inherit from basic class" << parent << std::endl;
                return;
            }

            if (parent == starting_class) {
                semant_error(cls) << "An inheritance cycle has been detected." << std::endl;
                return;
            }
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

    class_map.insert(std::make_pair(Object, Object_class));
    class_map.insert(std::make_pair(IO, IO_class));
    class_map.insert(std::make_pair(Int, Int_class));
    class_map.insert(std::make_pair(Bool, Bool_class));
    class_map.insert(std::make_pair(Str, Str_class));
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


///////////////////////////////////////////////////////////////////


bool cls_is_defined(Symbol cls_name) {
    if (cls_name == SELF_TYPE) {
        return true;
    }
    if (class_map.find(cls_name) == class_map.end()) {
        return false;
    }
    return true;
}

bool is_subclass(Symbol child, Symbol parent, type_env &tenv) {
    if (child == SELF_TYPE) {
        if (parent == SELF_TYPE) {
            return true;
        }
        child = tenv.c->get_name();
    }

    for (auto c_it = class_map.find(child); c_it != class_map.end(); c_it = class_map.find(c_it->second->get_parent())) {
        if (c_it->second->get_name() == parent) {
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

method_class *method_in_class(Symbol cls_name, Symbol method_name) {
    auto it = method_env.find(std::make_pair(cls_name, method_name));
    if (it != method_env.end()) {
        return it->second;
    }
    return nullptr;
}

method_class *lookup_method(Symbol cls_name, Symbol method_name) {
    for (auto c_it = class_map.find(cls_name); c_it != class_map.end(); c_it = class_map.find(c_it->second->get_parent())) {
        method_class *mth = method_in_class(c_it->second->get_parent(), method_name);
        if (mth) {
            return mth;
        }
    }

    return nullptr;
}

// Type checking


Symbol int_const_class::typecheck(type_env &tenv) {
    type = Int;
    return type;
}

Symbol bool_const_class::typecheck(type_env &tenv) {
    type = Bool;
    return type;
}

Symbol string_const_class::typecheck(type_env &tenv) {
    type = Str;
    return type;
}

Symbol no_expr_class::typecheck(type_env &tenv) {
    type = No_type;
    return type;
}

Symbol isvoid_class::typecheck(type_env &tenv) {
    e1->typecheck(tenv);
    type = Bool;
    return type;
}

Symbol new__class::typecheck(type_env &tenv) {
    Symbol t = type_name;
    if (!cls_is_defined(t)) {
        classtable->semant_error(tenv.c -> get_filename(), this) << "Class " << t << " is not defined." << std::endl;
        type = Object;
    } else {
        type = t;
    }
    return type;
}

Symbol comp_class::typecheck(type_env &tenv) {
    type = e1->typecheck(tenv);
    if (type != Bool) {
        classtable->semant_error(tenv.c->get_filename(), this) << "Argument of 'not' has type " << type << " instead of Bool." << std::endl;
    }
    type = Bool;
    return type;
}

Symbol attr_class::typecheck(type_env &tenv) {
    if (name == self) {
        classtable->semant_error(tenv.c->get_filename(), this) << "'self' cannot be the name of an attribute." << std::endl;
        return Object;
    }

    Symbol t0 = type_decl;
    Symbol t1 = init->typecheck(tenv);

    if (t1 != No_type && !is_subclass(t1, t0, tenv)) {
        classtable->semant_error(tenv.c->get_filename(), this) <<
            "Inferred type " << t1 << " of initialization of " << name <<
            " does not conform to identifier's declared type " << t0 << "." << std::endl;
    }

    return t0;
}

Symbol loop_class::typecheck(type_env &tenv) {
    if (pred->typecheck(tenv) != Bool) {
        classtable->semant_error(tenv.c->get_filename(), this) << "Loop condition does not have type Bool." << std::endl;
    }
    body->typecheck(tenv);
    type = Object;
    return type;
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
        classtable->semant_error(tenv.c->get_filename(), this) << "Predicate of 'if' does not have type Bool." << std::endl;
    }
    type = cls_join(t2, t3, tenv);
    return type;
}

Symbol plus_class::typecheck(type_env &tenv) {
    if (e1->typecheck(tenv) != Int || e2->typecheck(tenv) != Int) {
        classtable->semant_error(tenv.c->get_filename(), this) << "non-Int arguments: " << e1->typecheck(tenv) << " + " << e2->typecheck(tenv) << std::endl;
        type = Object;
    } else {
        type = Int;
    }
    return type;
}

Symbol sub_class::typecheck(type_env &tenv) {
    if (e1->typecheck(tenv) != Int || e2->typecheck(tenv) != Int) {
        classtable->semant_error(tenv.c->get_filename(), this) << "non-Int arguments: " << e1->typecheck(tenv) << " - " << e2->typecheck(tenv) << std::endl;
        type = Object;
    } else {
        type = Int;
    }
    return type;
}

Symbol mul_class::typecheck(type_env &tenv) {
    if (e1->typecheck(tenv) != Int || e2->typecheck(tenv) != Int) {
        classtable->semant_error(tenv.c->get_filename(), this) << "non-Int arguments: " << e1->typecheck(tenv) << " * " << e2->typecheck(tenv) << std::endl;
        type = Object;
    } else {
        type = Int;
    }
    return type;
}

Symbol divide_class::typecheck(type_env &tenv) {
    if (e1->typecheck(tenv) != Int || e2->typecheck(tenv) != Int) {
        classtable->semant_error(tenv.c->get_filename(), this) << "non-Int arguments: " << e1->typecheck(tenv) << " / " << e2->typecheck(tenv) << std::endl;
        type = Object;
    } else {
        type = Int;
    }
    return type;
}

Symbol neg_class::typecheck(type_env &tenv) {
    type = e1->typecheck(tenv);
    if (type != Int) {
        classtable->semant_error(tenv.c->get_filename(), this) << "Argument of '~' has type " << e1->typecheck(tenv) << " instead of Int." << std::endl;
        type = Int;
    }
    return type;
}

Symbol lt_class::typecheck(type_env &tenv) {
    if (e1->typecheck(tenv) != Int || e2->typecheck(tenv) != Int) {
        classtable->semant_error(tenv.c->get_filename(), this) << "non-Int arguments: " << e1->typecheck(tenv) << " < " << e2->typecheck(tenv) << std::endl;
    }
    type = Bool;
    return type;
}

Symbol eq_class::typecheck(type_env &tenv) {
    Symbol t1 = e1->typecheck(tenv);
    Symbol t2 = e2->typecheck(tenv);

    if ((t1 == Int || t1 == Bool || t1 == Str || t2 == Int || t2 == Bool || t2 == Str) && t1 != t2) {
        classtable->semant_error(tenv.c->get_filename(), this) << "Illegal comparison with a basic type." << std::endl;
    }
    type = Bool;
    return type;
}

Symbol leq_class::typecheck(type_env &tenv) {
    if (e1->typecheck(tenv) != Int || e2->typecheck(tenv) != Int) {
        classtable->semant_error(tenv.c->get_filename(), this) << "non-Int arguments: " << e1->typecheck(tenv) << " <= " << e2->typecheck(tenv) << std::endl;
    }
    type = Bool;
    return type;
}

Symbol object_class::typecheck(type_env &tenv) {
    Symbol *t = tenv.o.lookup(name);
    if (!t) {
        classtable->semant_error(tenv.c->get_filename(), this) << "Undeclared identifier " << name << "." << std::endl;
        type = Object;
        return type;
    }
    type = *t;
    return type;
}

Symbol assign_class::typecheck(type_env &tenv) {
    type = Object;

    if (name == self) {
        classtable->semant_error(tenv.c->get_filename(), this) << "Cannot assign to 'self'." << std::endl;
        return type;
    }

    Symbol *t = tenv.o.lookup(name);
    Symbol t1 = expr->typecheck(tenv);

    if (!t) {
        classtable->semant_error(tenv.c->get_filename(), this) << "Assignment to undeclared variable " << name << "." << std::endl;
        return type;
    }

    if (!is_subclass(t1, *t, tenv)) {
        classtable->semant_error(tenv.c->get_filename(), this) << "Type " << t1 << " of assigned expression does not conform to declared type " << *t << " of identifier " << name << "." << std::endl;
        return type;
    }
    type = t1;
    return type;
}

Symbol let_class::typecheck(type_env &tenv) {

    Symbol t0 = type_decl;
    Symbol t1 = init->typecheck(tenv);

    if (t1 != No_type && !is_subclass(t1, t0, tenv)) {
        classtable->semant_error(tenv.c->get_filename(), this) <<
            "Inferred type " << t1 << " of initialization of " << identifier <<
            " does not conform to identifier's declared type " << t0 << "." << std::endl;
    }

    tenv.o.enterscope();

    if (identifier != self) {
        tenv.o.addid(identifier, new Symbol(t0));
    } else {
        classtable->semant_error(tenv.c->get_filename(), this) <<
            "'self' cannot be bound in a 'let' expression." << std::endl;
    }

    type = body->typecheck(tenv);

    tenv.o.exitscope();

    return type;
}

Symbol typcase_class::typecheck(type_env &tenv) {

    Symbol t0 = expr->typecheck(tenv);

    std::vector<Symbol> used;

    for (int i = cases->first(); cases->more(i); i = cases->next(i)) {
        Symbol prev_type = type;

        Case c = cases->nth(i);
        Symbol type_decl = c->get_type_decl();

        if (std::find(used.begin(), used.end(), type_decl) == used.end() ) {
            used.push_back(type_decl);
        } else {
            classtable->semant_error(tenv.c->get_filename(), this) <<
                "Duplicate branch " << type_decl << " in case statement." << std::endl;

            type = Object;
            return type;
        }

        tenv.o.enterscope();

        tenv.o.addid(c->get_name(), new Symbol(type_decl));
        type = c->get_expr()->typecheck(tenv);

        if (i > 0) {
            type = cls_join(type, prev_type, tenv);
        }

        tenv.o.exitscope();
    }

    return type;
}

Symbol method_class::typecheck(type_env &tenv) {

    tenv.o.enterscope();

    tenv.o.addid(self, new Symbol(SELF_TYPE));

    method_class *m = method_in_class(tenv.c->get_name(), name);

    if (this != m) {
        classtable->semant_error(tenv.c->get_filename(), this) <<
            "Method " << name << " is multiply defined." << std::endl;
    }

    auto c_iter = class_map.find(tenv.c->get_parent());
    if (c_iter != class_map.end()) {
        m = lookup_method(c_iter->second->get_name(), name);
    }

    bool derived_formals_are_less = false;

    std::vector<Symbol> defined;

    int i;
    for (i = formals->first(); formals->more(i); i = formals->next(i)) {
        Formal f = formals->nth(i);
        Symbol f_name = f->get_name();
        Symbol type_decl = f->get_type_decl();

        if (f_name == self) {
            classtable->semant_error(tenv.c->get_filename(), this) <<
                "'self' cannot be the name of a formal parameter." << std::endl;
        } else {
            if (type_decl == SELF_TYPE) {
                classtable->semant_error(tenv.c->get_filename(), this) <<
                    "Formal parameter " << f_name << " cannot have type SELF_TYPE." << std::endl;
            } else if (!cls_is_defined(type_decl)) {
                classtable->semant_error(tenv.c->get_filename(), this) <<
                    "Class " << type_decl << " of formal parameter " <<
                    f_name << " is undefined." << std::endl;
            }

            if (std::find(defined.begin(), defined.end(), f_name) == defined.end() ) {
                defined.push_back(f_name);
            } else {
                classtable->semant_error(tenv.c->get_filename(), this) <<
                    "Formal parameter " << f_name << " is multiply defined." << std::endl;
            }

            tenv.o.addid(f->get_name(), new Symbol(type_decl));
        }

        if (m) {
            Formals formals_derived = m->get_formals();
            if (formals_derived->more(i)) {
                Formal f_derived = formals_derived->nth(i);
                if (f_derived->get_type_decl() != type_decl) {
                    classtable->semant_error(tenv.c->get_filename(), this) <<
                        "In redefined method " << name << ", parameter type "
                         << type_decl << " is different from original type "
                         << f_derived->get_type_decl() << std::endl;
                }
            } else {
                derived_formals_are_less = true;
            }
        }
    }

    if (m && (derived_formals_are_less || m->get_formals()->more(i))) {
        classtable->semant_error(tenv.c->get_filename(), this) <<
            "Incompatible number of formal parameters in redefined method "
            << name << "." << std::endl;
    }

    Symbol t0_ = expr->typecheck(tenv);
    tenv.o.exitscope();

    if (! is_subclass(t0_, return_type, tenv)) {
        classtable->semant_error(tenv.c->get_filename(), this) <<
            "Inferred return type " << t0_ << " of method init does not conform "
            "to declared return type " << return_type << "." << std::endl;
    }

    return Object;
}

Symbol dispatch_class::typecheck(type_env &tenv) {
    Symbol t0 = expr->typecheck(tenv);
    Symbol t0_ = t0;
    if (t0_ == SELF_TYPE) {
        t0_ = tenv.c->get_name();
    }

    method_class *method = lookup_method(t0_, name);
    if (!method) {
        classtable->semant_error(tenv.c->get_filename(), this) <<
            "Dispatch to undefined method " << name << "." << std::endl;

        type = Object;
        return type;
    }

    Formals formals = method->get_formals();

    bool formals_are_less;
    int i;

    for (i = actual->first(); actual->more(i); i = actual->next(i)) {
        Symbol t_actual = actual->nth(i)->typecheck(tenv);

        if (formals->more(i)) {
            Formal f = formals->nth(i);
            Symbol t_formal = f->get_type_decl();

            if (!is_subclass(t_actual, t_formal, tenv)) {
                classtable->semant_error(tenv.c->get_filename(), this) <<
                    "In call of method " << name << ", type " << t_actual <<
                    " of parameter " << f->get_name() << " does not conform to "
                    "declared type " << t_formal << "." << std::endl;
            }
        } else {
            formals_are_less = true;
        }
    }

    if (formals_are_less || formals->more(i)) {
        classtable->semant_error(tenv.c->get_filename(), this) <<
            "Method " << name << " called with wrong number of arguments." << std::endl;
    }

    type = method->get_return_type();
    if (type == SELF_TYPE) {
        type = t0;
    }

    return type;
}

Symbol static_dispatch_class::typecheck(type_env &tenv) {
    
    Symbol t0 = expr->typecheck(tenv);
    Symbol t = type_name;

    if (!is_subclass(t0, t, tenv)) {
        classtable->semant_error(tenv.c->get_filename(), this) <<
            "Expression type " << t0 << " does not conform to declared static "
            "dispatch type " << t << "." << std::endl;
    }

    method_class *method = lookup_method(t, name);
    if (!method) {
        classtable->semant_error(tenv.c->get_filename(), this) <<
            "Dispatch to undefined method " << name << "." << std::endl;
        type = Object;
        return type;
    }

    Formals formals = method->get_formals();

    bool formals_are_less;
    int i;

    for (i = actual->first(); actual->more(i); i = actual->next(i)) {
        Symbol t_actual = actual->nth(i)->typecheck(tenv);

        if (formals->more(i)) {
            Formal f = formals->nth(i);
            Symbol t_formal = f->get_type_decl();

            if (!is_subclass(t_actual, t_formal, tenv)) {
                classtable->semant_error(tenv.c->get_filename(), this) <<
                    "In call of method " << name << ", type " << t_actual <<
                    " of parameter " << f->get_name() << " does not conform to "
                    "declared type " << t_formal << "." << std::endl;
            }
        } else {
            formals_are_less = true;
        }
    }

    if (formals_are_less || formals->more(i)) {
        classtable->semant_error(tenv.c->get_filename(), this) <<
            "Method " << name << " called with wrong number of arguments." << std::endl;
    }

    type = method->get_return_type();
    if (type == SELF_TYPE) {
        type = t0;
    }

    return type;
		
}


void build_method_env() {
    for (auto iter = class_map.begin(); iter != class_map.end(); iter++) {
        Class_ cls = iter->second;

        Features features = cls->get_features();
        for (int i = features->first(); features->more(i); i = features->next(i)) {
            Feature f = features->nth(i);

            method_class *method = dynamic_cast<method_class *>(f);
            if (!method) {
                continue;
            }

            method_env[std::make_pair(cls->get_name(), f->get_name())] = method;
        }
    }}


void build_initial_obj_env(type_env &tenv) {

    for (auto c_iter = class_map.find(tenv.c->get_parent());
         c_iter != class_map.end();
         c_iter = class_map.find(c_iter->second->get_parent())
         ) {

        Features features = c_iter->second->get_features();
        for (int i = features->first(); features->more(i); i = features->next(i)) {
            Feature f = features->nth(i);

            attr_class *attribute = dynamic_cast<attr_class *>(f);
            if (!attribute) {
                continue;
            }

            tenv.o.addid(attribute->get_name(), new Symbol(attribute->get_type_decl()));
        }
    }

    Features features = tenv.c->get_features();
    for (int i = features->first(); features->more(i); i = features->next(i)) {
        Feature f = features->nth(i);

        attr_class *attribute = dynamic_cast<attr_class *>(f);
        if (!attribute) {
            continue;
        }

        if (tenv.o.lookup(attribute->get_name())) {
            classtable->semant_error(tenv.c->get_filename(), attribute) <<
                "Attribute " << attribute->get_name() << " is already defined "
                "either in the same class or in a superclass." << std::endl;
        } else {
            tenv.o.addid(attribute->get_name(), new Symbol(attribute->get_type_decl()));
        }
    }

    tenv.o.addid(self, new Symbol(SELF_TYPE));
}

void class__class::check() {
    type_env tenv;
    tenv.c = this;
    tenv.o.enterscope();

    build_initial_obj_env(tenv);

    Features features = tenv.c->get_features();
    for (int i = features->first(); features->more(i); i = features->next(i)) {
        features->nth(i)->typecheck(tenv);
    }

    tenv.o.exitscope();
}

void program_class::check() {
    for (int i = classes->first(); classes->more(i); i = classes->next(i)) {
        classes->nth(i)->check();
    }
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
    if (classtable->errors()) {
        goto exit_error;
    }

    /* some semantic analysis code may go here */

    build_method_env();

    check();

    if (classtable->errors()) {
        exit_error:
            cerr << "Compilation halted due to static semantic errors." << endl;
            exit(1);
    }
}


