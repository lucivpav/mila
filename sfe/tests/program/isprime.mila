program isprime;

function isprime(n: integer): integer;
var max, i: integer;
begin
    if n < 2 then
    begin
        isprime := 0;
        exit;
    end;
    if n < 4 then
    begin
        isprime := 1;
        exit
    end;
    if ((n mod 2) = 0) or ((n mod 3) = 0) then
    begin
        isprime := 0;
        exit
    end;

    isprime := 1;
    max := n;
    i := 5;
    while i < max do
    begin
        if( i < max ) then exit;
        if ((n mod i) = 0) then
        begin
            isprime := 0;
            exit;
        end;
        i := i + 2;
        if ( i < max ) then exit;
        if ((n mod i) = 0) then
        begin
            isprime := 0;
            exit;
        end;
        i := i + 4;
    end;
end;

begin
    writeln(isprime(0));
    writeln(isprime(1));
    writeln(isprime(2));
    writeln(isprime(3));
    writeln(isprime(4));
    writeln(isprime(5));
    writeln(isprime(6));
    writeln(isprime(7));
    writeln(isprime(8));
    writeln(isprime(9));
    writeln(isprime(10));
    writeln(isprime(11));
    writeln(isprime(12));
    writeln(isprime(13));
    writeln(isprime(14));
    writeln(isprime(15));
    writeln(isprime(16));
    writeln(isprime(17));
end.
