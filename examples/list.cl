class List inherits A2I{
    item: Object;
    next: List;
    
    init(i: Object, n: List): List {
        {
            item <- i;
            next <- n;
            self;
        }
    };

    flatten(): String {
        let string: String <-
            case item of
                i: Int => i2a(i);
                s: String => s;
                o: Object => { abort(); ""; };
            esac
        in
            if (isvoid next) then -- if next is null (void)
                string
            else
                string.concat(next.flatten())
            fi
    };
};

class Main inherits IO {
    main(): Object {
        let first: String <- "Hello ",
            second: String <- "World!",
            newline: String <- "\n",
            nil: List, -- no null exists in cool
            list: List <- (new List).init(first, (new List).init(second,(new List).init(123, (new List).init(newline, nil))))
        in
            out_string(list.flatten())
    };
};
