program factorization;

procedure factorization(n: integer);
var max, i: integer;
begin
    write('Factors of: ');
    writeln(n);

    if n < 4 then
    begin
        writeln(n);
        exit;
    end;

    while ((n mod 2) = 0) do
    begin
        writeln(2);
        n := n div 2;
    end;

    while ((n mod 3) = 0) do
    begin
        writeln(3);
        n := n div 3;
    end;

    max := n;
    i := 5;
    while i <= max do
    begin
        while ((n mod i) = 0) do
        begin
            writeln(i);
            n := n div i;
        end;
        i := i + 2;
        while ((n mod i) = 0) do
        begin
            writeln(i);
            n := n div i;
        end;
        i := i + 4;
    end;
    if n <> 1 then writeln(n);
end;

begin
    factorization(0);
    factorization(1);
    factorization(2);
    factorization(3);
    factorization(4);
    factorization(5);
    factorization(6);
    factorization(7);
    factorization(8);
    factorization(9);
    factorization(10);
    factorization(11);
    factorization(12);
    factorization(13);
    factorization(14);
    factorization(15);
    factorization(16);
    factorization(17);
    factorization(100);
    factorization(131);
    factorization(133);
end.
