class List {
    item : String;
    next : List;

    init(i : String, n : List) : List {
        {
            item <- i;
            next <- n;
            self;
        }
    };

    getNext() : List {
        next
    };

    getItem() : String {
        item
    };

    flatten() : String {
        if (isvoid next) then
            item
        else
            item.concat(next.flatten())
        fi
    };

    get(i : Int) : List {
        let current : List <- self in 
        {
            while not (i = 0) loop 
            {
                current <- current.getNext();
                i <- i - 1;
            }
            pool;
            current;
        }
    };
};

class Stack{
    top : Int <- 0;
    list : List <- (new List);
    io: IO <- (new IO);

    push(item : String) : Stack {
        {
            list <- (new List).init(item, list);
            top <- top + 1;
            self;
        }
    };

    tostring() : String {
        list.flatten()
    };

    pop() : String {
        let result : String <- list.getItem() in
        {
            list <- (new List).init(list.getNext().getItem(),list.getNext().getNext());
            top <- top - 1;
            result;
        }
    };

    getsize() : Int {
        top
    };
};

class Main inherits IO {
    main() : Object {
        let s : Stack <- (new Stack) in 
        {
            s.push("A");
            s.push("B");
            s.push("C");
            out_string(s.tostring().concat("\n"));

            while not (s.getsize() = 0) loop
                {
                    out_string(s.tostring().concat("\n"));
                    s.pop();
                }
            pool;
            s;
        }
    };
};

